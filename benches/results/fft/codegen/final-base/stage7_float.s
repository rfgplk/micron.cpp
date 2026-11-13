_ZN6micron4math3fft6__impl7__stageILm7ELb0EfEEvPNS0_7complexIT1_EEPKS6_mmm:
.LFB7643:
	.cfi_startproc
	cmpq	%r8, %rcx
	jnb	.L1751
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rdi, %r10
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	pushq	%r13
	.cfi_offset 13, -24
	movq	%r8, %r13
	pushq	%r12
	subq	%rcx, %r13
	pushq	%rbx
	andl	$1, %r13d
	.cfi_offset 12, -32
	.cfi_offset 3, -40
	movq	%rsi, %rbx
	movq	%rdx, %rsi
	subq	%r13, %r8
	movq	%r8, %r12
	cmpq	%r8, %rcx
	jnb	.L1746
	movq	%rdx, %r8
	leaq	(%rdx,%rdx,4), %r9
	leaq	(%rdx,%rdx,2), %rdi
	movq	%rdx, %r11
	leaq	(%rcx,%rsi), %rax
	movaps	.LC26(%rip), %xmm8
	movaps	.LC46(%rip), %xmm14
	salq	$4, %r8
	movaps	.LC47(%rip), %xmm13
	salq	$3, %r9
	salq	$3, %rdi
	movaps	.LC48(%rip), %xmm12
	movaps	.LC50(%rip), %xmm11
	salq	$5, %r11
	leaq	(%rbx,%rcx,8), %rdx
	leaq	(%r10,%rax,8), %rax
	movaps	.LC29(%rip), %xmm10
	.p2align 4
	.p2align 3
.L1747:
	movups	(%rax), %xmm1
	movups	(%rdx), %xmm3
	movups	(%rax,%rsi,8), %xmm5
	movups	(%rdx,%rsi,8), %xmm4
	addq	$16, %rdx
	movaps	%xmm1, %xmm2
	movaps	%xmm3, %xmm0
	movups	-16(%rdx,%r8), %xmm15
	movups	(%r10,%rcx,8), %xmm7
	shufps	$177, %xmm3, %xmm0
	shufps	$245, %xmm1, %xmm2
	mulps	%xmm0, %xmm2
	movaps	%xmm1, %xmm0
	shufps	$160, %xmm1, %xmm0
	mulps	%xmm3, %xmm0
	movaps	%xmm5, %xmm6
	movaps	%xmm4, %xmm9
	movups	(%rax,%r8), %xmm1
	shufps	$177, %xmm4, %xmm9
	movaps	%xmm5, %xmm3
	shufps	$245, %xmm5, %xmm6
	mulps	%xmm9, %xmm6
	shufps	$160, %xmm5, %xmm3
	movups	(%rax,%rdi), %xmm9
	mulps	%xmm4, %xmm3
	movaps	%xmm1, %xmm4
	xorps	%xmm8, %xmm2
	shufps	$245, %xmm1, %xmm4
	movaps	%xmm9, %xmm5
	addps	%xmm2, %xmm0
	movaps	%xmm15, %xmm2
	shufps	$245, %xmm9, %xmm5
	shufps	$160, %xmm9, %xmm9
	shufps	$177, %xmm15, %xmm2
	mulps	%xmm2, %xmm4
	movaps	%xmm1, %xmm2
	shufps	$160, %xmm1, %xmm2
	mulps	%xmm15, %xmm2
	xorps	%xmm8, %xmm6
	movups	(%rax,%r11), %xmm1
	addps	%xmm6, %xmm3
	movups	-16(%rdx,%rdi), %xmm6
	movaps	%xmm6, %xmm15
	xorps	%xmm8, %xmm4
	shufps	$177, %xmm6, %xmm15
	addps	%xmm4, %xmm2
	mulps	%xmm15, %xmm5
	movaps	%xmm9, %xmm4
	movups	-16(%rdx,%r11), %xmm9
	mulps	%xmm6, %xmm4
	movaps	%xmm1, %xmm6
	shufps	$245, %xmm1, %xmm6
	xorps	%xmm8, %xmm5
	addps	%xmm5, %xmm4
	movaps	%xmm9, %xmm5
	shufps	$177, %xmm9, %xmm5
	mulps	%xmm5, %xmm6
	movaps	%xmm1, %xmm5
	shufps	$160, %xmm1, %xmm5
	mulps	%xmm9, %xmm5
	movups	-16(%rdx,%r9), %xmm9
	movaps	%xmm9, %xmm15
	shufps	$177, %xmm9, %xmm15
	xorps	%xmm8, %xmm6
	addps	%xmm6, %xmm5
	movups	(%rax,%r9), %xmm6
	movaps	%xmm6, %xmm1
	shufps	$245, %xmm6, %xmm1
	mulps	%xmm15, %xmm1
	shufps	$160, %xmm6, %xmm6
	mulps	%xmm9, %xmm6
	xorps	%xmm8, %xmm1
	addps	%xmm1, %xmm6
	movaps	%xmm0, %xmm1
	addps	%xmm6, %xmm1
	subps	%xmm6, %xmm0
	movaps	%xmm3, %xmm6
	addps	%xmm5, %xmm6
	subps	%xmm5, %xmm3
	movaps	%xmm2, %xmm5
	addps	%xmm4, %xmm5
	subps	%xmm4, %xmm2
	movaps	%xmm7, %xmm4
	addps	%xmm1, %xmm4
	movaps	%xmm1, %xmm9
	mulps	%xmm14, %xmm9
	movaps	%xmm6, %xmm15
	mulps	%xmm13, %xmm15
	addps	%xmm6, %xmm4
	addps	%xmm5, %xmm4
	addps	%xmm7, %xmm9
	movups	%xmm4, (%r10,%rcx,8)
	movaps	%xmm5, %xmm4
	addps	%xmm15, %xmm9
	movaps	%xmm3, %xmm15
	mulps	%xmm12, %xmm4
	addq	$2, %rcx
	mulps	%xmm11, %xmm15
	addps	%xmm4, %xmm9
	movaps	.LC49(%rip), %xmm4
	mulps	%xmm0, %xmm4
	addps	%xmm15, %xmm4
	movaps	.LC51(%rip), %xmm15
	mulps	%xmm2, %xmm15
	addps	%xmm15, %xmm4
	movaps	%xmm9, %xmm15
	shufps	$177, %xmm4, %xmm4
	xorps	%xmm10, %xmm4
	addps	%xmm4, %xmm15
	subps	%xmm4, %xmm9
	movaps	%xmm6, %xmm4
	mulps	%xmm12, %xmm4
	mulps	%xmm14, %xmm6
	movups	%xmm15, (%rax)
	movaps	%xmm5, %xmm15
	movups	%xmm9, (%rax,%r9)
	mulps	%xmm14, %xmm15
	movaps	%xmm1, %xmm9
	mulps	%xmm13, %xmm9
	mulps	%xmm12, %xmm1
	mulps	%xmm13, %xmm5
	addps	%xmm7, %xmm9
	addps	%xmm7, %xmm1
	addps	%xmm4, %xmm9
	movaps	%xmm0, %xmm4
	mulps	%xmm11, %xmm4
	addps	%xmm6, %xmm1
	mulps	.LC51(%rip), %xmm0
	addps	%xmm15, %xmm9
	movaps	.LC52(%rip), %xmm15
	addps	%xmm5, %xmm1
	mulps	%xmm3, %xmm15
	mulps	.LC53(%rip), %xmm3
	movaps	%xmm1, %xmm7
	addps	%xmm15, %xmm4
	movaps	.LC53(%rip), %xmm15
	addps	%xmm3, %xmm0
	mulps	%xmm2, %xmm15
	mulps	%xmm11, %xmm2
	addps	%xmm15, %xmm4
	movaps	%xmm9, %xmm15
	addps	%xmm2, %xmm0
	shufps	$177, %xmm4, %xmm4
	xorps	%xmm10, %xmm4
	shufps	$177, %xmm0, %xmm0
	xorps	%xmm10, %xmm0
	addps	%xmm4, %xmm15
	subps	%xmm4, %xmm9
	addps	%xmm0, %xmm7
	subps	%xmm0, %xmm1
	movups	%xmm15, (%rax,%rsi,8)
	addq	$16, %rax
	movups	%xmm9, -16(%rax,%r11)
	movups	%xmm7, -16(%rax,%r8)
	movups	%xmm1, -16(%rax,%rdi)
	cmpq	%r12, %rcx
	jb	.L1747
.L1746:
	testq	%r13, %r13
	je	.L1744
	movq	(%rbx,%r12,8), %xmm8
	leaq	(%r12,%rsi), %r8
	leaq	(%r10,%r12,8), %r13
	leaq	(%r10,%r8,8), %rcx
	movq	(%rbx,%r8,8), %xmm2
	addq	%rsi, %r8
	movq	(%rcx), %xmm13
	movaps	%xmm8, %xmm14
	movq	%xmm8, %xmm6
	movq	(%rbx,%r8,8), %xmm8
	shufps	$0xe1, %xmm14, %xmm14
	movq	(%r10,%r8,8), %xmm5
	addq	%rsi, %r8
	movaps	%xmm2, %xmm1
	movq	%xmm14, %xmm10
	movaps	%xmm13, %xmm12
	shufps	$0xe5, %xmm13, %xmm13
	movaps	%xmm8, %xmm14
	movaps	%xmm10, %xmm3
	shufps	$0xe0, %xmm12, %xmm12
	movq	%xmm12, %xmm11
	movq	%xmm13, %xmm0
	mulps	%xmm11, %xmm3
	movq	(%r10,%r8,8), %xmm13
	shufps	$0xe1, %xmm1, %xmm1
	movaps	%xmm5, %xmm9
	mulps	%xmm6, %xmm11
	shufps	$0xe1, %xmm14, %xmm14
	movq	%xmm1, %xmm15
	shufps	$0xe0, %xmm9, %xmm9
	mulps	%xmm0, %xmm6
	movaps	%xmm13, %xmm12
	movq	%xmm2, %xmm7
	shufps	$0xe5, %xmm5, %xmm5
	mulps	%xmm10, %xmm0
	movq	%xmm14, %xmm1
	movq	%xmm5, %xmm4
	shufps	$0xe5, %xmm13, %xmm13
	movq	%xmm8, %xmm10
	movq	%xmm13, %xmm5
	movaps	%xmm1, %xmm2
	shufps	$0xe0, %xmm12, %xmm12
	movq	%xmm12, %xmm12
	movq	%xmm3, %xmm3
	movaps	%xmm11, -48(%rbp)
	mulps	%xmm12, %xmm2
	movq	%xmm9, %xmm11
	movq	(%rbx,%r8,8), %xmm9
	mulps	%xmm10, %xmm12
	addq	%rsi, %r8
	movq	%xmm6, %xmm6
	movaps	%xmm0, -64(%rbp)
	mulps	%xmm5, %xmm10
	movaps	%xmm15, %xmm0
	mulps	%xmm11, %xmm0
	mulps	%xmm1, %xmm5
	movq	(%r10,%r8,8), %xmm1
	mulps	%xmm7, %xmm11
	movq	%xmm2, %xmm2
	mulps	%xmm4, %xmm7
	movaps	%xmm1, %xmm8
	shufps	$0xe5, %xmm1, %xmm1
	movq	%xmm1, %xmm1
	mulps	%xmm15, %xmm4
	movaps	%xmm9, %xmm15
	shufps	$0xe0, %xmm8, %xmm8
	movq	%xmm9, %xmm9
	shufps	$0xe1, %xmm15, %xmm15
	movq	%xmm8, %xmm13
	movq	%xmm10, %xmm10
	movq	%xmm15, %xmm14
	movaps	%xmm10, %xmm15
	movq	%xmm12, %xmm12
	movaps	%xmm14, %xmm8
	addps	%xmm2, %xmm15
	subps	%xmm10, %xmm2
	mulps	%xmm13, %xmm8
	movq	%xmm5, %xmm5
	movq	%xmm7, %xmm7
	mulps	%xmm9, %xmm13
	movq	%xmm0, %xmm0
	movq	%xmm11, %xmm11
	mulps	%xmm1, %xmm9
	movss	%xmm15, %xmm2
	movq	%xmm4, %xmm4
	mulps	%xmm14, %xmm1
	movq	%xmm2, %xmm2
	movq	%xmm8, %xmm8
	movq	%xmm13, %xmm13
	movq	%xmm9, %xmm9
	movaps	%xmm13, %xmm14
	movaps	%xmm9, %xmm10
	addps	%xmm8, %xmm10
	subps	%xmm9, %xmm8
	movss	%xmm10, %xmm8
	movq	%xmm8, %xmm15
	movq	%xmm1, %xmm8
	movaps	%xmm12, %xmm1
	subps	%xmm8, %xmm14
	subps	%xmm5, %xmm1
	addps	%xmm13, %xmm8
	addps	%xmm12, %xmm5
	movq	(%rbx,%r8,8), %xmm13
	addq	%rsi, %r8
	subps	%xmm15, %xmm2
	movaps	%xmm13, %xmm10
	movss	%xmm14, %xmm8
	movss	%xmm1, %xmm5
	movq	(%r10,%r8,8), %xmm14
	shufps	$0xe1, %xmm10, %xmm10
	movq	%xmm5, %xmm9
	movq	%xmm8, %xmm15
	movq	%xmm2, %xmm2
	addps	%xmm9, %xmm15
	movaps	%xmm14, %xmm8
	movq	%xmm10, %xmm9
	shufps	$0xe5, %xmm14, %xmm14
	movq	%xmm13, %xmm10
	movq	%xmm14, %xmm1
	movaps	%xmm9, %xmm12
	shufps	$0xe0, %xmm8, %xmm8
	movq	%xmm8, %xmm5
	movaps	%xmm7, %xmm13
	mulps	%xmm5, %xmm12
	addps	%xmm0, %xmm13
	subps	%xmm7, %xmm0
	mulps	%xmm10, %xmm5
	movq	%xmm15, %xmm15
	mulps	%xmm1, %xmm10
	mulps	%xmm9, %xmm1
	movss	%xmm13, %xmm0
	movq	%xmm0, %xmm13
	movaps	%xmm11, %xmm0
	movq	%xmm12, %xmm14
	subps	%xmm4, %xmm0
	addps	%xmm11, %xmm4
	movq	%xmm5, %xmm5
	movaps	%xmm6, %xmm11
	movq	%xmm10, %xmm8
	movaps	%xmm5, %xmm9
	addps	%xmm3, %xmm11
	movaps	%xmm8, %xmm12
	movq	%xmm1, %xmm1
	movss	%xmm0, %xmm4
	addps	%xmm14, %xmm12
	subps	%xmm8, %xmm14
	movq	(%rbx,%r8,8), %xmm8
	addq	%rsi, %r8
	subps	%xmm1, %xmm9
	addps	%xmm5, %xmm1
	movq	(%r10,%r8,8), %xmm5
	movq	%xmm4, %xmm7
	subps	%xmm6, %xmm3
	movss	%xmm12, %xmm14
	movaps	%xmm8, %xmm12
	movq	%xmm8, %xmm8
	movq	%xmm14, %xmm10
	movss	%xmm9, %xmm1
	shufps	$0xe1, %xmm12, %xmm12
	movq	%xmm12, %xmm9
	subps	%xmm10, %xmm13
	movq	%xmm1, %xmm14
	movaps	%xmm5, %xmm10
	addps	%xmm7, %xmm14
	movaps	%xmm9, %xmm4
	shufps	$0xe5, %xmm5, %xmm5
	shufps	$0xe0, %xmm10, %xmm10
	movq	%xmm5, %xmm7
	movq	%xmm10, %xmm1
	movss	%xmm11, %xmm3
	mulps	%xmm1, %xmm4
	movq	%xmm13, %xmm13
	mulps	%xmm8, %xmm1
	movq	%xmm14, %xmm14
	mulps	%xmm7, %xmm8
	mulps	%xmm9, %xmm7
	movq	%xmm4, %xmm0
	movq	%xmm3, %xmm4
	movaps	%xmm13, %xmm3
	movq	%xmm1, %xmm1
	movq	%xmm8, %xmm12
	movaps	.LC63(%rip), %xmm8
	movaps	%xmm12, %xmm5
	movq	%xmm7, %xmm9
	movaps	%xmm1, %xmm7
	addps	%xmm0, %xmm5
	subps	%xmm12, %xmm0
	movaps	.LC64(%rip), %xmm12
	subps	%xmm9, %xmm7
	addps	%xmm1, %xmm9
	movaps	%xmm14, %xmm1
	movss	%xmm5, %xmm0
	movaps	.LC62(%rip), %xmm5
	movq	%xmm0, %xmm10
	movss	%xmm7, %xmm9
	movq	(%r10,%r12,8), %xmm7
	leaq	(%rsi,%rsi,2), %r10
	subps	%xmm10, %xmm4
	mulps	%xmm5, %xmm3
	movaps	%xmm2, %xmm10
	movq	%rsi, %r12
	mulps	%xmm12, %xmm10
	negq	%r12
	movq	%r10, %rbx
	movq	%r12, %r9
	salq	$4, %rbx
	salq	$5, %r9
	movq	%xmm4, %xmm0
	addq	%r13, %rbx
	salq	$4, %r12
	mulps	%xmm0, %xmm8
	mulps	%xmm0, %xmm12
	movq	%xmm3, %xmm6
	movaps	.LC65(%rip), %xmm3
	mulps	%xmm5, %xmm0
	movq	%xmm10, %xmm4
	movq	%xmm8, %xmm11
	movaps	%xmm13, %xmm8
	mulps	%xmm3, %xmm8
	addps	%xmm11, %xmm6
	movq	%xmm12, %xmm11
	mulps	.LC66(%rip), %xmm13
	movaps	%xmm2, %xmm12
	mulps	%xmm5, %xmm12
	movq	%xmm0, %xmm5
	mulps	%xmm3, %xmm2
	movq	-64(%rbp), %xmm3
	movq	%xmm6, %xmm6
	addps	%xmm4, %xmm6
	movq	%xmm8, %xmm10
	addps	%xmm11, %xmm10
	movq	-48(%rbp), %xmm11
	movq	%xmm13, %xmm13
	movq	%xmm12, %xmm4
	addps	%xmm5, %xmm13
	movq	%xmm6, %xmm6
	movq	%xmm2, %xmm2
	movq	%xmm10, %xmm8
	movaps	%xmm11, %xmm10
	subps	%xmm3, %xmm10
	addps	%xmm11, %xmm3
	movq	%xmm13, %xmm0
	movaps	.LC67(%rip), %xmm13
	addps	%xmm4, %xmm8
	movq	%xmm9, %xmm4
	addps	%xmm2, %xmm0
	movss	%xmm10, %xmm3
	movq	%xmm3, %xmm12
	movaps	.LC68(%rip), %xmm3
	movq	%xmm0, %xmm0
	addps	%xmm12, %xmm4
	movaps	%xmm15, %xmm12
	mulps	%xmm3, %xmm1
	movq	%xmm4, %xmm2
	movaps	%xmm2, %xmm5
	mulps	%xmm13, %xmm5
	movq	%xmm1, %xmm11
	movaps	.LC69(%rip), %xmm1
	mulps	%xmm1, %xmm12
	movq	%xmm5, %xmm9
	addps	%xmm7, %xmm9
	movq	%xmm12, %xmm4
	movaps	%xmm14, %xmm12
	mulps	%xmm13, %xmm12
	movq	%xmm9, %xmm10
	addps	%xmm11, %xmm10
	mulps	%xmm15, %xmm13
	movq	%xmm10, %xmm5
	movaps	%xmm2, %xmm10
	mulps	%xmm1, %xmm10
	addps	%xmm4, %xmm5
	mulps	%xmm14, %xmm1
	movq	%xmm13, %xmm13
	movq	%xmm5, %xmm11
	movaps	%xmm11, %xmm9
	movq	%xmm10, %xmm4
	movaps	%xmm15, %xmm10
	subps	%xmm6, %xmm9
	mulps	%xmm3, %xmm10
	addps	%xmm7, %xmm4
	addps	%xmm11, %xmm6
	mulps	%xmm2, %xmm3
	movq	%xmm12, %xmm11
	movq	%xmm1, %xmm1
	movq	%xmm4, %xmm5
	addps	%xmm11, %xmm5
	movq	%xmm8, %xmm11
	movq	%xmm10, %xmm4
	movaps	%xmm11, %xmm8
	movq	%xmm3, %xmm3
	addps	%xmm7, %xmm3
	movq	%xmm5, %xmm12
	addps	%xmm2, %xmm7
	addps	%xmm4, %xmm12
	movq	%xmm3, %xmm4
	movq	%xmm7, %xmm2
	addps	%xmm1, %xmm4
	addps	%xmm14, %xmm2
	movq	%xmm12, %xmm5
	addps	%xmm5, %xmm8
	subps	%xmm11, %xmm5
	movaps	%xmm0, %xmm11
	movaps	%xmm9, %xmm14
	movq	%xmm4, %xmm10
	movq	%xmm2, %xmm7
	movss	%xmm6, %xmm14
	addps	%xmm13, %xmm10
	addps	%xmm15, %xmm7
	movss	%xmm9, %xmm6
	movq	%xmm10, %xmm12
	movlps	%xmm7, 0(%r13)
	addps	%xmm12, %xmm11
	subps	%xmm0, %xmm12
	movlps	%xmm14, (%rcx)
	movlps	%xmm6, (%rbx)
	addq	%r9, %rbx
	movaps	%xmm5, %xmm6
	leaq	(%rbx,%r10,8), %rdi
	movss	%xmm8, %xmm6
	movss	%xmm5, %xmm8
	movaps	%xmm12, %xmm15
	movss	%xmm11, %xmm15
	movss	%xmm12, %xmm11
	movlps	%xmm15, (%rbx)
	movlps	%xmm11, (%rdi)
	addq	%r12, %rdi
	movlps	%xmm6, (%rdi)
	movlps	%xmm8, (%rdi,%rsi,8)
.L1744:
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
.L1751:
	.cfi_restore 3
	.cfi_restore 6
	.cfi_restore 12
	.cfi_restore 13
	ret
	.cfi_endproc
.LFE7643:
	.size	_ZN6micron4math3fft6__impl7__stageILm7ELb0EfEEvPNS0_7complexIT1_EEPKS6_mmm, .-_ZN6micron4math3fft6__impl7__stageILm7ELb0EfEEvPNS0_7complexIT1_EEPKS6_mmm
