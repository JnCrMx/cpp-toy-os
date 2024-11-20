#pragma once

#include <arch/arm/interrupts.hpp>

namespace kernel {

cpu::interrupts::interrupt_result handle_svc(
    cpu::interrupts::interrupt_context& context,
    void*);

void register_svc(uint32_t svc_number, cpu::interrupts::interrupt_handler handler, void* userdata);

}
