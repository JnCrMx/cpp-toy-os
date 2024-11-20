#include <drivers/timer.hpp>
#include <kernel/debug.hpp>

#include <cstdint>
#include <tuple>
#include <utility>

namespace kernel::driver::timer {

constexpr std::uintptr_t TIMER_BASE = (0x7E003000 - 0x3F000000);
constexpr unsigned int NUM_TIMERS = 4;

struct system_timer_controller {
    uint32_t cs;
    uint32_t clo;
    uint32_t chi;
    uint32_t cc[NUM_TIMERS];
};
static volatile struct system_timer_controller *const timer_controller = reinterpret_cast<struct system_timer_controller*>(TIMER_BASE);

static std::tuple<uint32_t, timer_func, void*> timer_configs[NUM_TIMERS]{};

void setup(system_timer timer, uint32_t interval, timer_func func, void *userdata) {
    unsigned int t = std::to_underlying(timer);

    timer_configs[t] = {interval, func, userdata};

    uint32_t current = timer_controller->clo;
    uint32_t next = current + interval;
    timer_controller->cc[t] = next;
    timer_controller->cs = (1<<t);

    debug::kdebug("Configured timer {} with interval {} (current value is {} and compare is {}).", t, interval, current, next);
}

void reset(system_timer timer, interrupt_context& context) {
    unsigned int t = std::to_underlying(timer);
    const auto& [delay, func, userdata] = timer_configs[t];

    uint32_t current = timer_controller->clo;
    uint32_t next = current + delay;
    timer_controller->cc[t] = next;
    timer_controller->cs = (1<<t);
    debug::ktrace("Reset timer {} compare to {} (current timer value is {})", t, next, current);

    func(timer, current, context, userdata);
}

}
