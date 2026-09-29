	.file	1 "probe.c"
gcc2_compiled.:
__gnu_compiled_c:
	.text
	.align	2
	.globl	f
	.text
	.ent	f
f:
	.frame	$fp,8,$31		# vars= 0, regs= 1/0, args= 0, extra= 0
	.mask	0x40000000,-8
	.fmask	0x00000000,0
	subu	$sp,$sp,8
	sw	$fp,0($sp)
	move	$fp,$sp
	sw	$4,8($fp)
	lw	$2,8($fp)
	addu	$3,$2,1
	move	$2,$3
	j	$L1
$L1:
	move	$sp,$fp
	lw	$fp,0($sp)
	addu	$sp,$sp,8
	j	$31
	.end	f
