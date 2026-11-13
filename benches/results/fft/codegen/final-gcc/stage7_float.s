_ZN6micron4math3fft6__impl7__stageILm7ELb0EfEEvPNS0_7complexIT1_EEPKS6_mmm:
.LFB7652:
	.cfi_startproc
	cmpq	%r8, %rcx
	jnb	.L1716
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%r8, %rax
	movq	%rcx, %r9
	subq	%rcx, %rax
	andl	$3, %eax
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	pushq	%r15
	subq	%rax, %r8
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	andq	$-32, %rsp
	.cfi_offset 15, -24
	.cfi_offset 14, -32
	.cfi_offset 13, -40
	.cfi_offset 12, -48
	.cfi_offset 3, -56
	movq	%rax, -8(%rsp)
	cmpq	%r8, %rcx
	jnb	.L1706
	movq	%rdx, %r12
	leaq	(%rdx,%rdx,4), %r11
	leaq	(%rdx,%rdx,2), %r10
	movq	%rdx, %rbx
	leaq	(%r9,%rdx), %r13
	salq	$4, %r12
	salq	$3, %r11
	leaq	(%rsi,%rcx,8), %rcx
	vbroadcastss	.LC53(%rip), %ymm13
	salq	$3, %r10
	vbroadcastss	.LC55(%rip), %ymm12
	salq	$5, %rbx
	vbroadcastss	.LC57(%rip), %ymm11
	vmovaps	.LC29(%rip), %ymm9
	leaq	(%rdi,%r13,8), %r14
	vbroadcastss	.LC61(%rip), %ymm10
	vbroadcastss	.LC63(%rip), %ymm14
	.p2align 4
	.p2align 3
.L1707:
	vmovdqu	(%r14), %ymm0
	vmovdqu	(%rcx), %ymm2
	vmovdqu	(%r14,%rdx,8), %ymm7
	vmovdqu	(%rcx,%rdx,8), %ymm6
	addq	$32, %rcx
	vmovshdup	%ymm0, %ymm4
	vmovsldup	%ymm0, %ymm1
	vpermilps	$177, %ymm2, %ymm3
	vmovdqu	(%rdi,%r9,8), %ymm8
	vmovshdup	%ymm7, %ymm15
	vpermilps	$177, %ymm6, %ymm0
	vmulps	%ymm3, %ymm4, %ymm5
	vmovdqu	(%r14,%r12), %ymm3
	vmulps	%ymm0, %ymm15, %ymm4
	vmovsldup	%ymm3, %ymm0
	vfmaddsub132ps	%ymm2, %ymm5, %ymm1
	vmovsldup	%ymm7, %ymm2
	vmovshdup	%ymm3, %ymm7
	vfmaddsub132ps	%ymm6, %ymm4, %ymm2
	vmovdqu	-32(%rcx,%r12), %ymm6
	vmovdqu	-32(%rcx,%r10), %ymm4
	vpermilps	$177, %ymm6, %ymm5
	vmulps	%ymm5, %ymm7, %ymm15
	vpermilps	$177, %ymm4, %ymm7
	vfmaddsub132ps	%ymm6, %ymm15, %ymm0
	vmovdqu	(%r14,%r10), %ymm6
	vmovshdup	%ymm6, %ymm3
	vmovsldup	%ymm6, %ymm5
	vmovdqu	-32(%rcx,%rbx), %ymm6
	vmulps	%ymm7, %ymm3, %ymm15
	vmovdqu	(%r14,%rbx), %ymm3
	vpermilps	$177, %ymm6, %ymm7
	vfmaddsub132ps	%ymm4, %ymm15, %ymm5
	vmovshdup	%ymm3, %ymm15
	vmovsldup	%ymm3, %ymm4
	vmulps	%ymm7, %ymm15, %ymm3
	vmovdqu	(%r14,%r11), %ymm15
	vmovdqu	-32(%rcx,%r11), %ymm7
	vfmaddsub132ps	%ymm6, %ymm3, %ymm4
	vmovshdup	%ymm15, %ymm3
	vmovsldup	%ymm15, %ymm6
	vpermilps	$177, %ymm7, %ymm15
	vmulps	%ymm15, %ymm3, %ymm3
	vfmaddsub132ps	%ymm7, %ymm3, %ymm6
	vaddps	%ymm6, %ymm1, %ymm7
	vsubps	%ymm6, %ymm1, %ymm1
	vaddps	%ymm4, %ymm2, %ymm6
	vsubps	%ymm4, %ymm2, %ymm2
	vaddps	%ymm5, %ymm0, %ymm4
	vsubps	%ymm5, %ymm0, %ymm0
	vaddps	%ymm8, %ymm7, %ymm5
	vaddps	%ymm6, %ymm5, %ymm15
	vmovaps	%ymm7, %ymm5
	vfmadd132ps	%ymm13, %ymm8, %ymm5
	vaddps	%ymm4, %ymm15, %ymm3
	vmulps	%ymm11, %ymm2, %ymm15
	vfmadd231ps	.LC58(%rip), %ymm1, %ymm15
	vfmadd231ps	%ymm12, %ymm6, %ymm5
	vmovdqu	%ymm3, (%rdi,%r9,8)
	addq	$4, %r9
	vfmadd231ps	%ymm14, %ymm0, %ymm15
	vfmadd231ps	%ymm10, %ymm4, %ymm5
	vpermilps	$177, %ymm15, %ymm3
	vxorps	%ymm9, %ymm3, %ymm3
	vaddps	%ymm3, %ymm5, %ymm15
	vsubps	%ymm3, %ymm5, %ymm5
	vmulps	.LC64(%rip), %ymm2, %ymm3
	vmovdqu	%ymm15, (%r14)
	vmovdqu	%ymm5, (%r14,%r11)
	vmovaps	%ymm7, %ymm5
	vfmadd132ps	%ymm10, %ymm8, %ymm7
	vfmadd132ps	%ymm12, %ymm8, %ymm5
	vfmadd231ps	%ymm11, %ymm1, %ymm3
	vmulps	.LC66(%rip), %ymm2, %ymm8
	vfmadd231ps	.LC66(%rip), %ymm0, %ymm3
	vfmadd231ps	%ymm10, %ymm6, %ymm5
	vfmadd132ps	%ymm13, %ymm7, %ymm6
	vfmadd132ps	%ymm14, %ymm8, %ymm1
	vpermilps	$177, %ymm3, %ymm15
	vxorps	%ymm9, %ymm15, %ymm3
	vfmadd231ps	%ymm13, %ymm4, %ymm5
	vfmadd132ps	%ymm12, %ymm6, %ymm4
	vfmadd132ps	%ymm11, %ymm1, %ymm0
	vaddps	%ymm3, %ymm5, %ymm15
	vsubps	%ymm3, %ymm5, %ymm5
	vpermilps	$177, %ymm0, %ymm7
	vxorps	%ymm9, %ymm7, %ymm6
	vaddps	%ymm6, %ymm4, %ymm1
	vsubps	%ymm6, %ymm4, %ymm2
	vmovdqu	%ymm15, (%r14,%rdx,8)
	addq	$32, %r14
	vmovdqu	%ymm5, -32(%r14,%rbx)
	vmovdqu	%ymm1, -32(%r14,%r12)
	vmovdqu	%ymm2, -32(%r14,%r10)
	cmpq	%r8, %r9
	jb	.L1707
	vzeroupper
.L1706:
	cmpq	$0, -8(%rsp)
	je	.L1714
	leaq	(%r8,%rdx), %rcx
	leaq	0(,%r8,8), %r11
	vmovaps	.LC77(%rip), %xmm15
	vmovaps	.LC81(%rip), %xmm14
	leaq	0(,%rdx,4), %r8
	leaq	0(,%rcx,8), %r14
	vmovaps	.LC82(%rip), %xmm13
	vmovaps	.LC83(%rip), %xmm12
	addq	%r8, %rcx
	leaq	(%rdi,%r11), %rbx
	leaq	(%rdi,%r14), %r10
	addq	%rsi, %r11
	leaq	0(,%rcx,8), %rax
	addq	%rdx, %rcx
	addq	%rsi, %r14
	leaq	(%rdi,%rcx,8), %r9
	subq	%r8, %rcx
	leaq	(%rsi,%rax), %r13
	addq	%rdi, %rax
	leaq	0(,%rcx,8), %r15
	leaq	(%rcx,%rdx,2), %rcx
	leaq	0(,%rcx,8), %r12
	subq	%rdx, %rcx
	leaq	(%rdi,%r15), %r8
	addq	%rsi, %r15
	leaq	0(,%rcx,8), %rdx
	movq	%r12, -16(%rsp)
	addq	%rsi, %r12
	leaq	(%rdi,%rdx), %rcx
	addq	%rdx, %rsi
	addq	-16(%rsp), %rdi
	xorl	%edx, %edx
.L1708:
	vmovq	(%r10,%rdx,8), %xmm9
	vmovq	(%r11,%rdx,8), %xmm11
	vmovshdup	%xmm9, %xmm4
	vmovsldup	%xmm9, %xmm3
	vshufps	$0xe1, %xmm11, %xmm11, %xmm10
	vmovq	(%r8,%rdx,8), %xmm9
	vmovq	%xmm10, %xmm6
	vmovq	%xmm11, %xmm5
	vmovq	(%r14,%rdx,8), %xmm11
	vshufps	$0xe1, %xmm11, %xmm11, %xmm10
	vmovq	%xmm4, %xmm0
	vmovq	%xmm3, %xmm7
	vmulps	%xmm5, %xmm0, %xmm8
	vmovshdup	%xmm9, %xmm3
	vmulps	%xmm6, %xmm0, %xmm1
	vmovq	%xmm3, %xmm0
	vmovq	(%r15,%rdx,8), %xmm3
	vmovq	%xmm8, %xmm4
	vmovsldup	%xmm9, %xmm8
	vmovq	%xmm1, %xmm2
	vfmsubadd231ps	%xmm7, %xmm6, %xmm4
	vfmaddsub132ps	%xmm7, %xmm2, %xmm5
	vmovq	%xmm11, %xmm7
	vmovq	%xmm10, %xmm2
	vmulps	%xmm7, %xmm0, %xmm6
	vmulps	%xmm2, %xmm0, %xmm10
	vmovq	(%rcx,%rdx,8), %xmm0
	vmovq	%xmm8, %xmm1
	vmovsldup	%xmm0, %xmm8
	vmovq	%xmm4, %xmm4
	vmovq	%xmm5, %xmm5
	vmovq	%xmm6, %xmm11
	vmovq	%xmm3, %xmm6
	vmovq	%xmm10, %xmm9
	vfmsubadd231ps	%xmm1, %xmm2, %xmm11
	vshufps	$0xe1, %xmm3, %xmm3, %xmm2
	vfmaddsub132ps	%xmm1, %xmm9, %xmm7
	vmovshdup	%xmm0, %xmm1
	vmovq	%xmm2, %xmm2
	vmovq	%xmm8, %xmm8
	vmovq	%xmm1, %xmm10
	vmovq	%xmm11, %xmm11
	vmulps	%xmm6, %xmm10, %xmm9
	vmovq	%xmm7, %xmm7
	vmulps	%xmm2, %xmm10, %xmm0
	vmovq	%xmm9, %xmm3
	vfmsubadd231ps	%xmm8, %xmm2, %xmm3
	vmovq	(%rdi,%rdx,8), %xmm2
	vmovq	%xmm0, %xmm1
	vfmaddsub132ps	%xmm8, %xmm1, %xmm6
	vmovq	(%rsi,%rdx,8), %xmm8
	vmovshdup	%xmm2, %xmm0
	vmovsldup	%xmm2, %xmm9
	vshufps	$0xe1, %xmm8, %xmm8, %xmm10
	vmovq	%xmm8, %xmm8
	vmovq	%xmm10, %xmm10
	vmovq	%xmm3, %xmm3
	vmovq	%xmm0, %xmm0
	vmovq	%xmm9, %xmm9
	vmovq	%xmm6, %xmm6
	vmulps	%xmm8, %xmm0, %xmm1
	vmulps	%xmm10, %xmm0, %xmm0
	vmovq	%xmm1, %xmm2
	vfmsubadd231ps	%xmm9, %xmm10, %xmm2
	vmovq	%xmm0, %xmm10
	vfmaddsub132ps	%xmm9, %xmm10, %xmm8
	vmovq	(%rax,%rdx,8), %xmm10
	vmovq	%xmm2, %xmm2
	vmovq	%xmm8, %xmm9
	vsubps	%xmm2, %xmm3, %xmm1
	vmovq	(%r12,%rdx,8), %xmm2
	vaddps	%xmm6, %xmm9, %xmm0
	vmovshdup	%xmm10, %xmm6
	vshufps	$0xe1, %xmm2, %xmm2, %xmm3
	vmovsldup	%xmm10, %xmm9
	vmovq	%xmm2, %xmm8
	vmovq	%xmm1, %xmm1
	vmovq	%xmm0, %xmm0
	vmovq	%xmm6, %xmm2
	vmulps	%xmm8, %xmm2, %xmm10
	vmovq	%xmm9, %xmm9
	vmovq	%xmm10, %xmm6
	vmovq	%xmm3, %xmm10
	vmulps	%xmm10, %xmm2, %xmm2
	vfmsubadd231ps	%xmm9, %xmm10, %xmm6
	vmovq	%xmm2, %xmm10
	vmovq	%xmm6, %xmm6
	vfmaddsub132ps	%xmm9, %xmm10, %xmm8
	vsubps	%xmm6, %xmm11, %xmm3
	vmovq	(%r9,%rdx,8), %xmm11
	vmovq	0(%r13,%rdx,8), %xmm6
	vshufps	$0xe1, %xmm6, %xmm6, %xmm10
	vmovq	%xmm6, %xmm6
	vmovq	%xmm3, %xmm3
	vmovq	%xmm8, %xmm9
	vmovsldup	%xmm11, %xmm8
	vaddps	%xmm7, %xmm9, %xmm2
	vmovshdup	%xmm11, %xmm7
	vmovq	%xmm8, %xmm8
	vmovq	%xmm2, %xmm2
	vmovq	%xmm7, %xmm7
	vmulps	%xmm6, %xmm7, %xmm9
	vmovq	%xmm9, %xmm11
	vmovq	%xmm10, %xmm9
	vmulps	%xmm9, %xmm7, %xmm7
	vfmsubadd231ps	%xmm8, %xmm9, %xmm11
	vmovq	%xmm11, %xmm11
	vmovq	%xmm7, %xmm9
	vmovq	(%rbx,%rdx,8), %xmm7
	vsubps	%xmm11, %xmm4, %xmm10
	vfmaddsub132ps	%xmm8, %xmm9, %xmm6
	vmovq	%xmm10, %xmm10
	vmulps	.LC76(%rip), %xmm10, %xmm11
	vmovq	%xmm6, %xmm6
	vaddps	%xmm5, %xmm6, %xmm8
	vmovq	%xmm11, %xmm4
	vmulps	.LC78(%rip), %xmm10, %xmm11
	vfmadd231ps	%xmm15, %xmm3, %xmm4
	vmulps	%xmm15, %xmm10, %xmm10
	vmovq	%xmm11, %xmm11
	vmovq	%xmm4, %xmm4
	vfmadd231ps	.LC79(%rip), %xmm3, %xmm11
	vmovq	%xmm10, %xmm10
	vfmadd132ps	.LC80(%rip), %xmm10, %xmm3
	vfmadd231ps	.LC78(%rip), %xmm1, %xmm4
	vmovq	%xmm11, %xmm11
	vfmadd231ps	%xmm15, %xmm1, %xmm11
	vmovq	%xmm3, %xmm3
	vmovq	%xmm4, %xmm4
	vfmadd132ps	.LC79(%rip), %xmm3, %xmm1
	vmovq	%xmm8, %xmm3
	vmovaps	%xmm3, %xmm8
	vaddps	%xmm7, %xmm3, %xmm10
	vfmadd132ps	%xmm14, %xmm7, %xmm8
	vmovq	%xmm10, %xmm9
	vmovq	%xmm11, %xmm11
	vaddps	%xmm2, %xmm9, %xmm5
	vmovq	%xmm1, %xmm1
	vmovq	%xmm8, %xmm10
	vfmadd231ps	%xmm13, %xmm2, %xmm10
	vmovq	%xmm5, %xmm6
	vaddps	%xmm0, %xmm6, %xmm6
	vmovlps	%xmm6, (%rbx,%rdx,8)
	vmovq	%xmm10, %xmm9
	vfmadd231ps	%xmm12, %xmm0, %xmm9
	vmovq	%xmm9, %xmm5
	vmovaps	%xmm3, %xmm9
	vfmadd132ps	%xmm12, %xmm7, %xmm9
	vfmadd132ps	%xmm13, %xmm7, %xmm3
	vaddps	%xmm5, %xmm4, %xmm10
	vsubps	%xmm4, %xmm5, %xmm8
	vaddsubps	%xmm4, %xmm5, %xmm5
	vblendps	$1, %xmm10, %xmm8, %xmm8
	vmovq	%xmm9, %xmm4
	vmovq	%xmm3, %xmm7
	vmovlps	%xmm8, (%r10,%rdx,8)
	vfmadd231ps	%xmm14, %xmm2, %xmm4
	vfmadd132ps	%xmm12, %xmm7, %xmm2
	vmovlps	%xmm5, (%r9,%rdx,8)
	vmovq	%xmm4, %xmm10
	vmovq	%xmm2, %xmm3
	vfmadd231ps	%xmm13, %xmm0, %xmm10
	vfmadd132ps	%xmm14, %xmm3, %xmm0
	vmovq	%xmm10, %xmm4
	vmovq	%xmm0, %xmm0
	vaddsubps	%xmm11, %xmm4, %xmm9
	vaddps	%xmm4, %xmm11, %xmm10
	vaddps	%xmm0, %xmm1, %xmm6
	vsubps	%xmm11, %xmm4, %xmm11
	vsubps	%xmm1, %xmm0, %xmm2
	vaddsubps	%xmm1, %xmm0, %xmm5
	vblendps	$1, %xmm10, %xmm11, %xmm4
	vblendps	$1, %xmm6, %xmm2, %xmm8
	vmovlps	%xmm8, (%r8,%rdx,8)
	vmovlps	%xmm5, (%rax,%rdx,8)
	vmovlps	%xmm4, (%rcx,%rdx,8)
	vmovlps	%xmm9, (%rdi,%rdx,8)
	addq	$1, %rdx
	cmpq	%rdx, -8(%rsp)
	jne	.L1708
.L1714:
	leaq	-40(%rbp), %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
.L1716:
	.cfi_restore 3
	.cfi_restore 6
	.cfi_restore 12
	.cfi_restore 13
	.cfi_restore 14
	.cfi_restore 15
	ret
	.cfi_endproc
.LFE7652:
	.size	_ZN6micron4math3fft6__impl7__stageILm7ELb0EfEEvPNS0_7complexIT1_EEPKS6_mmm, .-_ZN6micron4math3fft6__impl7__stageILm7ELb0EfEEvPNS0_7complexIT1_EEPKS6_mmm
