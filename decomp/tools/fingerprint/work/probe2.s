	.file	1 "probe.c"
gcc2_compiled.:
__gnu_compiled_c:
	.text
	.align	2
	.globl	f
	.text
	.ent	f
f:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, extra= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	j	$31
	addu	$2,$4,1
	.set	macro
	.set	reorder

	.end	f
