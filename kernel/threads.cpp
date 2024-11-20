#include <kernel/threads.hpp>

#include <arch/arm/cpu.hpp>
#include <arch/arm/interrupts.hpp>
#include <drivers/interrupt_controller.hpp>
#include <drivers/timer.hpp>
#include <config.hpp>
#include <kernel/supervisor.hpp>
#include <lib/format.hpp>
#include <lib/string.hpp>
#include <lib/queue.hpp>

#include <cstdint>
#include <limits>
#include <sys/types.h>
#include <utility>

namespace kernel::threads {

using driver::timer::system_timer;
using cpu::interrupts::interrupt_context;
using cpu::interrupts::interrupt_result;

void kprint_value(ostream& out, const char*& format, thread_create_error value) {
    using namespace ::kernel::detail;
    format_options options{};
    read_options(format, options);

    switch(value) {
        case thread_create_error::no_free_thread: out << aligned("no_free_thread", options); return;
        case thread_create_error::out_of_argument_memory: out << aligned("out_of_argument_memory", options); return;
        case thread_create_error::no_more_thread_ids: out << aligned("no_more_thread_ids", options); return;
    }
}

enum class thread_state {
    running,
    ready,
    waiting,
    empty,
};
enum class thread_wait_type {
    sleep,
    uart,
};

constexpr uint32_t default_psr = std::to_underlying(cpu::cpu_mode::usr);

[[noreturn]] void terminate() {
    __asm__ __volatile__("mov r0, #4\nmov r1, #0\nsvc #0" : : : "r0", "r1");
    __builtin_unreachable();
}
void yield() {
    __asm__ __volatile__("mov r0, #5\nsvc #0" : : : "r0");
}

struct thread_control_block : public queue_mixin<thread_control_block> {
    struct registers {
        uint32_t r[13]{};
        uint32_t lr = reinterpret_cast<uint32_t>(&terminate);
        uint32_t sp{};
        uint32_t pc{};
        uint32_t psr = default_psr;
    } registers{};

    thread_control_block() = default;
    thread_control_block(entry_point pc, void* sp, uint32_t arg) : thread_control_block() {
        registers.pc = reinterpret_cast<uint32_t>(pc);
        registers.sp = reinterpret_cast<uint32_t>(sp);
        registers.r[0] = arg;
        state = thread_state::ready;
    }

    thread_state state = thread_state::empty;
    thread_wait_type wait_type{};
    uint32_t wait_arg{};

    bool is_empty() const {
        return state == thread_state::empty;
    }
    bool is_valid() const {
        return state != thread_state::empty;
    }

    void save_registers(const interrupt_context& ctx) {
        memcpy(registers.r, ctx.registers.r, sizeof(registers.r));
        __asm__("mrs %0, lr_usr" : "=r"(registers.lr));
        __asm__("mrs %0, sp_usr" : "=r"(registers.sp));
        __asm__("mrs %0, spsr" : "=r"(registers.psr));
        registers.pc = ctx.address + 4; // interrupt context points to the last finished instruction
    }
    void restore_registers(interrupt_context& ctx) const {
        memcpy(ctx.registers.r, registers.r, sizeof(registers.r));
        __asm__("msr lr_usr, %0" : : "r"(registers.lr));
        __asm__("msr sp_usr, %0" : : "r"(registers.sp));
        __asm__("msr spsr, %0" : : "r"(registers.psr));
        ctx.address = registers.pc - 4; // the interrupt handler will add the 4 again to go to the next instruction
    }
};

static thread_control_block threads[config::thread_count];
static uintptr_t thread_stacks[config::thread_count];

static struct thread_control_block* thread_running = NULL;
static queue<thread_control_block> thread_ready_queue{};
static queue<thread_control_block> thread_waiting_queue{};

void scheduler_timer_tick(system_timer, uint32_t, interrupt_context& ctx, void*);
interrupt_result terminate_thread(interrupt_context& ctx, void*);
interrupt_result yield_thread(interrupt_context& ctx, void*);

void init() {
    uintptr_t base = config::end_of_kernel + 6*config::mode_stack_size;

    for(unsigned int i=0; i<config::thread_count; i++) {
        thread_stacks[i] = base + i*config::thread_stack_size;
        threads[i] = {};
    }
    threads[0].state = thread_state::running;
    thread_running = &threads[0];

    driver::interrupts::enable_source(driver::interrupts::interrupt_source::sys_timer3);
    driver::timer::setup(system_timer::sys_timer3, config::system_timer_interval, scheduler_timer_tick, nullptr);

    kernel::register_svc(0x04, &terminate_thread, nullptr);
    kernel::register_svc(0x05, &yield_thread, nullptr);
}

static unsigned int thread_id = 1;

namespace detail {
    std::expected<prepared_thread, thread_create_error> prepare_thread(entry_point entry, std::size_t args_size) {
        if(thread_id == std::numeric_limits<unsigned int>::max())
            return std::unexpected(thread_create_error::no_more_thread_ids);

        if(args_size > config::thread_stack_size)
            return std::unexpected(thread_create_error::out_of_argument_memory);

        thread_control_block* block = nullptr;
        char* stackpointer = nullptr;
        for(unsigned int i=0; i<config::thread_count; i++) {
            if(threads[i].state == thread_state::empty) {
                block = &threads[i];
                stackpointer = reinterpret_cast<char*>(thread_stacks[i]);
                break;
            }
        }
        if(!block)
            return std::unexpected(thread_create_error::no_free_thread);

        stackpointer -= args_size;
        char* args_pointer = stackpointer;
        stackpointer -= reinterpret_cast<uintptr_t>(stackpointer) % 8;

        *block = thread_control_block(entry, stackpointer, reinterpret_cast<uint32_t>(args_pointer));
        return prepared_thread{thread_id++, block, args_pointer};
    }

    std::expected<unsigned int, thread_create_error> start_thread(prepared_thread thread) {
        thread_ready_queue.add(reinterpret_cast<thread_control_block*>(thread.block));
        return thread.id;
    }
};

std::expected<unsigned int, thread_create_error> create(entry_point entry, const void* args, std::size_t args_size) {
    auto prepared = detail::prepare_thread(entry, args_size);
    if(!prepared)
        return std::unexpected(prepared.error());

    memcpy(prepared->arg(), args, args_size);

    auto res = detail::start_thread(*prepared);
    if(!res)
        return std::unexpected(res.error());
    return *res;
}

void thread_preempt() {
    thread_running->state = thread_state::ready;
    thread_ready_queue.add(thread_running);
    thread_running = nullptr;
}
thread_control_block* thread_continue_next() {
    thread_running = thread_ready_queue.remove();
    if(!thread_running) {
        panic("No more threads to run. Event loop thread probably crashed.");
    }
    thread_running->state = thread_state::running;
    return thread_running;
}
thread_control_block* thread_current() {
    return thread_running;
}
thread_control_block* thread_peek_next() {
    return thread_ready_queue.peek();
}

void scheduler_timer_tick(system_timer, uint32_t, interrupt_context& ctx, void*) {
    ctx.result = yield_thread(ctx, nullptr);
}

interrupt_result terminate_thread(interrupt_context& ctx, void*) {
    thread_current()->state = thread_state::empty;

    auto next = thread_continue_next();
    next->restore_registers(ctx);
    return interrupt_result::next;
}

interrupt_result yield_thread(interrupt_context& ctx, void*) {
    if(!thread_peek_next()) {
        return interrupt_result::next;
    }

    auto current = thread_current();
    if(current) {
        current->save_registers(ctx);
        thread_preempt();
    }

    auto next = thread_continue_next();
    if(next != current) {
        next->restore_registers(ctx);
    }
    return interrupt_result::next;
}

}
