_ZN6micron4math3fft6__impl7__stageILm4ELb0EdEEvPNS0_7complexIT1_EEPKS6_mmm:
.LFB7646:
	.cfi_startproc
	movq	%rdi, %r10
	cmpq	%r8, %rcx
	jnb	.L1767
	salq	$4, %rcx
	movq	%rdx, %rdi
	leaq	(%rdx,%rdx,2), %r9
	salq	$4, %r8
	leaq	(%r10,%rcx), %rax
	salq	$5, %rdi
	addq	%rcx, %rsi
	salq	$4, %r9
	movapd	.LC72(%rip), %xmm6
	leaq	(%r10,%r8), %rcx
	salq	$4, %rdx
	movq	.LC25(%rip), %xmm5
	.p2align 4
	.p2align 3
.L1769:
	movupd	(%rsi), %xmm0
	movupd	(%rsi,%rdx), %xmm8
	addq	$16, %rax
	addq	$16, %rsi
	movupd	-16(%rsi,%rdi), %xmm11
	movupd	-16(%rax,%rdx), %xmm1
	movupd	-16(%rax,%rdx), %xmm4
	movupd	-16(%rax,%rdi), %xmm7
	movapd	%xmm0, %xmm2
	movapd	%xmm8, %xmm10
	movupd	-16(%rax,%rdi), %xmm9
	movupd	-16(%rax,%r9), %xmm13
	movapd	%xmm11, %xmm14
	unpckhpd	%xmm1, %xmm1
	movupd	-16(%rax,%r9), %xmm12
	shufpd	$1, %xmm0, %xmm2
	unpcklpd	%xmm4, %xmm4
	shufpd	$1, %xmm8, %xmm10
	unpckhpd	%xmm9, %xmm9
	unpckhpd	%xmm13, %xmm13
	unpcklpd	%xmm7, %xmm7
	shufpd	$1, %xmm11, %xmm14
	mulpd	%xmm2, %xmm1
	unpcklpd	%xmm12, %xmm12
	movupd	-16(%rax), %xmm3
	mulpd	%xmm10, %xmm9
	mulpd	%xmm14, %xmm13
	movapd	%xmm3, %xmm15
	mulpd	%xmm4, %xmm0
	mulpd	%xmm7, %xmm8
	mulpd	%xmm12, %xmm11
	xorpd	%xmm5, %xmm1
	xorpd	%xmm5, %xmm9
	xorpd	%xmm5, %xmm13
	addpd	%xmm1, %xmm0
	addpd	%xmm8, %xmm9
	addpd	%xmm13, %xmm11
	movapd	%xmm0, %xmm4
	addpd	%xmm9, %xmm15
	subpd	%xmm9, %xmm3
	subpd	%xmm11, %xmm0
	addpd	%xmm11, %xmm4
	movapd	%xmm15, %xmm1
	movapd	%xmm3, %xmm2
	shufpd	$1, %xmm0, %xmm0
	xorpd	%xmm6, %xmm0
	addpd	%xmm4, %xmm1
	subpd	%xmm4, %xmm15
	addpd	%xmm0, %xmm2
	subpd	%xmm0, %xmm3
	movups	%xmm1, -16(%rax)
	movups	%xmm15, -16(%rax,%rdi)
	movups	%xmm2, -16(%rax,%rdx)
	movups	%xmm3, -16(%rax,%r9)
	cmpq	%rax, %rcx
	jne	.L1769
.L1767:
	ret
	.cfi_endproc
.LFE7646:
	.size	_ZN6micron4math3fft6__impl7__stageILm4ELb0EdEEvPNS0_7complexIT1_EEPKS6_mmm, .-_ZN6micron4math3fft6__impl7__stageILm4ELb0EdEEvPNS0_7complexIT1_EEPKS6_mmm
