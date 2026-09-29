	.file	1 "/work/f4_tablecopy.i"

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
	.globl	func_8005E938

	.text
	.ent	func_8005E938
func_8005E938:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, extra= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	sll	$5,$5,1
	lhu	$2,D_8009F8EC($5)
	.set	noreorder
	.set	nomacro
	j	$31
	sh	$2,0($4)
	.set	macro
	.set	reorder

	.end	func_8005E938
