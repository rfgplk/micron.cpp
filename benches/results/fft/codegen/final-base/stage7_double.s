_ZN6micron4math3fft6__impl7__stageILm7ELb0EdEEvPNS0_7complexIT1_EEPKS6_mmm:
.LFB7648:
	.cfi_startproc
	cmpq	%r8, %rcx
	jnb	.L1782
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	leaq	(%rdx,%rdx,2), %r9
	leaq	(%rdx,%rdx,4), %r10
	movq	%rdx, %r11
	salq	$4, %rcx
	salq	$4, %r10
	movapd	.LC77(%rip), %xmm14
	movapd	.LC78(%rip), %xmm13
	movapd	.LC79(%rip), %xmm12
	salq	$6, %r11
	movapd	.LC81(%rip), %xmm11
	movq	.LC25(%rip), %xmm8
	movapd	.LC72(%rip), %xmm10
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	pushq	%r13
	.cfi_offset 13, -24
	movq	%r8, %r13
	pushq	%r12
	movq	%rdx, %r8
	.cfi_offset 12, -32
	movq	%rdi, %r12
	salq	$4, %r13
	movq	%rdx, %rdi
	pushq	%rbx
	.cfi_offset 3, -40
	movq	%r9, %rbx
	leaq	(%rsi,%rcx), %rdx
	salq	$5, %rbx
	salq	$5, %r8
	leaq	(%r12,%rcx), %rax
	leaq	(%r12,%r13), %rsi
	salq	$4, %r9
	salq	$4, %rdi
	.p2align 4
	.p2align 3
.L1779:
	movupd	(%rdx), %xmm0
	movupd	(%rax,%rdi), %xmm1
	addq	$16, %rax
	addq	$16, %rdx
	movupd	-16(%rax,%rdi), %xmm3
	movupd	-16(%rdx,%rdi), %xmm4
	movapd	%xmm0, %xmm2
	unpckhpd	%xmm1, %xmm1
	movupd	-16(%rax,%r8), %xmm6
	movupd	-16(%rax,%r8), %xmm5
	shufpd	$1, %xmm0, %xmm2
	unpcklpd	%xmm3, %xmm3
	movapd	%xmm4, %xmm9
	movupd	-16(%rax,%r9), %xmm15
	mulpd	%xmm2, %xmm1
	unpckhpd	%xmm6, %xmm6
	unpcklpd	%xmm5, %xmm5
	shufpd	$1, %xmm4, %xmm9
	mulpd	%xmm3, %xmm0
	movupd	-16(%rdx,%r8), %xmm3
	unpcklpd	%xmm15, %xmm15
	movupd	-16(%rax), %xmm7
	mulpd	%xmm9, %xmm6
	movupd	-16(%rax,%r11), %xmm9
	mulpd	%xmm5, %xmm4
	movapd	%xmm3, %xmm2
	shufpd	$1, %xmm3, %xmm2
	mulpd	%xmm15, %xmm3
	unpckhpd	%xmm9, %xmm9
	movupd	-16(%rax,%r10), %xmm15
	xorpd	%xmm8, %xmm1
	addpd	%xmm1, %xmm0
	movupd	-16(%rax,%r9), %xmm1
	unpcklpd	%xmm15, %xmm15
	xorpd	%xmm8, %xmm6
	unpckhpd	%xmm1, %xmm1
	addpd	%xmm6, %xmm4
	movupd	-16(%rax,%r11), %xmm6
	mulpd	%xmm2, %xmm1
	movupd	-16(%rdx,%r9), %xmm2
	unpcklpd	%xmm6, %xmm6
	movapd	%xmm2, %xmm5
	shufpd	$1, %xmm2, %xmm5
	mulpd	%xmm5, %xmm9
	movupd	-16(%rdx,%r11), %xmm5
	mulpd	%xmm6, %xmm2
	xorpd	%xmm8, %xmm1
	movapd	%xmm5, %xmm6
	addpd	%xmm1, %xmm3
	movupd	-16(%rax,%r10), %xmm1
	shufpd	$1, %xmm5, %xmm6
	mulpd	%xmm15, %xmm5
	unpckhpd	%xmm1, %xmm1
	xorpd	%xmm8, %xmm9
	mulpd	%xmm6, %xmm1
	movupd	-16(%rax,%rbx), %xmm6
	addpd	%xmm9, %xmm2
	movupd	-16(%rax,%rbx), %xmm9
	unpckhpd	%xmm6, %xmm6
	unpcklpd	%xmm9, %xmm9
	xorpd	%xmm8, %xmm1
	addpd	%xmm1, %xmm5
	movupd	-16(%rdx,%r10), %xmm1
	movapd	%xmm1, %xmm15
	shufpd	$1, %xmm1, %xmm15
	mulpd	%xmm15, %xmm6
	mulpd	%xmm9, %xmm1
	xorpd	%xmm8, %xmm6
	addpd	%xmm1, %xmm6
	movapd	%xmm0, %xmm1
	addpd	%xmm6, %xmm1
	subpd	%xmm6, %xmm0
	movapd	%xmm4, %xmm6
	addpd	%xmm5, %xmm6
	subpd	%xmm5, %xmm4
	movapd	%xmm3, %xmm5
	addpd	%xmm2, %xmm5
	subpd	%xmm2, %xmm3
	movapd	%xmm7, %xmm2
	addpd	%xmm1, %xmm2
	movapd	%xmm1, %xmm9
	mulpd	%xmm14, %xmm9
	movapd	%xmm6, %xmm15
	mulpd	%xmm13, %xmm15
	addpd	%xmm6, %xmm2
	addpd	%xmm5, %xmm2
	addpd	%xmm7, %xmm9
	movups	%xmm2, -16(%rax)
	movapd	%xmm5, %xmm2
	addpd	%xmm15, %xmm9
	movapd	%xmm4, %xmm15
	mulpd	%xmm12, %xmm2
	mulpd	%xmm11, %xmm15
	addpd	%xmm2, %xmm9
	movapd	.LC80(%rip), %xmm2
	mulpd	%xmm0, %xmm2
	addpd	%xmm15, %xmm2
	movapd	.LC82(%rip), %xmm15
	mulpd	%xmm3, %xmm15
	addpd	%xmm15, %xmm2
	movapd	%xmm9, %xmm15
	shufpd	$1, %xmm2, %xmm2
	xorpd	%xmm10, %xmm2
	addpd	%xmm2, %xmm15
	subpd	%xmm2, %xmm9
	movapd	%xmm6, %xmm2
	mulpd	%xmm12, %xmm2
	mulpd	%xmm14, %xmm6
	movups	%xmm15, -16(%rax,%rdi)
	movapd	%xmm5, %xmm15
	movups	%xmm9, -16(%rax,%rbx)
	mulpd	%xmm14, %xmm15
	movapd	%xmm1, %xmm9
	mulpd	%xmm13, %xmm9
	mulpd	%xmm12, %xmm1
	mulpd	%xmm13, %xmm5
	addpd	%xmm7, %xmm9
	addpd	%xmm7, %xmm1
	addpd	%xmm2, %xmm9
	movapd	%xmm0, %xmm2
	mulpd	%xmm11, %xmm2
	addpd	%xmm6, %xmm1
	mulpd	.LC82(%rip), %xmm0
	addpd	%xmm15, %xmm9
	movapd	.LC83(%rip), %xmm15
	addpd	%xmm5, %xmm1
	mulpd	%xmm4, %xmm15
	mulpd	.LC84(%rip), %xmm4
	movapd	%xmm1, %xmm7
	addpd	%xmm15, %xmm2
	movapd	.LC84(%rip), %xmm15
	addpd	%xmm4, %xmm0
	mulpd	%xmm3, %xmm15
	mulpd	%xmm11, %xmm3
	addpd	%xmm15, %xmm2
	movapd	%xmm9, %xmm15
	addpd	%xmm3, %xmm0
	shufpd	$1, %xmm2, %xmm2
	xorpd	%xmm10, %xmm2
	shufpd	$1, %xmm0, %xmm0
	xorpd	%xmm10, %xmm0
	addpd	%xmm2, %xmm15
	subpd	%xmm2, %xmm9
	addpd	%xmm0, %xmm7
	subpd	%xmm0, %xmm1
	movups	%xmm15, -16(%rax,%r8)
	movups	%xmm9, -16(%rax,%r10)
	movups	%xmm7, -16(%rax,%r9)
	movups	%xmm1, -16(%rax,%r11)
	cmpq	%rax, %rsi
	jne	.L1779
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
.L1782:
	.cfi_restore 3
	.cfi_restore 6
	.cfi_restore 12
	.cfi_restore 13
	ret
	.cfi_endproc
.LFE7648:
	.size	_ZN6micron4math3fft6__impl7__stageILm7ELb0EdEEvPNS0_7complexIT1_EEPKS6_mmm, .-_ZN6micron4math3fft6__impl7__stageILm7ELb0EdEEvPNS0_7complexIT1_EEPKS6_mmm
