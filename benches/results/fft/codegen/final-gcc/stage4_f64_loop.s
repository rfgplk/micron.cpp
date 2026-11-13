.L1778:
	vmovdqu	(%rdx), %ymm4
	vpermilpd	$15, (%rax,%rsi), %ymm2
	addq	$2, %rcx
	addq	$32, %rdx
	vmovdqu	-32(%rdx,%rdi), %ymm12
	vmovddup	(%rax,%rsi), %ymm0
	vpermilpd	$5, %ymm4, %ymm5
	vpermilpd	$15, (%rax,%r9), %ymm14
	vmovdqu	-32(%rdx,%rsi), %ymm7
	vmulpd	%ymm5, %ymm2, %ymm6
	vpermilpd	$5, %ymm12, %ymm15
	vmovddup	(%rax,%rdi), %ymm8
	vpermilpd	$15, (%rax,%rdi), %ymm9
	vpermilpd	$5, %ymm7, %ymm10
	vmovddup	(%rax,%r9), %ymm13
	vmovdqu	(%rax), %ymm1
	addq	$32, %rax
	vmulpd	%ymm10, %ymm9, %ymm11
	vfmaddsub132pd	%ymm4, %ymm6, %ymm0
	vmulpd	%ymm15, %ymm14, %ymm4
	vfmaddsub132pd	%ymm7, %ymm11, %ymm8
	vfmaddsub132pd	%ymm12, %ymm4, %ymm13
	vaddpd	%ymm1, %ymm8, %ymm2
	vsubpd	%ymm8, %ymm1, %ymm1
	vaddpd	%ymm13, %ymm0, %ymm5
	vsubpd	%ymm13, %ymm0, %ymm0
	vpermilpd	$5, %ymm0, %ymm6
	vaddpd	%ymm5, %ymm2, %ymm8
	vsubpd	%ymm5, %ymm2, %ymm9
	vxorpd	%ymm3, %ymm6, %ymm7
	vaddpd	%ymm7, %ymm1, %ymm10
	vsubpd	%ymm7, %ymm1, %ymm11
	vmovdqu	%ymm8, -32(%rax)
	vmovdqu	%ymm9, -32(%rax,%rdi)
	vmovdqu	%ymm10, -32(%rax,%rsi)
	vmovdqu	%ymm11, -32(%rax,%r9)
	cmpq	%r8, %rcx
	jb	.L1778
