_ZN6micron4math3fft6__impl7__stageILm4ELb0EfEEvPNS0_7complexIT1_EEPKS6_mmm:
.LFB7650:
	.cfi_startproc
	cmpq	%r8, %rcx
	jnb	.L1682
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsi, %r11
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$40, %rsp
	movq	%r14, -16(%rbp)
	.cfi_offset 14, -32
	movq	%r8, %r14
	subq	%rcx, %r14
	movq	%r13, -24(%rbp)
	.cfi_offset 13, -40
	movq	%r14, %r13
	movq	%r12, -32(%rbp)
	.cfi_offset 12, -48
	movq	%rdx, %r12
	andl	$3, %r13d
	movq	%rbx, -40(%rbp)
	subq	%r13, %r8
	cmpq	%r8, %rcx
	.cfi_offset 3, -56
	jnb	.L1664
	movq	%r15, -8(%rbp)
	.cfi_offset 15, -24
	movq	%rdx, %rbx
	leaq	(%rdx,%rdx,2), %rax
	leaq	0(,%rdx,8), %r15
	leaq	(%rdi,%r15), %r9
	vmovdqu	(%r11,%rcx,8), %ymm4
	addq	%rsi, %r15
	movq	%rcx, %rdx
	vmovdqu	(%r9,%rcx,8), %ymm1
	salq	$4, %rbx
	notq	%rdx
	vmovdqu	(%r15,%rcx,8), %ymm8
	vpermilps	$177, %ymm4, %ymm5
	leaq	(%rdi,%rbx), %r10
	addq	%rsi, %rbx
	vmovdqu	(%rdi,%rcx,8), %ymm3
	vmovshdup	%ymm1, %ymm6
	vmovsldup	%ymm1, %ymm0
	leaq	(%rdi,%rax,8), %rsi
	addq	%r8, %rdx
	vmovdqu	(%r10,%rcx,8), %ymm9
	vmovdqu	(%rsi,%rcx,8), %ymm15
	vpermilps	$177, %ymm8, %ymm12
	leaq	4(%rcx), %rax
	vmovdqu	(%rbx,%rcx,8), %ymm14
	vmovaps	.LC29(%rip), %ymm2
	shrq	$2, %rdx
	vmovshdup	%ymm9, %ymm11
	vmovsldup	%ymm9, %ymm10
	vmulps	%ymm5, %ymm6, %ymm7
	vmovsldup	%ymm15, %ymm6
	vpermilps	$177, %ymm14, %ymm1
	vmulps	%ymm12, %ymm11, %ymm13
	vfmaddsub132ps	%ymm4, %ymm7, %ymm0
	vmovshdup	%ymm15, %ymm4
	vfmaddsub132ps	%ymm8, %ymm13, %ymm10
	vmulps	%ymm1, %ymm4, %ymm5
	vaddps	%ymm3, %ymm10, %ymm7
	vsubps	%ymm10, %ymm3, %ymm8
	vfmaddsub132ps	%ymm14, %ymm5, %ymm6
	vaddps	%ymm6, %ymm0, %ymm3
	vsubps	%ymm6, %ymm0, %ymm0
	vpermilps	$177, %ymm0, %ymm9
	vaddps	%ymm3, %ymm7, %ymm11
	vsubps	%ymm3, %ymm7, %ymm12
	vxorps	%ymm2, %ymm9, %ymm10
	vaddps	%ymm10, %ymm8, %ymm13
	vsubps	%ymm10, %ymm8, %ymm14
	vmovdqu	%ymm11, (%rdi,%rcx,8)
	vmovdqu	%ymm12, (%r10,%rcx,8)
	vmovdqu	%ymm13, (%r9,%rcx,8)
	vmovdqu	%ymm14, (%rsi,%rcx,8)
	cmpq	%r8, %rax
	jnb	.L1685
	andl	$1, %edx
	je	.L1679
	vmovdqu	(%r9,%rax,8), %ymm4
	vmovdqu	(%r11,%rax,8), %ymm6
	vmovdqu	(%rsi,%rax,8), %ymm14
	vmovdqu	(%r10,%rax,8), %ymm0
	vmovshdup	%ymm4, %ymm1
	vpermilps	$177, %ymm6, %ymm5
	vmovdqu	(%rbx,%rax,8), %ymm13
	vmovdqu	(%r15,%rax,8), %ymm3
	vmovsldup	%ymm4, %ymm8
	vmovshdup	%ymm14, %ymm4
	vmovdqu	(%rdi,%rax,8), %ymm15
	vmovshdup	%ymm0, %ymm10
	vmovsldup	%ymm0, %ymm9
	vpermilps	$177, %ymm3, %ymm11
	vmulps	%ymm5, %ymm1, %ymm7
	vpermilps	$177, %ymm13, %ymm1
	vmulps	%ymm1, %ymm4, %ymm5
	vmulps	%ymm11, %ymm10, %ymm12
	vfmaddsub132ps	%ymm6, %ymm7, %ymm8
	vmovsldup	%ymm14, %ymm6
	vfmaddsub132ps	%ymm3, %ymm12, %ymm9
	vfmaddsub132ps	%ymm13, %ymm5, %ymm6
	vaddps	%ymm15, %ymm9, %ymm7
	vsubps	%ymm9, %ymm15, %ymm15
	vaddps	%ymm6, %ymm8, %ymm3
	vsubps	%ymm6, %ymm8, %ymm8
	vpermilps	$177, %ymm8, %ymm0
	vaddps	%ymm3, %ymm7, %ymm10
	vsubps	%ymm3, %ymm7, %ymm11
	vxorps	%ymm2, %ymm0, %ymm9
	vaddps	%ymm9, %ymm15, %ymm12
	vsubps	%ymm9, %ymm15, %ymm13
	vmovdqu	%ymm10, (%rdi,%rax,8)
	vmovdqu	%ymm11, (%r10,%rax,8)
	vmovdqu	%ymm12, (%r9,%rax,8)
	vmovdqu	%ymm13, (%rsi,%rax,8)
	leaq	8(%rcx), %rax
	cmpq	%r8, %rax
	jnb	.L1685
	.p2align 4
	.p2align 3
.L1679:
	vmovdqu	(%r9,%rax,8), %ymm4
	vmovdqu	(%r11,%rax,8), %ymm6
	leaq	4(%rax), %rcx
	vmovdqu	(%rsi,%rax,8), %ymm13
	vmovdqu	(%r10,%rax,8), %ymm8
	vmovshdup	%ymm4, %ymm1
	vpermilps	$177, %ymm6, %ymm5
	vmovdqu	(%rbx,%rax,8), %ymm12
	vmovdqu	(%r15,%rax,8), %ymm3
	vmovsldup	%ymm4, %ymm15
	vmovshdup	%ymm13, %ymm4
	vmovdqu	(%rdi,%rax,8), %ymm14
	vmovshdup	%ymm8, %ymm9
	vmovsldup	%ymm8, %ymm0
	vpermilps	$177, %ymm3, %ymm10
	vmulps	%ymm5, %ymm1, %ymm7
	vpermilps	$177, %ymm12, %ymm1
	vmulps	%ymm1, %ymm4, %ymm5
	vmulps	%ymm10, %ymm9, %ymm11
	vfmaddsub132ps	%ymm6, %ymm7, %ymm15
	vmovsldup	%ymm13, %ymm6
	vfmaddsub132ps	%ymm3, %ymm11, %ymm0
	vfmaddsub132ps	%ymm12, %ymm5, %ymm6
	vaddps	%ymm14, %ymm0, %ymm7
	vsubps	%ymm0, %ymm14, %ymm14
	vaddps	%ymm6, %ymm15, %ymm3
	vsubps	%ymm6, %ymm15, %ymm15
	vpermilps	$177, %ymm15, %ymm8
	vaddps	%ymm3, %ymm7, %ymm9
	vsubps	%ymm3, %ymm7, %ymm10
	vxorps	%ymm2, %ymm8, %ymm0
	vaddps	%ymm0, %ymm14, %ymm11
	vsubps	%ymm0, %ymm14, %ymm12
	vmovdqu	%ymm9, (%rdi,%rax,8)
	vmovdqu	%ymm10, (%r10,%rax,8)
	vmovdqu	%ymm11, (%r9,%rax,8)
	vmovdqu	%ymm12, (%rsi,%rax,8)
	vmovdqu	(%r9,%rcx,8), %ymm4
	vmovdqu	(%rsi,%rcx,8), %ymm12
	addq	$8, %rax
	vmovdqu	(%r11,%rcx,8), %ymm6
	vmovdqu	(%r10,%rcx,8), %ymm15
	vmovshdup	%ymm4, %ymm1
	vmovsldup	%ymm4, %ymm14
	vmovdqu	(%rbx,%rcx,8), %ymm11
	vmovdqu	(%r15,%rcx,8), %ymm3
	vpermilps	$177, %ymm6, %ymm5
	vmovshdup	%ymm12, %ymm4
	vmovdqu	(%rdi,%rcx,8), %ymm13
	vmovshdup	%ymm15, %ymm0
	vmovsldup	%ymm15, %ymm8
	vpermilps	$177, %ymm3, %ymm9
	vmulps	%ymm5, %ymm1, %ymm7
	vpermilps	$177, %ymm11, %ymm1
	vmulps	%ymm1, %ymm4, %ymm5
	vmulps	%ymm9, %ymm0, %ymm10
	vfmaddsub132ps	%ymm6, %ymm7, %ymm14
	vmovsldup	%ymm12, %ymm6
	vfmaddsub132ps	%ymm3, %ymm10, %ymm8
	vfmaddsub132ps	%ymm11, %ymm5, %ymm6
	vaddps	%ymm13, %ymm8, %ymm7
	vsubps	%ymm8, %ymm13, %ymm13
	vaddps	%ymm6, %ymm14, %ymm3
	vsubps	%ymm6, %ymm14, %ymm14
	vpermilps	$177, %ymm14, %ymm15
	vaddps	%ymm3, %ymm7, %ymm0
	vsubps	%ymm3, %ymm7, %ymm9
	vxorps	%ymm2, %ymm15, %ymm8
	vaddps	%ymm8, %ymm13, %ymm10
	vsubps	%ymm8, %ymm13, %ymm11
	vmovdqu	%ymm0, (%rdi,%rcx,8)
	vmovdqu	%ymm9, (%r10,%rcx,8)
	vmovdqu	%ymm10, (%r9,%rcx,8)
	vmovdqu	%ymm11, (%rsi,%rcx,8)
	cmpq	%r8, %rax
	jb	.L1679
.L1685:
	vzeroupper
	movq	-8(%rbp), %r15
	.cfi_restore 15
.L1664:
	testq	%r13, %r13
	je	.L1680
	leaq	0(,%r8,8), %r9
	addq	%r12, %r8
	leaq	0(,%r8,8), %rsi
	addq	%r12, %r8
	leaq	(%r11,%r9), %rbx
	leaq	(%r11,%rsi), %r10
	addq	%rdi, %rsi
	leaq	0(,%r8,8), %rdx
	vmovq	(%rbx), %xmm2
	vmovq	(%rsi), %xmm6
	leaq	(%rdi,%rdx), %rax
	vmovq	(%r10), %xmm9
	addq	%r12, %r8
	vmovq	(%rax), %xmm10
	vshufps	$0xe1, %xmm2, %xmm2, %xmm12
	vmovq	%xmm2, %xmm13
	leaq	(%rdi,%r8,8), %r8
	vmovshdup	%xmm6, %xmm1
	vmovq	%xmm12, %xmm15
	vshufps	$0xe1, %xmm9, %xmm9, %xmm2
	addq	%rdx, %r11
	vmovsldup	%xmm6, %xmm4
	vmovshdup	%xmm10, %xmm12
	andl	$2, %r14d
	vmovsldup	%xmm10, %xmm11
	vmovq	%xmm1, %xmm5
	vmulps	%xmm5, %xmm13, %xmm7
	vmovq	%xmm4, %xmm3
	vmovq	%xmm12, %xmm6
	vmulps	%xmm5, %xmm15, %xmm8
	vmovq	%xmm2, %xmm4
	vmovq	(%r8), %xmm2
	vmulps	%xmm6, %xmm4, %xmm1
	vmovshdup	%xmm2, %xmm6
	vmovsldup	%xmm2, %xmm12
	vmovq	%xmm7, %xmm14
	vmovq	%xmm11, %xmm7
	vmovq	%xmm8, %xmm0
	vfmsubadd231ps	%xmm3, %xmm15, %xmm14
	vfmaddsub132ps	%xmm3, %xmm0, %xmm13
	vmovq	%xmm9, %xmm3
	vmovq	(%r11), %xmm9
	vmovq	%xmm1, %xmm5
	vfmaddsub132ps	%xmm7, %xmm5, %xmm3
	vmovq	%xmm6, %xmm4
	vmovq	(%rdi,%r9), %xmm0
	vshufps	$0xe1, %xmm9, %xmm9, %xmm11
	vmovq	%xmm9, %xmm7
	vmulps	%xmm4, %xmm7, %xmm1
	vmovq	%xmm11, %xmm5
	vmulps	%xmm4, %xmm5, %xmm4
	vmovq	%xmm14, %xmm14
	vmovq	%xmm13, %xmm13
	vmovq	%xmm3, %xmm15
	vsubps	%xmm15, %xmm0, %xmm8
	vaddps	%xmm15, %xmm0, %xmm10
	vmovq	%xmm12, %xmm15
	vmovq	%xmm1, %xmm3
	vfmsubadd231ps	%xmm15, %xmm5, %xmm3
	vmovq	%xmm4, %xmm1
	vfmaddsub132ps	%xmm15, %xmm1, %xmm7
	vmovq	%xmm8, %xmm2
	vmovq	%xmm10, %xmm10
	vmovq	%xmm3, %xmm0
	vsubps	%xmm0, %xmm14, %xmm9
	vmovq	%xmm7, %xmm7
	vaddps	%xmm13, %xmm7, %xmm15
	vmovq	%xmm9, %xmm11
	vmovq	%xmm15, %xmm5
	vaddps	%xmm11, %xmm2, %xmm12
	vsubps	%xmm11, %xmm2, %xmm8
	vaddsubps	%xmm11, %xmm2, %xmm3
	vaddps	%xmm10, %xmm5, %xmm0
	vsubps	%xmm5, %xmm10, %xmm14
	vblendps	$1, %xmm12, %xmm8, %xmm6
	vmovlps	%xmm0, (%rdi,%r9)
	vmovlps	%xmm14, (%rax)
	vmovlps	%xmm6, (%rsi)
	vmovlps	%xmm3, (%r8)
	je	.L1680
	vmovq	8(%rsi), %xmm2
	vmovq	8(%rax), %xmm5
	vmovq	8(%rbx), %xmm9
	vmovq	8(%r10), %xmm14
	vmovshdup	%xmm2, %xmm8
	vmovsldup	%xmm2, %xmm12
	vmovshdup	%xmm5, %xmm0
	vshufps	$0xe1, %xmm9, %xmm9, %xmm11
	vmovq	%xmm9, %xmm3
	vmovq	%xmm11, %xmm7
	vmovsldup	%xmm5, %xmm9
	vshufps	$0xe1, %xmm14, %xmm14, %xmm11
	vmovq	%xmm11, %xmm2
	vmovq	%xmm8, %xmm6
	vmovq	%xmm12, %xmm13
	vmulps	%xmm3, %xmm6, %xmm4
	vmovq	%xmm0, %xmm12
	vmovq	8(%r8), %xmm0
	vmulps	%xmm6, %xmm7, %xmm15
	vmulps	%xmm2, %xmm12, %xmm8
	vmovshdup	%xmm0, %xmm2
	vmovsldup	%xmm0, %xmm11
	vmovq	%xmm4, %xmm1
	vmovq	%xmm9, %xmm4
	vmovq	8(%r11), %xmm9
	vmovq	%xmm15, %xmm10
	vfmsubadd231ps	%xmm13, %xmm7, %xmm1
	vmovq	%xmm2, %xmm12
	vmovq	8(%rdi,%r9), %xmm15
	vfmaddsub132ps	%xmm13, %xmm10, %xmm3
	vmovq	%xmm8, %xmm6
	vmovq	%xmm14, %xmm13
	vfmaddsub231ps	%xmm4, %xmm13, %xmm6
	vmovq	%xmm9, %xmm4
	vshufps	$0xe1, %xmm9, %xmm9, %xmm5
	vmulps	%xmm12, %xmm4, %xmm8
	vmovq	%xmm1, %xmm1
	vmovq	%xmm3, %xmm3
	vmovq	%xmm6, %xmm7
	vmovq	%xmm11, %xmm6
	vsubps	%xmm7, %xmm15, %xmm10
	vaddps	%xmm15, %xmm7, %xmm14
	vmovq	%xmm8, %xmm13
	vmovq	%xmm5, %xmm7
	vmulps	%xmm12, %xmm7, %xmm12
	vfmsubadd231ps	%xmm6, %xmm7, %xmm13
	vmovq	%xmm10, %xmm10
	vmovq	%xmm14, %xmm14
	vmovq	%xmm13, %xmm15
	vmovq	%xmm12, %xmm13
	vfmaddsub132ps	%xmm6, %xmm13, %xmm4
	vsubps	%xmm15, %xmm1, %xmm9
	vmovq	%xmm9, %xmm5
	vaddps	%xmm10, %xmm5, %xmm0
	vsubps	%xmm5, %xmm10, %xmm11
	vmovq	%xmm4, %xmm4
	vaddsubps	%xmm5, %xmm10, %xmm2
	vaddps	%xmm3, %xmm4, %xmm6
	vblendps	$1, %xmm0, %xmm11, %xmm8
	vmovq	%xmm6, %xmm7
	vaddps	%xmm7, %xmm14, %xmm15
	vsubps	%xmm7, %xmm14, %xmm1
	vmovlps	%xmm15, 8(%rdi,%r9)
	vmovlps	%xmm1, 8(%rax)
	vmovlps	%xmm8, 8(%rsi)
	vmovlps	%xmm2, 8(%r8)
	cmpq	$3, %r13
	jne	.L1680
	vmovq	16(%rsi), %xmm5
	vmovq	16(%rbx), %xmm9
	vmovq	16(%rax), %xmm15
	vmovq	16(%r10), %xmm14
	vmovshdup	%xmm5, %xmm0
	vmovsldup	%xmm5, %xmm11
	vshufps	$0xe1, %xmm9, %xmm9, %xmm10
	vmovq	%xmm9, %xmm12
	vmovshdup	%xmm15, %xmm9
	vmovq	%xmm10, %xmm4
	vshufps	$0xe1, %xmm14, %xmm14, %xmm10
	vmovsldup	%xmm15, %xmm1
	vmovq	16(%r11), %xmm15
	vmovq	%xmm10, %xmm5
	vmovq	%xmm0, %xmm8
	vmovq	%xmm11, %xmm3
	vmulps	%xmm12, %xmm8, %xmm2
	vmovq	%xmm9, %xmm11
	vshufps	$0xe1, %xmm15, %xmm15, %xmm9
	vmulps	%xmm4, %xmm8, %xmm6
	vmulps	%xmm5, %xmm11, %xmm0
	vmovq	%xmm15, %xmm11
	vmovq	%xmm2, %xmm13
	vmovq	%xmm1, %xmm2
	vmovq	16(%r8), %xmm1
	vmovq	%xmm6, %xmm7
	vfmsubadd231ps	%xmm3, %xmm4, %xmm13
	vmovq	16(%rdi,%r9), %xmm6
	vfmaddsub132ps	%xmm3, %xmm7, %xmm12
	vmovq	%xmm0, %xmm8
	vmovq	%xmm14, %xmm3
	vfmaddsub231ps	%xmm2, %xmm3, %xmm8
	vmovshdup	%xmm1, %xmm5
	vmovsldup	%xmm1, %xmm10
	vmovq	%xmm13, %xmm13
	vmovq	%xmm5, %xmm0
	vmovq	%xmm12, %xmm12
	vmovq	%xmm8, %xmm4
	vmovq	%xmm10, %xmm3
	vmulps	%xmm11, %xmm0, %xmm8
	vsubps	%xmm4, %xmm6, %xmm7
	vaddps	%xmm6, %xmm4, %xmm14
	vmovq	%xmm9, %xmm6
	vmulps	%xmm6, %xmm0, %xmm0
	vmovq	%xmm7, %xmm1
	vmovq	%xmm14, %xmm14
	vmovq	%xmm8, %xmm2
	vfmsubadd231ps	%xmm3, %xmm6, %xmm2
	vmovq	%xmm2, %xmm4
	vmovq	%xmm0, %xmm2
	vfmaddsub132ps	%xmm3, %xmm2, %xmm11
	vsubps	%xmm4, %xmm13, %xmm15
	vmovq	%xmm15, %xmm10
	vaddps	%xmm1, %xmm10, %xmm9
	vsubps	%xmm10, %xmm1, %xmm7
	vmovq	%xmm11, %xmm11
	vaddsubps	%xmm10, %xmm1, %xmm8
	vaddps	%xmm12, %xmm11, %xmm3
	vblendps	$1, %xmm9, %xmm7, %xmm5
	vmovq	%xmm3, %xmm6
	vaddps	%xmm14, %xmm6, %xmm4
	vsubps	%xmm6, %xmm14, %xmm13
	vmovlps	%xmm4, 16(%rdi,%r9)
	vmovlps	%xmm13, 16(%rax)
	vmovlps	%xmm5, 16(%rsi)
	vmovlps	%xmm8, 16(%r8)
.L1680:
	movq	-40(%rbp), %rbx
	movq	-32(%rbp), %r12
	movq	-24(%rbp), %r13
	movq	-16(%rbp), %r14
	leave
	.cfi_restore 6
	.cfi_restore 14
	.cfi_restore 13
	.cfi_restore 12
	.cfi_restore 3
	.cfi_def_cfa 7, 8
	ret
	.p2align 4,,10
	.p2align 3
.L1682:
	ret
	.cfi_endproc
.LFE7650:
	.size	_ZN6micron4math3fft6__impl7__stageILm4ELb0EfEEvPNS0_7complexIT1_EEPKS6_mmm, .-_ZN6micron4math3fft6__impl7__stageILm4ELb0EfEEvPNS0_7complexIT1_EEPKS6_mmm
