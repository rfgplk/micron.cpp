_ZN6micron4math3fft6__impl7__stageILm4ELb0ETkNS1_10fft_scalarEdEEvPNS0_7complexIT1_EEPKS6_mmm: # @_ZN6micron4math3fft6__impl7__stageILm4ELb0ETkNS1_10fft_scalarEdEEvPNS0_7complexIT1_EEPKS6_mmm
	.cfi_startproc
# %bb.0:
	cmpq	%rcx, %r8
	jbe	.LBB38_7
# %bb.1:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	.cfi_offset %rbx, -56
	.cfi_offset %r12, -48
	.cfi_offset %r13, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	movq	%rdi, %rax
	movl	%r8d, %r9d
	subl	%ecx, %r9d
	andl	$1, %r9d
	subq	%r9, %r8
	leaq	(%rdx,%rdx,2), %rdi
	movq	%rdi, -48(%rbp)                 # 8-byte Spill
	cmpq	%r8, %rcx
	jae	.LBB38_4
# %bb.2:
	movq	%rdx, %r10
	shlq	$5, %r10
	leaq	(%rdx,%rdx,2), %r11
	shlq	$4, %r11
	movq	%rcx, %rbx
	shlq	$4, %rbx
	addq	%rax, %r11
	movq	%rdx, %r14
	shlq	$4, %r14
	leaq	(%rax,%r14), %r15
	leaq	(%rax,%r10), %r12
	addq	%rsi, %r10
	addq	%rsi, %r14
	vbroadcastf128	.LCPI38_1(%rip), %ymm0  # ymm0 = [0,0,0,2147483648,0,0,0,2147483648]
                                        # ymm0 = mem[0,1,0,1]
	movq	%rsi, %r13
	movq	%rax, %rdi
	.p2align	4
.LBB38_3:                               # =>This Inner Loop Header: Depth=1
	vmovupd	(%rdi,%rbx), %ymm1
	vmovupd	(%r13,%rbx), %ymm2
	vmovupd	(%r15,%rbx), %ymm3
	vmovddup	%ymm3, %ymm4                    # ymm4 = ymm3[0,0,2,2]
	vshufpd	$15, %ymm3, %ymm3, %ymm3        # ymm3 = ymm3[1,1,3,3]
	vshufpd	$5, %ymm2, %ymm2, %ymm5         # ymm5 = ymm2[1,0,3,2]
	vmulpd	%ymm3, %ymm5, %ymm3
	vfmaddsub231pd	%ymm4, %ymm2, %ymm3     # ymm3 = (ymm2 * ymm4) +/- ymm3
	vmovupd	(%r14,%rbx), %ymm2
	vmovupd	(%r12,%rbx), %ymm4
	vmovddup	%ymm4, %ymm5                    # ymm5 = ymm4[0,0,2,2]
	vshufpd	$15, %ymm4, %ymm4, %ymm4        # ymm4 = ymm4[1,1,3,3]
	vshufpd	$5, %ymm2, %ymm2, %ymm6         # ymm6 = ymm2[1,0,3,2]
	vmulpd	%ymm4, %ymm6, %ymm4
	vfmaddsub231pd	%ymm5, %ymm2, %ymm4     # ymm4 = (ymm2 * ymm5) +/- ymm4
	vmovupd	(%r10,%rbx), %ymm2
	vmovupd	(%r11,%rbx), %ymm5
	vmovddup	%ymm5, %ymm6                    # ymm6 = ymm5[0,0,2,2]
	vshufpd	$15, %ymm5, %ymm5, %ymm5        # ymm5 = ymm5[1,1,3,3]
	vshufpd	$5, %ymm2, %ymm2, %ymm7         # ymm7 = ymm2[1,0,3,2]
	vmulpd	%ymm5, %ymm7, %ymm5
	vfmaddsub231pd	%ymm6, %ymm2, %ymm5     # ymm5 = (ymm2 * ymm6) +/- ymm5
	vaddpd	%ymm4, %ymm1, %ymm2
	vsubpd	%ymm4, %ymm1, %ymm1
	vaddpd	%ymm5, %ymm3, %ymm4
	vsubpd	%ymm5, %ymm3, %ymm3
	vaddpd	%ymm4, %ymm2, %ymm5
	vmovupd	%ymm5, (%rdi,%rbx)
	vshufpd	$5, %ymm3, %ymm3, %ymm3         # ymm3 = ymm3[1,0,3,2]
	vsubpd	%ymm4, %ymm2, %ymm2
	vmovupd	%ymm2, (%r12,%rbx)
	vxorpd	%ymm0, %ymm3, %ymm2
	vaddpd	%ymm2, %ymm1, %ymm3
	vmovupd	%ymm3, (%r15,%rbx)
	vsubpd	%ymm2, %ymm1, %ymm1
	vmovupd	%ymm1, (%r11,%rbx)
	addq	$2, %rcx
	addq	$32, %r11
	addq	$32, %r15
	addq	$32, %r12
	addq	$32, %rdi
	addq	$32, %r13
	addq	$32, %r10
	addq	$32, %r14
	cmpq	%r8, %rcx
	jb	.LBB38_3
.LBB38_4:
	testq	%r9, %r9
	je	.LBB38_6
# %bb.5:
	shlq	$4, %r8
	leaq	(%rax,%r8), %rcx
	leaq	(%rsi,%r8), %rdi
	vmovsd	(%rax,%r8), %xmm0               # xmm0 = mem[0],zero
	vmovsd	8(%rax,%r8), %xmm1              # xmm1 = mem[0],zero
	vmovsd	(%rsi,%r8), %xmm3               # xmm3 = mem[0],zero
	vmovsd	8(%rsi,%r8), %xmm4              # xmm4 = mem[0],zero
	movq	%rdx, %rsi
	shlq	$4, %rsi
	vmovsd	(%rsi,%rcx), %xmm5              # xmm5 = mem[0],zero
	vmovsd	8(%rsi,%rcx), %xmm6             # xmm6 = mem[0],zero
	vmulsd	%xmm4, %xmm6, %xmm2
	vfmsub231sd	%xmm3, %xmm5, %xmm2     # xmm2 = (xmm5 * xmm3) - xmm2
	vmulsd	%xmm6, %xmm3, %xmm3
	vfmadd231sd	%xmm4, %xmm5, %xmm3     # xmm3 = (xmm5 * xmm4) + xmm3
	vmovsd	(%rsi,%rdi), %xmm4              # xmm4 = mem[0],zero
	vmovsd	8(%rsi,%rdi), %xmm5             # xmm5 = mem[0],zero
	shlq	$5, %rdx
	vmovsd	(%rdx,%rcx), %xmm6              # xmm6 = mem[0],zero
	vmovsd	8(%rdx,%rcx), %xmm7             # xmm7 = mem[0],zero
	vmulsd	%xmm5, %xmm7, %xmm8
	vfmsub231sd	%xmm4, %xmm6, %xmm8     # xmm8 = (xmm6 * xmm4) - xmm8
	vmulsd	%xmm7, %xmm4, %xmm4
	vfmadd231sd	%xmm5, %xmm6, %xmm4     # xmm4 = (xmm6 * xmm5) + xmm4
	vmovsd	(%rdx,%rdi), %xmm5              # xmm5 = mem[0],zero
	vmovsd	8(%rdx,%rdi), %xmm6             # xmm6 = mem[0],zero
	movq	-48(%rbp), %rdi                 # 8-byte Reload
	shlq	$4, %rdi
	vmovsd	(%rdi,%rcx), %xmm7              # xmm7 = mem[0],zero
	vmovsd	8(%rdi,%rcx), %xmm9             # xmm9 = mem[0],zero
	vmulsd	%xmm6, %xmm9, %xmm10
	vfmsub231sd	%xmm5, %xmm7, %xmm10    # xmm10 = (xmm7 * xmm5) - xmm10
	vmulsd	%xmm5, %xmm9, %xmm5
	vfmadd231sd	%xmm6, %xmm7, %xmm5     # xmm5 = (xmm7 * xmm6) + xmm5
	vaddsd	%xmm0, %xmm8, %xmm6
	vaddsd	%xmm4, %xmm1, %xmm7
	vsubsd	%xmm8, %xmm0, %xmm0
	vsubsd	%xmm4, %xmm1, %xmm1
	vaddsd	%xmm2, %xmm10, %xmm4
	vaddsd	%xmm5, %xmm3, %xmm8
	vsubsd	%xmm10, %xmm2, %xmm2
	vsubsd	%xmm5, %xmm3, %xmm3
	vaddsd	%xmm4, %xmm6, %xmm5
	vaddsd	%xmm7, %xmm8, %xmm9
	vmovsd	%xmm5, (%rax,%r8)
	vmovsd	%xmm9, 8(%rax,%r8)
	vsubsd	%xmm4, %xmm6, %xmm4
	vsubsd	%xmm8, %xmm7, %xmm5
	vmovsd	%xmm4, (%rdx,%rcx)
	vmovsd	%xmm5, 8(%rdx,%rcx)
	vaddsd	%xmm3, %xmm0, %xmm4
	vsubsd	%xmm2, %xmm1, %xmm5
	vmovsd	%xmm4, (%rsi,%rcx)
	vmovsd	%xmm5, 8(%rsi,%rcx)
	vsubsd	%xmm3, %xmm0, %xmm0
	vaddsd	%xmm2, %xmm1, %xmm1
	vmovsd	%xmm0, (%rdi,%rcx)
	vmovsd	%xmm1, 8(%rdi,%rcx)
.LBB38_6:
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	.cfi_restore %rbx
	.cfi_restore %r12
	.cfi_restore %r13
	.cfi_restore %r14
	.cfi_restore %r15
	.cfi_restore %rbp
.LBB38_7:
	vzeroupper
	retq
.Lfunc_end38:
	.size	_ZN6micron4math3fft6__impl7__stageILm4ELb0ETkNS1_10fft_scalarEdEEvPNS0_7complexIT1_EEPKS6_mmm, .Lfunc_end38-_ZN6micron4math3fft6__impl7__stageILm4ELb0ETkNS1_10fft_scalarEdEEvPNS0_7complexIT1_EEPKS6_mmm
