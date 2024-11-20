#pragma once

#include <cstdint>
#include <type_traits>

namespace kernel::cpu::interrupts {
    struct interrupt_context;
}

namespace kernel::driver::timer {

enum class system_timer {
    sys_timer0 = 0,
    sys_timer1 = 1,
    sys_timer2 = 2,
    sys_timer3 = 3,
};

using interrupt_context = cpu::interrupts::interrupt_context;
using timer_func = std::add_pointer_t<void(system_timer, uint32_t, interrupt_context&, void*)>;

void setup(system_timer timer, uint32_t interval, timer_func func, void* userdata);
void reset(system_timer timer, interrupt_context& context);

}
