_ZN6micron4math3fft6__impl7__stageILm4ELb0EfEEvPNS0_7complexIT1_EEPKS6_mmm:
.LFB7641:
	.cfi_startproc
	cmpq	%r8, %rcx
	jnb	.L1729
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsi, %r9
	movq	%rcx, %rax
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	pushq	%r12
	.cfi_offset 12, -24
	movq	%r8, %r12
	subq	%rcx, %r12
	pushq	%rbx
	.cfi_offset 3, -32
	movq	%rdx, %rbx
	andl	$1, %r12d
	subq	%r12, %r8
	cmpq	%r8, %rcx
	jnb	.L1724
	movq	%rdx, %r10
	leaq	0(,%rdx,8), %r11
	movaps	.LC26(%rip), %xmm3
	leaq	(%rdx,%rdx,2), %rdx
	salq	$4, %r10
	movaps	.LC29(%rip), %xmm4
	leaq	(%rdi,%r11), %rsi
	leaq	(%rdi,%rdx,8), %rdx
	leaq	(%rdi,%r10), %rcx
	addq	%r9, %r11
	addq	%r9, %r10
	.p2align 4
	.p2align 3
.L1725:
	movups	(%r9,%rax,8), %xmm1
	movups	(%rsi,%rax,8), %xmm2
	movups	(%r10,%rax,8), %xmm12
	movups	(%rdx,%rax,8), %xmm13
	movups	(%r11,%rax,8), %xmm7
	movups	(%rcx,%rax,8), %xmm8
	movaps	%xmm2, %xmm0
	movaps	%xmm1, %xmm6
	movaps	%xmm13, %xmm14
	movaps	%xmm12, %xmm15
	shufps	$177, %xmm1, %xmm6
	shufps	$245, %xmm2, %xmm0
	movaps	%xmm7, %xmm10
	shufps	$245, %xmm13, %xmm14
	movaps	%xmm8, %xmm9
	shufps	$177, %xmm12, %xmm15
	mulps	%xmm6, %xmm0
	movaps	%xmm8, %xmm11
	shufps	$177, %xmm7, %xmm10
	shufps	$160, %xmm2, %xmm2
	mulps	%xmm15, %xmm14
	shufps	$245, %xmm8, %xmm9
	shufps	$160, %xmm13, %xmm13
	shufps	$160, %xmm8, %xmm11
	mulps	%xmm1, %xmm2
	movups	(%rdi,%rax,8), %xmm5
	mulps	%xmm10, %xmm9
	mulps	%xmm12, %xmm13
	movaps	%xmm5, %xmm6
	mulps	%xmm7, %xmm11
	xorps	%xmm3, %xmm0
	xorps	%xmm3, %xmm14
	addps	%xmm0, %xmm2
	xorps	%xmm3, %xmm9
	addps	%xmm14, %xmm13
	addps	%xmm9, %xmm11
	movaps	%xmm2, %xmm7
	subps	%xmm13, %xmm2
	addps	%xmm13, %xmm7
	addps	%xmm11, %xmm6
	subps	%xmm11, %xmm5
	movaps	%xmm2, %xmm0
	movaps	%xmm6, %xmm1
	shufps	$177, %xmm2, %xmm0
	movaps	%xmm5, %xmm2
	xorps	%xmm4, %xmm0
	addps	%xmm7, %xmm1
	subps	%xmm7, %xmm6
	addps	%xmm0, %xmm2
	subps	%xmm0, %xmm5
	movups	%xmm1, (%rdi,%rax,8)
	movups	%xmm6, (%rcx,%rax,8)
	movups	%xmm2, (%rsi,%rax,8)
	movups	%xmm5, (%rdx,%rax,8)
	addq	$2, %rax
	cmpq	%r8, %rax
	jb	.L1725
.L1724:
	testq	%r12, %r12
	je	.L1722
	leaq	(%r8,%rbx), %rax
	movq	(%r9,%r8,8), %xmm5
	leaq	(%rdi,%r8,8), %r12
	movq	(%r9,%rax,8), %xmm10
	leaq	(%rdi,%rax,8), %r10
	addq	%rbx, %rax
	movq	(%rdi,%rax,8), %xmm15
	movq	(%r10), %xmm8
	movaps	%xmm5, %xmm4
	movq	%xmm5, %xmm13
	movq	%xmm10, %xmm6
	shufps	$0xe1, %xmm10, %xmm10
	shufps	$0xe1, %xmm4, %xmm4
	movaps	%xmm15, %xmm7
	movq	%xmm10, %xmm2
	shufps	$0xe5, %xmm15, %xmm15
	movq	%xmm15, %xmm0
	shufps	$0xe0, %xmm7, %xmm7
	mulps	%xmm2, %xmm0
	movq	%xmm7, %xmm1
	movaps	%xmm8, %xmm3
	mulps	%xmm6, %xmm1
	shufps	$0xe5, %xmm8, %xmm8
	movq	%xmm8, %xmm14
	movq	%xmm4, %xmm9
	movq	(%r9,%rax,8), %xmm10
	movaps	%xmm9, %xmm11
	shufps	$0xe0, %xmm3, %xmm3
	movq	%xmm3, %xmm12
	mulps	%xmm12, %xmm11
	addq	%rbx, %rax
	movq	(%rdi,%r8,8), %xmm15
	mulps	%xmm13, %xmm12
	movq	(%rdi,%rax,8), %xmm2
	movaps	%xmm10, %xmm7
	movq	%rbx, %rdi
	mulps	%xmm14, %xmm13
	movq	%xmm0, %xmm4
	shufps	$0xe1, %xmm7, %xmm7
	salq	$4, %rdi
	mulps	%xmm9, %xmm14
	movq	%xmm1, %xmm8
	movaps	%xmm15, %xmm9
	movaps	%xmm8, %xmm5
	movaps	%xmm2, %xmm6
	shufps	$0xe5, %xmm2, %xmm2
	movq	%xmm2, %xmm0
	subps	%xmm4, %xmm5
	addps	%xmm8, %xmm4
	movq	%xmm7, %xmm8
	shufps	$0xe0, %xmm6, %xmm6
	movaps	%xmm8, %xmm1
	movq	%xmm12, %xmm12
	movq	%xmm13, %xmm13
	movss	%xmm5, %xmm4
	movq	%xmm6, %xmm5
	movq	%xmm14, %xmm14
	mulps	%xmm5, %xmm1
	movq	%xmm4, %xmm3
	movq	%xmm11, %xmm4
	subps	%xmm3, %xmm9
	addps	%xmm3, %xmm15
	movq	%xmm10, %xmm3
	mulps	%xmm3, %xmm5
	movaps	%xmm13, %xmm11
	mulps	%xmm0, %xmm3
	addps	%xmm4, %xmm11
	subps	%xmm13, %xmm4
	mulps	%xmm8, %xmm0
	movq	%xmm9, %xmm9
	movq	%xmm15, %xmm15
	movq	%xmm1, %xmm2
	movaps	%xmm4, %xmm7
	movaps	%xmm9, %xmm4
	movq	%xmm5, %xmm5
	movss	%xmm11, %xmm7
	movaps	%xmm12, %xmm11
	movq	%xmm3, %xmm10
	subps	%xmm14, %xmm11
	addps	%xmm12, %xmm14
	movq	%xmm0, %xmm8
	movaps	%xmm10, %xmm6
	movaps	%xmm5, %xmm0
	addps	%xmm2, %xmm6
	subps	%xmm8, %xmm0
	movq	%xmm7, %xmm1
	subps	%xmm10, %xmm2
	addps	%xmm5, %xmm8
	movss	%xmm11, %xmm14
	movq	%xmm14, %xmm7
	movss	%xmm6, %xmm2
	movss	%xmm0, %xmm8
	movq	%xmm2, %xmm3
	movq	%xmm8, %xmm2
	subps	%xmm3, %xmm1
	addps	%xmm7, %xmm2
	movq	%xmm1, %xmm13
	movq	%xmm2, %xmm10
	subps	%xmm13, %xmm4
	movaps	%xmm10, %xmm6
	addps	%xmm9, %xmm13
	addps	%xmm15, %xmm6
	subps	%xmm10, %xmm15
	movaps	%xmm4, %xmm3
	movlps	%xmm6, (%r12)
	movss	%xmm13, %xmm3
	addq	%rdi, %r12
	movss	%xmm4, %xmm13
	movlps	%xmm15, (%r12)
	movlps	%xmm3, (%r10)
	movlps	%xmm13, (%r12,%rbx,8)
.L1722:
	popq	%rbx
	popq	%r12
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.p2align 4,,10
	.p2align 3
.L1729:
	.cfi_restore 3
	.cfi_restore 6
	.cfi_restore 12
	ret
	.cfi_endproc
.LFE7641:
	.size	_ZN6micron4math3fft6__impl7__stageILm4ELb0EfEEvPNS0_7complexIT1_EEPKS6_mmm, .-_ZN6micron4math3fft6__impl7__stageILm4ELb0EfEEvPNS0_7complexIT1_EEPKS6_mmm
