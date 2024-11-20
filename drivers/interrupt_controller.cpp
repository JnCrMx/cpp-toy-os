#include <drivers/interrupt_controller.hpp>

#include <cstddef>
#include <utility>

namespace kernel::driver::interrupts {

constexpr std::uintptr_t INTERRUPT_BASE = (0x7E00B000 - 0x3F000000);
struct interrupt_controller {
    uint32_t padding[128];
    uint32_t irq_basic_pending;
    uint32_t irq_pending[2];
    uint32_t fiq_control;
    uint32_t enable_irqs[2];
    uint32_t enable_basic_irqs;
    uint32_t disable_irqs[2];
    uint32_t disable_basic_irqs;
};
static_assert(offsetof(interrupt_controller, irq_basic_pending) == 0x200);
static_assert(offsetof(interrupt_controller, irq_pending[0]) == 0x204);
static_assert(offsetof(interrupt_controller, irq_pending[1]) == 0x208);
static_assert(offsetof(interrupt_controller, irq_pending[1]) == 0x208);
static_assert(offsetof(interrupt_controller, fiq_control) == 0x20c);
static_assert(offsetof(interrupt_controller, enable_irqs[0]) == 0x210);
static_assert(offsetof(interrupt_controller, enable_irqs[1]) == 0x214);
static_assert(offsetof(interrupt_controller, enable_basic_irqs) == 0x218);
static_assert(offsetof(interrupt_controller, disable_irqs[0]) == 0x21c);
static_assert(offsetof(interrupt_controller, disable_irqs[1]) == 0x220);
static_assert(offsetof(interrupt_controller, disable_basic_irqs) == 0x224);

static volatile struct interrupt_controller *const interrupt_controller = reinterpret_cast<struct interrupt_controller*>(INTERRUPT_BASE);

void enable_source(interrupt_source source) {
    uint32_t s = std::to_underlying(source);
    if(s >= 32) {
        interrupt_controller->enable_irqs[1] = (1<<(s-32));
    } else {
        interrupt_controller->enable_irqs[0] = (1<<s);
    }
}
void disable_source(interrupt_source source) {
    uint32_t s = std::to_underlying(source);
    if(s >= 32) {
        interrupt_controller->disable_irqs[1] = (1<<(s-32));
    } else {
        interrupt_controller->disable_irqs[0] = (1<<s);
    }
}
bool check_interrupt(interrupt_source source) {
    uint32_t s = std::to_underlying(source);
    if(s >= 32) {
        return interrupt_controller->irq_pending[1] & (1<<(s-32));
    } else {
        return interrupt_controller->irq_pending[0] & (1<<s);
    }
}

}
