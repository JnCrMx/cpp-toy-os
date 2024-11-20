ENTRY(_start)
SECTIONS
{
	. = 0x00008000;
	.init : { *(.init) }
	.text : { *(.text) *(.text.*) }
	.rodata : { *(.rodata) *(.rodata.*) }
	.data : { *(.data) *(.data.*) }
	.bss : { *(.bss) *(.bss.*) }

	. = ALIGN(16);
	PROVIDE(_end_of_kernel = .);
}
