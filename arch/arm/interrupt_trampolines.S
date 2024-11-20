.section .text

.global handle_interrupt

#define __hash #
#define TRAMPOLINE(name, type) .global name; \
    name: \
        stmfd sp!, {r0-r12, lr}; \
        mov r0, __hash(type); \
        mov r1, sp; /* Pointer to the interrupt_registers structure set up by stmfd */ \
        bl handle_interrupt; \
        ldmfd sp!, {r0-r12, pc}^;

TRAMPOLINE(undefined_instruction, 0)
TRAMPOLINE(software_interrupt, 1)
TRAMPOLINE(prefetch_abort, 2)
TRAMPOLINE(data_abort, 3)
TRAMPOLINE(not_used, 4)
TRAMPOLINE(irq, 5)
TRAMPOLINE(fiq, 6)
