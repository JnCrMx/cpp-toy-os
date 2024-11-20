#pragma once

#include <lib/io.hpp>

#include <initializer_list>
#include <cstdint>
#include <type_traits>

namespace kernel::cpu::interrupts {
    struct ivt{};
    extern "C" ivt _ivt;

    void setup_vector_table();
    void enable();

    enum class interrupt_type : uint32_t {
        undefined_instruction = 0,
        software_interrupt = 1,
        prefetch_abort = 2,
        data_abort = 3,
        not_used = 4,
        irq = 5,
        fiq = 6,
        INTERRUPT_TYPE_COUNT
    };
    struct interrupt_registers {
        uint32_t r[13];
        uint32_t pc;
    };
    enum class interrupt_result {
        repeat, next, event_loop, custom
    };

    struct interrupt_context {
        interrupt_type type;
        interrupt_registers& registers;
        uint32_t& address;
        interrupt_result& result;
    };

    using interrupt_handler = std::add_pointer_t<interrupt_result(interrupt_context& context, void* userdata)>;
    void set_handler(interrupt_type type, interrupt_handler handler, void* userdata);
    void set_handler(std::initializer_list<interrupt_type> types, interrupt_handler handler, void* userdata);

    extern "C" void handle_interrupt(interrupt_type type, interrupt_registers* registers);
}

namespace kernel {
    void kprint_value(ostream& out, const char*&, cpu::interrupts::interrupt_type value);
}
