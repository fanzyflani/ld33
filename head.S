	/*.org 0x10800*/
	.section ".boot"

	.ascii "PS-X EXE"
	.word 0, 0
	.word load_base, 0
	.word aaa_very_start, _LOADABLE_SIZE
	.word 0, 0
	.word 0, 0
	.word 0x801FF000, 0
	/*.word 0x1F8003FC, 0*/
	.word 0, 0

	.word 0, 0
	.word 0
	/*.ascii "Sony Computer Entertainment Inc. for North America area"*/
	.ascii "Sony Computer Entertainment Inc. for Europe area"
	.align 11
load_base:

aaa_very_start:
	/* Load stack pointer */
	/*lui $sp, 0x801F
	ori $sp, $sp, 0xFF00*/

	/* Load global pointer */
	lui $gp, %hi(_gp)
	ori $gp, $gp, %lo(_gp)

	/* Jump to start */
	jal aaa_start
	nop
	j aaa_very_start
	nop

	.global int_handler_stub_start
	.global int_handler_stub_end
int_handler_stub_start:
	lui $2, %hi(int_handler)
	ori $2, $0, %lo(int_handler)
	jr $2
	nop
int_handler_stub_end:

int_handler:
	.set noat
	addiu $sp, $sp, -17*4
	sw $1, 4*1($sp)
	.set at
	mfc0 $k0, $14
	sw $k0, 4*16($sp)
	sw $ra, 4*0($sp)
	sw $2, 4*2($sp)
	sw $3, 4*3($sp)
	sw $4, 4*4($sp)
	sw $5, 4*5($sp)
	sw $6, 4*6($sp)
	sw $7, 4*7($sp)
	sw $8, 4*8($sp)
	sw $9, 4*9($sp)
	sw $10, 4*10($sp)
	sw $11, 4*11($sp)
	sw $12, 4*12($sp)
	sw $13, 4*13($sp)
	sw $14, 4*14($sp)
	sw $15, 4*15($sp)

	mfc0 $a0, $13
	mfc0 $a1, $12
	move $a2, $k0
	jal isr_handler

	lw $2, 4*2($sp)
	lw $3, 4*3($sp)
	lw $4, 4*4($sp)
	lw $5, 4*5($sp)
	lw $6, 4*6($sp)
	lw $7, 4*7($sp)
	lw $8, 4*8($sp)
	lw $9, 4*9($sp)
	lw $10, 4*10($sp)
	lw $11, 4*11($sp)
	lw $12, 4*12($sp)
	lw $13, 4*13($sp)
	lw $14, 4*14($sp)
	lw $15, 4*15($sp)
	.set noat
	lw $1, 4*1($sp)
	lw $ra, 4*0($sp)
	lw $k0, 4*16($sp)
	addiu $sp, $sp, 17*4
	jr $k0
	rfe
	.set at

	.section ".rodata"
	.align 5
	.global fsys_rawcga
fsys_rawcga:
	.incbin "dat/rawcga.bin"
	.align 5

	.global fsys_piresy
fsys_piresy:
	.incbin "dat/piresy.bin"
	.align 5

	.global fsys_skydome
fsys_skydome:
	.incbin "dat/skydome.bin"
	.align 5

	.global fsys_s3m_test
fsys_s3m_test:
	.incbin "dat/33-t1.s3m"
	.align 5

