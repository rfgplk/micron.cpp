_ZN6micron4math3fft6__impl7__stageILm4ELb0EdEEvPNS0_7complexIT1_EEPKS6_mmm:
.LFB7655:
	.cfi_startproc
	cmpq	%r8, %rcx
	jnb	.L1783
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rdi, %r10
	movq	%rsi, %r11
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
	jnb	.L1777
	movq	%rdx, %rdi
	leaq	(%rdx,%rdx,2), %r9
	movq	%rcx, %rdx
	vmovapd	.LC87(%rip), %ymm3
	salq	$4, %rdx
	salq	$5, %rdi
	leaq	(%r10,%rdx), %rax
	addq	%rsi, %rdx
	movq	%rbx, %rsi
	salq	$4, %r9
	salq	$4, %rsi
	.p2align 4
	.p2align 3
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
	vzeroupper
.L1777:
	testq	%r12, %r12
	je	.L1781
	leaq	(%r8,%rbx), %rdi
	movq	%r8, %rcx
	leaq	(%r8,%rbx,2), %r8
	salq	$4, %rdi
	movq	%r8, %rdx
	salq	$4, %rcx
	addq	%rbx, %r8
	salq	$4, %rdx
	leaq	(%r10,%rdi), %r9
	leaq	(%r10,%rcx), %r12
	salq	$4, %r8
	vmovupd	(%r10,%rdx), %xmm0
	vmovupd	(%r11,%rdi), %xmm5
	vmovupd	(%r9), %xmm14
	vmovupd	(%r11,%rcx), %xmm13
	vpermilpd	$3, %xmm0, %xmm8
	vpermilpd	$1, %xmm5, %xmm6
	vpermilpd	$0, %xmm0, %xmm7
	vmovupd	(%r12), %xmm12
	vmulpd	%xmm6, %xmm8, %xmm9
	vpermilpd	$3, %xmm14, %xmm2
	vpermilpd	$1, %xmm13, %xmm3
	vmulpd	%xmm13, %xmm2, %xmm4
	vpermilpd	$0, %xmm14, %xmm15
	vmulpd	%xmm3, %xmm2, %xmm1
	vfmaddsub132pd	%xmm7, %xmm9, %xmm5
	vfmsubadd231pd	%xmm15, %xmm3, %xmm4
	vmovupd	(%r10,%r8), %xmm3
	movq	%rbx, %r10
	salq	$4, %rbx
	vfmaddsub132pd	%xmm15, %xmm1, %xmm13
	salq	$5, %r10
	vpermilpd	$3, %xmm3, %xmm2
	vpermilpd	$0, %xmm3, %xmm15
	vsubpd	%xmm5, %xmm12, %xmm10
	vaddpd	%xmm12, %xmm5, %xmm11
	vmovupd	(%r11,%rdx), %xmm12
	vmulpd	%xmm12, %xmm2, %xmm1
	vpermilpd	$1, %xmm12, %xmm14
	vmulpd	%xmm14, %xmm2, %xmm0
	vfmsubadd231pd	%xmm15, %xmm14, %xmm1
	vfmaddsub132pd	%xmm15, %xmm0, %xmm12
	vsubpd	%xmm1, %xmm4, %xmm4
	vaddpd	%xmm13, %xmm12, %xmm13
	vaddsubpd	%xmm4, %xmm10, %xmm8
	vaddpd	%xmm10, %xmm4, %xmm5
	vaddpd	%xmm11, %xmm13, %xmm6
	vsubpd	%xmm4, %xmm10, %xmm7
	vsubpd	%xmm13, %xmm11, %xmm9
	vmovupd	%xmm6, (%r12)
	addq	%r10, %r12
	vmovsd	%xmm5, %xmm7, %xmm10
	vmovupd	%xmm9, (%r12)
	vmovupd	%xmm10, (%r9)
	vmovupd	%xmm8, (%r12,%rbx)
.L1781:
	popq	%rbx
	popq	%r12
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.p2align 4,,10
	.p2align 3
.L1783:
	.cfi_restore 3
	.cfi_restore 6
	.cfi_restore 12
	ret
	.cfi_endproc
.LFE7655:
	.size	_ZN6micron4math3fft6__impl7__stageILm4ELb0EdEEvPNS0_7complexIT1_EEPKS6_mmm, .-_ZN6micron4math3fft6__impl7__stageILm4ELb0EdEEvPNS0_7complexIT1_EEPKS6_mmm
