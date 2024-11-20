
.section .init

.global _end_of_kernel

.global _start
_start:
	mrs r0, cpsr
	and r0, r0, #0x1F
	mov r1, #0x1A
	cmp r0, r1

	beq _noHypervisor
_disableOtherCores:
	mrc p15, 0, r0, c0, c0, 5
	tst r0, #3
	bne _disableCore

_enableEnforceAlignment:
	mrc p15, 0, r0, c1, c0, 0
	orr r0, r0, #0x2
	mcr p15, 0, r0, c1, c0, 0


_main:
	cps 0x1f
	ldr sp,=_end_of_kernel+0x200000
	bl  start_kernel
.end:
	WFI
	b .end

_disableCore:
	cpsid if
	b .end

_noHypervisor:
	ldr lr, =_disableOtherCores
	msr ELR_hyp, lr

	mrs r0, cpsr
	bic r0, r0, #0x1F
	orr r0, r0, #0x13
	msr spsr_hyp, r0
	eret
