#include <kernel/events.hpp>

#include <config.hpp>
#include <drivers/serial.hpp>
#include <drivers/interrupt_controller.hpp>
#include <drivers/timer.hpp>
#include <arch/arm/interrupts.hpp>
#include <kernel/debug.hpp>
#include <kernel/threads.hpp>
#include <lib/queue.hpp>

#include <cstdint>
#include <type_traits>

namespace kernel::events {

void configure() {
    cpu::interrupts::set_handler(
        {cpu::interrupts::interrupt_type::irq, cpu::interrupts::interrupt_type::fiq},
        &detail::process_interrupt, nullptr);

    driver::interrupts::enable_source(driver::interrupts::interrupt_source::sys_timer1);
    driver::interrupts::enable_source(driver::interrupts::interrupt_source::uart);

    driver::timer::setup(driver::timer::system_timer::sys_timer1, config::system_timer_interval,
        [](driver::timer::system_timer, uint32_t value, cpu::interrupts::interrupt_context&, void*){
            main_event_loop.fire_event(event{.type = type::system_timer, .data = value});
        }, nullptr);
}

void run_main_event_loop() {
    main_event_loop.current_coroutine = nullptr;
    main_event_loop.run();
}

void event_loop::run() {
    for(;;) {
        step();
        threads::yield();
    }
}
void event_loop::step() {
    if(counter % 10 == 0) {
        fire_event({.type = type::tick, .data = counter});
    }
    process_events();
    counter++;
}
void event_loop::process_events() {
    if(read_pos != write_pos) {
        event e = std::move(event_queue[read_pos++]);
        if(read_pos == config::event_queue_size) {
            read_pos = 0;
        }

        auto& slot = event_awaiters[static_cast<uint32_t>(e.type)];;
        auto* awaiter = slot;
        if(awaiter) {
            slot = nullptr;
            awaiter->complete(e.data);
        }
    }

    if(auto* y = yield_queue.remove()) {
        y->complete();
    }
    if(yield_queue.peek()) {
        panic("Yield queue not empty after processing all yields");
    }
}
void event_loop::fire_event(event &&event) {
    if(write_pos == read_pos-1) {
        debug::kerror("Event queue overrun. Throwing away event of type {} with data {}.",
            static_cast<std::underlying_type_t<type>>(event.type), event.data);
        return;
    }

    event_queue[write_pos++] = std::move(event);
    if(write_pos == config::event_queue_size) {
        write_pos = 0;
    }
}

awaiter* event_loop::register_event_handler(type t, awaiter *a) {
    auto& slot = event_awaiters[static_cast<uint32_t>(t)];
    awaiter* old = slot;
    slot = a;
    return old;
}
void event_loop::yield_coroutine(yield *awaiter) {
    yield_queue.add(awaiter);
}

namespace detail {
    using namespace cpu::interrupts;
    using namespace driver::interrupts;
    interrupt_result process_interrupt(interrupt_context& context, void*) {
        context.result = interrupt_result::next;

        if(check_interrupt(interrupt_source::sys_timer0)) {
            driver::timer::reset(driver::timer::system_timer::sys_timer0, context);
        }
        if(check_interrupt(interrupt_source::sys_timer1)) {
            driver::timer::reset(driver::timer::system_timer::sys_timer1, context);
        }
        if(check_interrupt(interrupt_source::sys_timer2)) {
            driver::timer::reset(driver::timer::system_timer::sys_timer2, context);
        }
        if(check_interrupt(interrupt_source::sys_timer3)) {
            driver::timer::reset(driver::timer::system_timer::sys_timer3, context);
        }
        if(check_interrupt(interrupt_source::uart)) {
            driver::serial::Serial.handle_interrupt();
        }

        return context.result;
    }
}

}
