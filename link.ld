OUTPUT_FORMAT("elf32-littlemips");
OUTPUT_ARCH(mips);

SECTIONS
{
	. = 0x80010800;
	_LOADABLE_START_WHEAD = .;
	.text : {
		*(.boot)
		*(.text.unlikely* .text.*_unlikely)
		*(.text.exit*)
		*(.text.startup*)
		*(.text.hot*)
		*(.text*)
		*(.stub)
	}
	
	. = ALIGN(8);
	.rodata : {
		*(.rodata.cst8)
		*(.rodata.cst4)
		*(.rodata)
		*(.rodata.str1.4)
	}
	.reginfo : { *(.reginfo) }
	.pdr : { *(.pdr) }
	.gnu.attributes : { *(.gnu.attributes) }

	.data : {
		*(.data*)
	}
	.eh_frame : { *(.eh_frame) }
	.mdebug.abi32 : { KEEP(*(.mdebug.abi32)) }
	.smain : ALIGN(1<<11) {
		_gp = ALIGN(16) + 0x7ff0;
		*(.scommon*)
		*(.sdata*)
		. = (. + 0x7FF) & ~ 0x7FF;
	} = 0x00000000
	_LOADABLE_END = .;
	_LOADABLE_SIZE = _LOADABLE_END - (_LOADABLE_START_WHEAD + 0x800);
	_BSS_START = .;
	.bss : {
		*(.sbss*)
		*(.bss)
	}
	. = (. + 0x7FF) & ~ 0x7FF;
	_BSS_END = .;
	_end = .; PROVIDE (end = .);
	/* Include debug crap if necessary */
	.debug 0 : { *(.debug) }
	.debug_info 0 : { *(.debug_info) }
	.debug_abbrev 0 : { *(.debug_abbrev) }
	.debug_loc 0 : { *(.debug_loc) }
	.debug_aranges 0 : { *(.debug_aranges) }
	.debug_ranges 0 : { *(.debug_ranges) }
	.debug_line 0 : { *(.debug_line) }
	.debug_str 0 : { *(.debug_str) }
	.debug_frame 0 : { *(.debug_frame) }
	/DISCARD/ : {
		*(.comment)
	}
}

