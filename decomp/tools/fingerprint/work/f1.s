	.file	1 "f1.i"
gcc2_compiled.:
__gnu_compiled_c:
	.text
	.align	2
	.globl	func_80010364
	.text
	.ent	func_80010364
func_80010364:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, extra= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	lhu	$2,D_800B5B20
	#nop
	addu	$2,$2,1
	sh	$2,D_800B5B20
	j	$31
	.end	func_80010364

	.comm	D_800B5B20,2
