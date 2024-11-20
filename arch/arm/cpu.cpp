#include <arch/arm/cpu.hpp>
#include <lib/format.hpp>

#include <cstdint>

namespace kernel::cpu {

constexpr uint32_t condition_mask_N = (1U<<31);
constexpr uint32_t condition_mask_Z = (1U<<30);
constexpr uint32_t condition_mask_C = (1U<<29);
constexpr uint32_t condition_mask_V = (1U<<28);
constexpr uint32_t endianness_mask  = (1U<<9);
constexpr uint32_t interrupt_mask_A = (1U<<8);
constexpr uint32_t interrupt_mask_I = (1U<<7);
constexpr uint32_t interrupt_mask_F = (1U<<6);
constexpr uint32_t thumb_mask       = (1U<<5);
constexpr uint32_t mode_mask        = 0b11111;

psr psr::current() {
    uint32_t cpsr;
    __asm__("mrs %0, cpsr" : "=r"(cpsr));
    return psr(cpsr);
}
psr psr::saved() {
    uint32_t spsr;
    __asm__("mrs %0, spsr" : "=r"(spsr));
    return psr(spsr);
}
cpu_mode psr::mode() const {
    return static_cast<cpu_mode>(raw_value & mode_mask);
}
cpu_endianness psr::endianness() const {
    return (raw_value & endianness_mask) ? cpu_endianness::big : cpu_endianness::little;
}
psr::interrupt_mask_bits psr::interrupt_mask() const {
    psr::interrupt_mask_bits bits{};
    bits.async_abort = raw_value & interrupt_mask_A;
    bits.irq         = raw_value & interrupt_mask_I;
    bits.fiq         = raw_value & interrupt_mask_F;
    return bits;
}
psr::condition_bits psr::conditions() const {
    psr::condition_bits bits{};
    bits.negative = raw_value & condition_mask_N;
    bits.zero     = raw_value & condition_mask_Z;
    bits.carry    = raw_value & condition_mask_C;
    bits.overflow = raw_value & condition_mask_V;
    return bits;
}
bool psr::thumb() const {
    return raw_value & thumb_mask;
}

uint32_t read_register(cpu_mode mode, cpu_register reg) {
    uint32_t value{};
    if(mode == psr::current().mode()) {
        switch(reg) {
            case cpu_register::sp: __asm__("mov %0, sp" : "=r"(value)); break;
            case cpu_register::lr: __asm__("mov %0, lr" : "=r"(value)); break;
            case cpu_register::spsr: __asm__("mrs %0, spsr" : "=r"(value)); break;
            default: value = 0;
        }
    } else {
        switch(mode) {
            case cpu_mode::usr:
            case cpu_mode::sys:
                switch(reg) {
                    case cpu_register::sp: __asm__("mrs %0, sp_usr" : "=r"(value)); break;
                    case cpu_register::lr: __asm__("mrs %0, lr_usr" : "=r"(value)); break;
                    default: value = 0;
                }
                break;
            case cpu_mode::irq:
                switch(reg) {
                    case cpu_register::sp: __asm__("mrs %0, sp_irq" : "=r"(value)); break;
                    case cpu_register::lr: __asm__("mrs %0, lr_irq" : "=r"(value)); break;
                    case cpu_register::spsr: __asm__("mrs %0, spsr_irq" : "=r"(value)); break;
                    default: value = 0;
                }
                break;
            case cpu_mode::fiq:
                switch(reg) {
                    case cpu_register::sp: __asm__("mrs %0, sp_fiq" : "=r"(value)); break;
                    case cpu_register::lr: __asm__("mrs %0, lr_fiq" : "=r"(value)); break;
                    case cpu_register::spsr: __asm__("mrs %0, spsr_fiq" : "=r"(value)); break;
                    default: value = 0;
                }
                break;
            case cpu_mode::abt:
                switch(reg) {
                    case cpu_register::sp: __asm__("mrs %0, sp_abt" : "=r"(value)); break;
                    case cpu_register::lr: __asm__("mrs %0, lr_abt" : "=r"(value)); break;
                    case cpu_register::spsr: __asm__("mrs %0, spsr_abt" : "=r"(value)); break;
                    default: value = 0;
                }
                break;
            case cpu_mode::und:
                switch(reg) {
                    case cpu_register::sp: __asm__("mrs %0, sp_und" : "=r"(value)); break;
                    case cpu_register::lr: __asm__("mrs %0, lr_und" : "=r"(value)); break;
                    case cpu_register::spsr: __asm__("mrs %0, spsr_und" : "=r"(value)); break;
                    default: value = 0;
                }
                break;
            case cpu_mode::svc:
                switch(reg) {
                    case cpu_register::sp: __asm__("mrs %0, sp_svc" : "=r"(value)); break;
                    case cpu_register::lr: __asm__("mrs %0, lr_svc" : "=r"(value)); break;
                    case cpu_register::spsr: __asm__("mrs %0, spsr_svc" : "=r"(value)); break;
                    default: value = 0;
                }
                break;
            default:
                value = 0;
        }
    }
    return value;
}

}

namespace kernel {
    void kprint_value(ostream& out, const char*& format, cpu::cpu_mode value) {
        detail::format_options options{};
        detail::read_options(format, options);

        using cpu::cpu_mode;
        switch(value) {
            case cpu_mode::usr: out << detail::aligned("User", options); return;
            case cpu_mode::fiq: out << detail::aligned("FIQ", options); return;
            case cpu_mode::irq: out << detail::aligned("IRQ", options); return;
            case cpu_mode::svc: out << detail::aligned("Supervisor", options); return;
            case cpu_mode::mon: out << detail::aligned("Monitor", options); return;
            case cpu_mode::abt: out << detail::aligned("Abort", options); return;
            case cpu_mode::hyp: out << detail::aligned("Hyp", options); return;
            case cpu_mode::und: out << detail::aligned("Undefined", options); return;
            case cpu_mode::sys: out << detail::aligned("System", options); return;
            default: out << detail::aligned("Invalid", options); return;
        }
    }
    void kprint_value(ostream& out, const char*& format, cpu::cpu_register value) {
        detail::format_options options{};
        detail::read_options(format, options);

        using cpu::cpu_register;
        switch(value) {
            case cpu_register::r0: out << detail::aligned("r0", options); return;
            case cpu_register::r1: out << detail::aligned("r1", options); return;
            case cpu_register::r2: out << detail::aligned("r2", options); return;
            case cpu_register::r3: out << detail::aligned("r3", options); return;
            case cpu_register::r4: out << detail::aligned("r4", options); return;
            case cpu_register::r5: out << detail::aligned("r5", options); return;
            case cpu_register::r6: out << detail::aligned("r6", options); return;
            case cpu_register::r7: out << detail::aligned("r7", options); return;
            case cpu_register::r8: out << detail::aligned("r8", options); return;
            case cpu_register::r9: out << detail::aligned("r9", options); return;
            case cpu_register::r10: out << detail::aligned("r10", options); return;
            case cpu_register::r11: out << detail::aligned("r11", options); return;
            case cpu_register::r12: out << detail::aligned("r12", options); return;
            case cpu_register::sp: out << detail::aligned("sp", options); return;
            case cpu_register::lr: out << detail::aligned("lr", options); return;
            case cpu_register::pc: out << detail::aligned("pc", options); return;
            case cpu_register::spsr: out << detail::aligned("spsr", options); return;
            default: out << detail::aligned("Invalid", options); return;
        }
    }
    void kprint_value(ostream& out, const char*&, cpu::psr value) {
        auto cond = value.conditions();
        out << (cond.negative ? 'N' : '_');
        out << (cond.zero     ? 'Z' : '_');
        out << (cond.carry    ? 'C' : '_');
        out << (cond.overflow ? 'V' : '_');
        out << ' ';
        out << (value.endianness() == cpu::cpu_endianness::big ? 'B' : 'L');
        out << ' ';
        auto mask = value.interrupt_mask();
        out << (mask.irq ? 'I' : '_');
        out << (mask.fiq ? 'F' : '_');
        out << (value.thumb() ? 'T' : '_');
        kprint(out, "  {:10} {:#010x}", value.mode(), value.raw_value);
    }
}
