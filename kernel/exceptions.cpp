#include <kernel/basic.hpp>

#include <arch/arm/cpu.hpp>
#include <arch/arm/interrupts.hpp>
#include <drivers/serial.hpp>
#include <kernel/coroutine.hpp>
#include <kernel/debug.hpp>
#include <lib/bitfield.hpp>

#include <cstdint>

namespace kernel {

using namespace cpu;
using namespace cpu::interrupts;

const char* decode_ifsr(uint32_t ifsr) {
    uint32_t status = extract_bits(ifsr, {0, 1, 2, 3, 10});
    switch(status) {
        case 0b00000: return "No function, reset value";
        case 0b00001: return "No function";
        case 0b00010: return "Debug event fault";
        case 0b00011: return "Access Flag fault on Section";
        case 0b00100: return "No function";
        case 0b00101: return "Translation fault on Section";
        case 0b00110: return "Access Flag fault on Page";
        case 0b00111: return "Translation fault on Page";
        case 0b01000: return "Precise External Abort";
        case 0b01001: return "Domain fault on Section";
        case 0b01010: return "No function";
        case 0b01011: return "Domain fault on Page";
        case 0b01100: return "External abort on Section";
        case 0b01101: return "Permission fault on Section";
        case 0b01110: return "External abort on Page";
        case 0b01111: return "Permission fault on Page";
        default: return "No function";
    }
}
const char* decode_dfsr(uint32_t dfsr) {
    uint32_t status = extract_bits(dfsr, {0, 1, 2, 3, 10});
    switch(status) {
        case 0b00000: return "No function, reset value";
        case 0b00001: return "Alignment fault";
        case 0b00010: return "Debug event fault";
        case 0b00011: return "Access Flag fault on Section";
        case 0b00100: return "Cache maintenance operation fault";
        case 0b00101: return "Translation fault on Section";
        case 0b00110: return "Access Flag fault on Page";
        case 0b00111: return "Translation fault on Page";
        case 0b01000: return "Precise External Abort";
        case 0b01001: return "Domain fault on Section";
        case 0b01010: return "No function";
        case 0b01011: return "Domain fault on Page";
        case 0b01100: return "External abort on Section";
        case 0b01101: return "Permission fault on Section";
        case 0b01110: return "External abort on Page";
        case 0b01111: return "Permission fault on Page";
        case 0b10110: return "Imprecise External Abort";
        case 0b10111: return "No function";
        default: return "No function";
    }
}

interrupt_result handle_exception(interrupt_context& context, void*) {
    auto& [type, registers, address, result] = context;
    debug::kerror("############ EXCEPTION ############");
    debug::kerror("{} an Adresse: {:#010x} in", type, address/*, get_coroutine_info(current_coroutine())*/);
    switch(type) {
        case interrupt_type::data_abort: {
            uint32_t dfsr, dfar;
            __asm__("mrc p15, 0, %0, c5, c0, 0" : "=r"(dfsr));
            __asm__("mrc p15, 0, %0, c6, c0, 0" : "=r"(dfar));
            debug::kerror("Data Fault Status Register: {:#010x} -> {}", dfsr, decode_dfsr(dfsr));
            debug::kerror("Data Fault Adress Register: {:#010x}", dfar);
        } break;
        case interrupt_type::prefetch_abort: {
            uint32_t ifsr, ifar;
            __asm__("mrc p15, 0, %0, c5, c0, 1" : "=r"(ifsr));
            __asm__("mrc p15, 0, %0, c6, c0, 2" : "=r"(ifar));
            debug::kerror("Instruction Fault Status Register: {:#010x} -> {}", ifsr, decode_ifsr(ifsr));
            debug::kerror("Instruction Fault Adress Register: {:#010x}", ifar);
        } break;
        default: ;
    }
    debug::kerror("");
    debug::kerror(">> Registerschnappschuss <<");
    debug::kerror("R0: {:#010x}  R5: {:#010x}  R10: {:#010x}", registers.r[0], registers.r[5], registers.r[10]);
    debug::kerror("R1: {:#010x}  R6: {:#010x}  R11: {:#010x}", registers.r[1], registers.r[6], registers.r[11]);
    debug::kerror("R2: {:#010x}  R7: {:#010x}  R12: {:#010x}", registers.r[2], registers.r[7], registers.r[12]);
    debug::kerror("R3: {:#010x}  R8: {:#010x}", registers.r[3], registers.r[8]);
    debug::kerror("R4: {:#010x}  R9: {:#010x}", registers.r[4], registers.r[9]);

    {
        using namespace cpu;

        auto print = [](cpu_mode mode) {
            debug::kerror("{:<11} | LR: {:#010x} | SP: {:#010x} | SPSR: {}", mode,
                read_register(mode, cpu_register::lr),
                read_register(mode, cpu_register::sp),
                cpu::psr(read_register(mode, cpu_register::spsr))
            );
        };

        debug::kerror("");
        debug::kerror(">> Modusspezifische Register <<");
        debug::kerror("User/System | LR: {:#010x} | SP: {:#010x} | CPSR: {}",
            read_register(cpu_mode::usr, cpu_register::lr),
            read_register(cpu_mode::usr, cpu_register::lr),
            psr::current());
        print(cpu_mode::irq);
        print(cpu_mode::abt);
        print(cpu_mode::und);
        print(cpu_mode::svc);
    }

    debug::kerror("");
    debug::kerror("Press 'n' for next instruction, 'r' to repeat the instruction, or 'e' to jump into the event loop.");
    do {
        char c = driver::serial::Serial.get();
        switch(c) {
            case 'n': return interrupt_result::next;
            case 'r': return interrupt_result::repeat;
            case 'e': return interrupt_result::event_loop;
        }
    }
    while(true);
}

}
