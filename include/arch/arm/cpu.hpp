#pragma once

#include <cstdint>
#include <lib/io.hpp>

namespace kernel::cpu {

enum class cpu_mode {
    usr = 0b10000,
    fiq = 0b10001,
    irq = 0b10010,
    svc = 0b10011,
    mon = 0b10110,
    abt = 0b10111,
    hyp = 0b11010,
    und = 0b11011,
    sys = 0b11111,
};
enum class cpu_register {
    r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12,
    sp, lr, pc,
    spsr
};
uint32_t read_register(cpu_mode mode, cpu_register reg);

enum class cpu_endianness {
    little = 0,
    big = 1
};

struct psr {
    struct condition_bits {
        bool negative;
        bool zero;
        bool carry;
        bool overflow;
    };
    struct interrupt_mask_bits {
        bool async_abort;
        bool irq;
        bool fiq;
    };

    condition_bits conditions() const;
    cpu_endianness endianness() const;
    interrupt_mask_bits interrupt_mask() const;
    bool thumb() const;
    cpu_mode mode() const;

    uint32_t raw_value;

    psr() = default;
    psr(uint32_t value) : raw_value(value) {}

    static psr current();
    static psr saved();
};

}

namespace kernel {
    void kprint_value(ostream& out, const char*&, cpu::cpu_mode value);
    void kprint_value(ostream& out, const char*&, cpu::cpu_register value);
    void kprint_value(ostream& out, const char*&, cpu::psr value);
}
