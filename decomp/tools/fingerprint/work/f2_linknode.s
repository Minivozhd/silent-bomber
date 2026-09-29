	.file	1 "/work/f2_linknode.i"

 # GNU C 2.6.3 [AL 1.1, MM 40] Sony Playstation compiled by GNU C

 # Cc1 defaults:
 # -mgas -msoft-float

 # Cc1 arguments (-G value = 8, Cpu = 3000, ISA = 1):
 # -O1 -G8 -mips1 -w -funsigned-char -fpeephole -ffunction-cse
 # -fpcc-struct-return -fcommon -msoft-float -mgas -fgnu-linker -quiet -o

gcc2_compiled.:
__gnu_compiled_c:
	.text
	.align	2
	.globl	func_80034C90

	.text
	.ent	func_80034C90
func_80034C90:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, extra= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	li	$2,0x00000001		# 1
	sh	$2,4($5)
	lw	$2,12($4)
	#nop
	sw	$2,0($5)
	sw	$6,12($5)
	sw	$7,8($5)
	.set	noreorder
	.set	nomacro
	j	$31
	sw	$5,12($4)
	.set	macro
	.set	reorder

	.end	func_80034C90
