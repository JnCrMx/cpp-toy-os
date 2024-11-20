#include <drivers/serial.hpp>
#include <drivers/timer.hpp>
#include <arch/arm/cpu.hpp>
#include <arch/arm/interrupts.hpp>
#include <kernel/debug.hpp>
#include <lib/bitfield.hpp>
#include <config.hpp>

#include <initializer_list>
#include <cstdint>
#include <utility>
#include <tuple>

namespace kernel::cpu::interrupts {

void init_stack_pointers()
{
    // usr/sys mode is already initialized
    __asm__("msr sp_fiq, %0" : : "r"(config::stackpointers.fiq));
    __asm__("msr sp_irq, %0" : : "r"(config::stackpointers.irq));
    __asm__("msr sp_svc, %0" : : "r"(config::stackpointers.svc));
    __asm__("msr sp_abt, %0" : : "r"(config::stackpointers.abt));
    __asm__("msr sp_und, %0" : : "r"(config::stackpointers.und));
}

void setup_vector_table()
{
    uintptr_t address = reinterpret_cast<uintptr_t>(&_ivt);
    __asm__ __volatile__("mcr p15, 0, %0, c12, c0, 0" : : "r"(address));
}
void enable()
{
    init_stack_pointers();

    debug::kdebug("We are in mode {} before enabling interrupts.", psr::current().mode());
    __asm__ __volatile__("cpsie if");
    debug::kdebug("We are still alive after enabling interrupts.");
}

static std::tuple<interrupt_handler, void*> interrupt_handlers[std::to_underlying(interrupt_type::INTERRUPT_TYPE_COUNT)];
void set_handler(interrupt_type type, interrupt_handler handler, void* userdata) {
    interrupt_handlers[std::to_underlying(type)] = {handler, userdata};
}
void set_handler(std::initializer_list<interrupt_type> types, interrupt_handler handler, void* userdata) {
    for(auto t : types) {
        set_handler(t, handler, userdata);
    }
}

extern "C" void handle_interrupt(interrupt_type type, interrupt_registers* registers) {
    uint32_t address = registers->pc;
    interrupt_result res = interrupt_result::next;
    switch(type) {
        case interrupt_type::data_abort:
            address -= 8;
            break;
        case interrupt_type::undefined_instruction:
        case interrupt_type::software_interrupt:
            address -= 4;
            break;
        case interrupt_type::prefetch_abort:
            address -= 4;
            break;
        case interrupt_type::irq:
        case interrupt_type::fiq:
            address -= 8;
            break;
        default:
            break;
    }
    auto [handler, userdata] = interrupt_handlers[std::to_underlying(type)];
    if(handler) {
        interrupt_context ctx {
            .type = type,
            .registers = *registers,
            .address = address,
            .result = res
        };
        res = handler(ctx, userdata);
    } else {
        res = interrupt_result::next;
    }

    switch(res) {
        case interrupt_result::next:
            registers->pc = address + 4;
            return;
        case interrupt_result::repeat:
            registers->pc = address;
            return;
        case interrupt_result::event_loop:
            registers->pc = reinterpret_cast<uint32_t>(&events::run_main_event_loop);
            return;
        case interrupt_result::custom:
            return;
    }
}

}

namespace kernel {
    void kprint_value(ostream& out, const char*& format, cpu::interrupts::interrupt_type value) {
        detail::format_options options{};
        detail::read_options(format, options);

        using cpu::interrupts::interrupt_type;
        switch(value) {
            case interrupt_type::undefined_instruction: out << detail::aligned("Undefined Instruction", options); return;
            case interrupt_type::software_interrupt:    out << detail::aligned("Software Interrupt"   , options); return;
            case interrupt_type::prefetch_abort:        out << detail::aligned("Prefetch Abort"       , options); return;
            case interrupt_type::data_abort:            out << detail::aligned("Data Abort"           , options); return;
            case interrupt_type::irq:                   out << detail::aligned("IRQ"                  , options); return;
            case interrupt_type::fiq:                   out << detail::aligned("FIQ"                  , options); return;
            default:                                    out << detail::aligned("INVALID!"             , options); return;
        }
    }
}
