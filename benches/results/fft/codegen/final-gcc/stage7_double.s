_ZN6micron4math3fft6__impl7__stageILm7ELb0EdEEvPNS0_7complexIT1_EEPKS6_mmm:
.LFB7657:
	.cfi_startproc
	cmpq	%r8, %rcx
	jnb	.L1808
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rdi, %r11
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	pushq	%r15
	pushq	%r14
	.cfi_offset 15, -24
	.cfi_offset 14, -32
	movq	%r8, %r14
	pushq	%r13
	subq	%rcx, %r14
	pushq	%r12
	andl	$1, %r14d
	.cfi_offset 13, -40
	.cfi_offset 12, -48
	movq	%rdx, %r12
	pushq	%rbx
	subq	%r14, %r8
	.cfi_offset 3, -56
	movq	%rsi, %rbx
	andq	$-32, %rsp
	subq	$136, %rsp
	vbroadcastsd	.LC105(%rip), %ymm12
	vbroadcastsd	.LC103(%rip), %ymm13
	vbroadcastsd	.LC117(%rip), %ymm4
	vbroadcastsd	.LC115(%rip), %ymm5
	vbroadcastsd	.LC113(%rip), %ymm3
	vmovapd	%ymm12, 40(%rsp)
	vbroadcastsd	.LC111(%rip), %ymm6
	vbroadcastsd	.LC109(%rip), %ymm2
	vmovapd	%ymm13, 72(%rsp)
	vbroadcastsd	.LC107(%rip), %ymm7
	cmpq	%r8, %rcx
	jnb	.L1802
	leaq	(%rdx,%rdx,2), %rsi
	movq	%rdx, %r10
	leaq	(%rdx,%rdx,4), %rdi
	movq	%rdx, %r9
	movq	%rcx, %rdx
	movq	%rsi, %r13
	movq	%r12, %r15
	salq	$5, %r10
	salq	$4, %rdx
	salq	$5, %r13
	vmovapd	%ymm7, %ymm11
	vmovapd	%ymm6, %ymm10
	leaq	(%r11,%rdx), %rax
	salq	$4, %rdi
	salq	$4, %rsi
	addq	%rbx, %rdx
	vmovapd	.LC87(%rip), %ymm9
	salq	$6, %r9
	salq	$4, %r15
	vmovapd	%ymm3, %ymm14
	vmovapd	%ymm7, 104(%rsp)
	vmovapd	%ymm2, 8(%rsp)
	vmovapd	%ymm6, -24(%rsp)
	vmovapd	%ymm3, -56(%rsp)
	vmovapd	%ymm5, -88(%rsp)
	vmovapd	%ymm4, -120(%rsp)
	.p2align 4
	.p2align 3
.L1803:
	vmovdqu	(%rdx), %ymm1
	vpermilpd	$15, (%rax,%r15), %ymm0
	addq	$2, %rcx
	addq	$32, %rdx
	vmovdqu	-32(%rdx,%r15), %ymm5
	vpermilpd	$15, (%rax,%r10), %ymm6
	vpermilpd	$5, %ymm1, %ymm2
	vmovddup	(%rax,%r15), %ymm4
	vmovdqu	(%rax), %ymm8
	vmulpd	%ymm2, %ymm0, %ymm3
	vpermilpd	$5, %ymm5, %ymm7
	vmovddup	(%rax,%r10), %ymm2
	vmovddup	(%rax,%rsi), %ymm0
	vmulpd	%ymm7, %ymm6, %ymm15
	vmovdqu	-32(%rdx,%rsi), %ymm7
	vfmaddsub132pd	%ymm1, %ymm3, %ymm4
	vmovdqu	-32(%rdx,%r10), %ymm3
	vpermilpd	$15, (%rax,%rsi), %ymm1
	vfmaddsub132pd	%ymm5, %ymm15, %ymm2
	vpermilpd	$15, (%rax,%r9), %ymm15
	vpermilpd	$5, %ymm3, %ymm5
	vmulpd	%ymm5, %ymm1, %ymm6
	vpermilpd	$5, %ymm7, %ymm1
	vmulpd	%ymm1, %ymm15, %ymm5
	vpermilpd	$15, (%rax,%rdi), %ymm15
	vfmaddsub132pd	%ymm3, %ymm6, %ymm0
	vmovdqu	-32(%rdx,%r9), %ymm6
	vmovddup	(%rax,%r9), %ymm3
	vfmaddsub132pd	%ymm7, %ymm5, %ymm3
	vpermilpd	$5, %ymm6, %ymm7
	vmovddup	(%rax,%rdi), %ymm5
	vmulpd	%ymm7, %ymm15, %ymm1
	vmovdqu	-32(%rdx,%rdi), %ymm7
	vpermilpd	$5, %ymm7, %ymm15
	vfmaddsub132pd	%ymm6, %ymm1, %ymm5
	vpermilpd	$15, (%rax,%r13), %ymm1
	vmovddup	(%rax,%r13), %ymm6
	addq	$32, %rax
	vmulpd	%ymm15, %ymm1, %ymm1
	vfmaddsub132pd	%ymm7, %ymm1, %ymm6
	vaddpd	%ymm5, %ymm2, %ymm7
	vsubpd	%ymm5, %ymm2, %ymm2
	vaddpd	%ymm6, %ymm4, %ymm1
	vsubpd	%ymm6, %ymm4, %ymm4
	vaddpd	%ymm3, %ymm0, %ymm6
	vsubpd	%ymm3, %ymm0, %ymm0
	vaddpd	%ymm8, %ymm1, %ymm3
	vaddpd	%ymm7, %ymm3, %ymm5
	vmulpd	%ymm11, %ymm2, %ymm3
	vfmadd231pd	.LC108(%rip), %ymm4, %ymm3
	vaddpd	%ymm6, %ymm5, %ymm15
	vmovapd	%ymm1, %ymm5
	vfmadd132pd	%ymm13, %ymm8, %ymm5
	vfmadd231pd	%ymm14, %ymm0, %ymm3
	vmovdqu	%ymm15, -32(%rax)
	vfmadd231pd	%ymm12, %ymm7, %ymm5
	vpermilpd	$5, %ymm3, %ymm15
	vxorpd	%ymm9, %ymm15, %ymm3
	vfmadd231pd	%ymm10, %ymm6, %ymm5
	vaddpd	%ymm3, %ymm5, %ymm15
	vsubpd	%ymm3, %ymm5, %ymm5
	vmulpd	.LC114(%rip), %ymm2, %ymm3
	vmovdqu	%ymm15, -32(%rax,%r15)
	vmovdqu	%ymm5, -32(%rax,%r13)
	vmovapd	%ymm1, %ymm5
	vfmadd132pd	%ymm10, %ymm8, %ymm1
	vfmadd132pd	%ymm12, %ymm8, %ymm5
	vfmadd231pd	%ymm11, %ymm4, %ymm3
	vfmadd231pd	.LC116(%rip), %ymm0, %ymm3
	vfmadd231pd	%ymm13, %ymm7, %ymm1
	vfmadd231pd	%ymm10, %ymm7, %ymm5
	vpermilpd	$5, %ymm3, %ymm15
	vxorpd	%ymm9, %ymm15, %ymm3
	vfmadd231pd	%ymm12, %ymm6, %ymm1
	vfmadd231pd	%ymm13, %ymm6, %ymm5
	vaddpd	%ymm3, %ymm5, %ymm15
	vsubpd	%ymm3, %ymm5, %ymm5
	vmovdqu	%ymm15, -32(%rax,%r10)
	vmovdqu	%ymm5, -32(%rax,%rdi)
	vmulpd	.LC116(%rip), %ymm2, %ymm8
	vfmadd231pd	%ymm14, %ymm4, %ymm8
	vfmadd132pd	%ymm11, %ymm8, %ymm0
	vpermilpd	$5, %ymm0, %ymm4
	vxorpd	%ymm9, %ymm4, %ymm7
	vaddpd	%ymm7, %ymm1, %ymm2
	vsubpd	%ymm7, %ymm1, %ymm1
	vmovdqu	%ymm2, -32(%rax,%rsi)
	vmovdqu	%ymm1, -32(%rax,%r9)
	cmpq	%r8, %rcx
	jb	.L1803
	vmovapd	104(%rsp), %ymm7
	vmovapd	8(%rsp), %ymm2
	vmovapd	-24(%rsp), %ymm6
	vmovapd	-56(%rsp), %ymm3
	vmovapd	-88(%rsp), %ymm5
	vmovapd	-120(%rsp), %ymm4
.L1802:
	testq	%r14, %r14
	je	.L1811
	leaq	(%r8,%r12), %rsi
	movq	%r8, %rcx
	leaq	(%r8,%r12,2), %r8
	salq	$4, %rsi
	salq	$4, %rcx
	movq	%r8, %r10
	leaq	(%r8,%r12), %rdi
	leaq	(%r11,%rsi), %r13
	salq	$4, %r10
	salq	$4, %rdi
	leaq	(%r8,%r12,2), %r9
	vmovupd	(%rbx,%rcx), %xmm12
	movq	%r9, %rdx
	leaq	(%r9,%r12), %rax
	leaq	(%r11,%rcx), %r14
	vmovupd	0(%r13), %xmm13
	vmovupd	(%r11,%r10), %xmm0
	salq	$4, %rdx
	salq	$4, %rax
	vpermilpd	$1, %xmm12, %xmm11
	vpermilpd	$3, %xmm13, %xmm14
	vpermilpd	$0, %xmm13, %xmm10
	vpermilpd	$0, %xmm0, %xmm8
	vmulpd	%xmm11, %xmm14, %xmm9
	vmulpd	%xmm12, %xmm14, %xmm13
	vfmaddsub132pd	%xmm10, %xmm9, %xmm12
	vmovupd	(%rbx,%rsi), %xmm9
	vfmsubadd231pd	%xmm10, %xmm11, %xmm13
	vpermilpd	$1, %xmm9, %xmm15
	vmovapd	%xmm12, 104(%rsp)
	vpermilpd	$3, %xmm0, %xmm12
	vmulpd	%xmm15, %xmm12, %xmm11
	vmulpd	%xmm9, %xmm12, %xmm1
	vfmaddsub132pd	%xmm8, %xmm11, %xmm9
	vfmsubadd231pd	%xmm8, %xmm15, %xmm1
	vmovupd	(%rbx,%r10), %xmm8
	vmovupd	(%r11,%rdi), %xmm15
	vpermilpd	$1, %xmm8, %xmm14
	vpermilpd	$3, %xmm15, %xmm0
	vpermilpd	$0, %xmm15, %xmm10
	vmulpd	%xmm14, %xmm0, %xmm12
	vmulpd	%xmm8, %xmm0, %xmm11
	vmovupd	(%r11,%rdx), %xmm0
	vfmaddsub132pd	%xmm10, %xmm12, %xmm8
	vfmsubadd231pd	%xmm10, %xmm14, %xmm11
	vmovupd	(%rbx,%rdi), %xmm10
	vpermilpd	$0, %xmm0, %xmm14
	vpermilpd	$3, %xmm0, %xmm0
	vmulpd	%xmm10, %xmm0, %xmm12
	vpermilpd	$1, %xmm10, %xmm15
	vfmsubadd231pd	%xmm14, %xmm15, %xmm12
	vmulpd	%xmm15, %xmm0, %xmm15
	vsubpd	%xmm12, %xmm11, %xmm11
	vfmaddsub231pd	%xmm14, %xmm10, %xmm15
	vmovupd	(%rbx,%rdx), %xmm10
	movq	%r12, %rdx
	negq	%rdx
	vpermilpd	$1, %xmm10, %xmm14
	movq	%rdx, %rcx
	salq	$5, %rdx
	salq	$6, %rcx
	vaddpd	%xmm8, %xmm15, %xmm0
	vmovupd	(%r11,%rax), %xmm8
	vpermilpd	$0, %xmm8, %xmm12
	vpermilpd	$3, %xmm8, %xmm8
	vmulpd	%xmm10, %xmm8, %xmm15
	vfmsubadd231pd	%xmm12, %xmm14, %xmm15
	vmulpd	%xmm14, %xmm8, %xmm14
	vsubpd	%xmm15, %xmm1, %xmm1
	vfmaddsub231pd	%xmm12, %xmm10, %xmm14
	vaddpd	%xmm9, %xmm14, %xmm8
	vmovupd	(%rbx,%rax), %xmm9
	leaq	(%r9,%r12,2), %rbx
	salq	$4, %rbx
	vmovupd	(%r11,%rbx), %xmm10
	vpermilpd	$1, %xmm9, %xmm15
	leaq	(%r12,%r12,2), %r11
	salq	$4, %r12
	movq	%r11, %r15
	salq	$4, %r11
	vpermilpd	$0, %xmm10, %xmm14
	vpermilpd	$3, %xmm10, %xmm10
	salq	$5, %r15
	vmulpd	%xmm9, %xmm10, %xmm12
	vfmsubadd231pd	%xmm14, %xmm15, %xmm12
	vsubpd	%xmm12, %xmm13, %xmm13
	vmulpd	%xmm2, %xmm13, %xmm2
	vfmadd231pd	%xmm7, %xmm1, %xmm2
	vfmadd231pd	%xmm3, %xmm11, %xmm2
	vmulpd	%xmm3, %xmm13, %xmm3
	vfmadd231pd	%xmm4, %xmm1, %xmm3
	vfmadd231pd	%xmm7, %xmm11, %xmm3
	vmulpd	%xmm7, %xmm13, %xmm7
	vfmadd132pd	%xmm5, %xmm7, %xmm1
	vmovapd	72(%rsp), %xmm7
	vmulpd	%xmm15, %xmm10, %xmm5
	vfmadd132pd	%xmm11, %xmm1, %xmm4
	vfmaddsub132pd	%xmm14, %xmm5, %xmm9
	vmovapd	40(%rsp), %xmm5
	vaddpd	104(%rsp), %xmm9, %xmm1
	vaddpd	(%r14), %xmm1, %xmm11
	vaddpd	%xmm8, %xmm11, %xmm9
	vmovapd	%xmm1, %xmm14
	vfmadd213pd	(%r14), %xmm7, %xmm14
	vaddpd	%xmm0, %xmm9, %xmm15
	vmovapd	%xmm1, %xmm9
	vfmadd213pd	(%r14), %xmm5, %xmm1
	vfmadd213pd	(%r14), %xmm6, %xmm9
	vfmadd231pd	%xmm5, %xmm8, %xmm14
	vmovupd	%xmm15, (%r14)
	addq	%r15, %r14
	leaq	(%r14,%rcx), %rsi
	addq	%rsi, %r11
	vfmadd231pd	%xmm7, %xmm8, %xmm9
	vfmadd132pd	%xmm6, %xmm1, %xmm8
	vfmadd231pd	%xmm6, %xmm0, %xmm14
	vfmadd231pd	%xmm5, %xmm0, %xmm9
	vfmadd132pd	%xmm7, %xmm8, %xmm0
	vaddpd	%xmm14, %xmm2, %xmm12
	vsubpd	%xmm2, %xmm14, %xmm11
	vaddsubpd	%xmm2, %xmm14, %xmm2
	vaddpd	%xmm0, %xmm4, %xmm8
	vsubpd	%xmm4, %xmm0, %xmm1
	vmovsd	%xmm12, %xmm11, %xmm6
	vmovupd	%xmm6, 0(%r13)
	vaddsubpd	%xmm3, %xmm9, %xmm10
	vaddpd	%xmm9, %xmm3, %xmm13
	vmovupd	%xmm2, (%r14)
	vaddsubpd	%xmm4, %xmm0, %xmm4
	vsubpd	%xmm3, %xmm9, %xmm3
	vmovsd	%xmm8, %xmm1, %xmm15
	vmovupd	%xmm15, (%r14,%rcx)
	leaq	(%rdx,%r11), %r14
	vmovupd	%xmm4, (%r11)
	vmovsd	%xmm13, %xmm3, %xmm0
	vmovupd	%xmm0, (%rdx,%r11)
	vmovupd	%xmm10, (%r12,%r14)
	vzeroupper
	leaq	-40(%rbp), %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
.L1808:
	.cfi_restore 3
	.cfi_restore 6
	.cfi_restore 12
	.cfi_restore 13
	.cfi_restore 14
	.cfi_restore 15
	ret
.L1811:
	.cfi_def_cfa 6, 16
	.cfi_offset 3, -56
	.cfi_offset 6, -16
	.cfi_offset 12, -48
	.cfi_offset 13, -40
	.cfi_offset 14, -32
	.cfi_offset 15, -24
	vzeroupper
	leaq	-40(%rbp), %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE7657:
	.size	_ZN6micron4math3fft6__impl7__stageILm7ELb0EdEEvPNS0_7complexIT1_EEPKS6_mmm, .-_ZN6micron4math3fft6__impl7__stageILm7ELb0EdEEvPNS0_7complexIT1_EEPKS6_mmm
