#pragma once

#include <source_location>
#include <arch/arm/interrupts.hpp>

namespace kernel {

[[noreturn]] void reboot();
[[noreturn]] void shutdown();
[[noreturn]] void panic(const char* message = nullptr, std::source_location loc = std::source_location::current());

cpu::interrupts::interrupt_result handle_exception(
    cpu::interrupts::interrupt_context& context,
    void*);

}
