#include <kernel/supervisor.hpp>

#include <arch/arm/interrupts.hpp>
#include <kernel/basic.hpp>
#include <kernel/debug.hpp>

#include <utility>

namespace kernel {

static std::pair<cpu::interrupts::interrupt_handler, void*> svc_handlers[256]{};

cpu::interrupts::interrupt_result handle_svc(cpu::interrupts::interrupt_context& context, void*) {
    auto [handler, userdata] = svc_handlers[context.registers.r[0]];
    if(handler) {
        return handler(context, userdata);
    } else {
        return handle_exception(context, nullptr);
    }
    return cpu::interrupts::interrupt_result::next;
}

void register_svc(uint32_t svc_number, cpu::interrupts::interrupt_handler handler, void* userdata) {
    if(svc_number >= 256) {
        panic("Cannot register SVC handler for SVC number greater than 255.");
    }
    if(svc_handlers[svc_number].first) {
        debug::kwarn("Overwriting SVC handler for SVC number {}.", svc_number);
    }
    svc_handlers[svc_number] = {handler, userdata};
}

}
