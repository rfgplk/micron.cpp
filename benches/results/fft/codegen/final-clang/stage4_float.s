_ZN6micron4math3fft6__impl7__stageILm4ELb0ETkNS1_10fft_scalarEfEEvPNS0_7complexIT1_EEPKS6_mmm: # @_ZN6micron4math3fft6__impl7__stageILm4ELb0ETkNS1_10fft_scalarEfEEvPNS0_7complexIT1_EEPKS6_mmm
	.cfi_startproc
# %bb.0:
	cmpq	%rcx, %r8
	jbe	.LBB30_9
# %bb.1:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	pushq	%r14
	pushq	%rbx
	.cfi_offset %rbx, -32
	.cfi_offset %r14, -24
	movl	%r8d, %eax
	subl	%ecx, %eax
	andl	$3, %eax
	subq	%rax, %r8
	cmpq	%r8, %rcx
	jae	.LBB30_4
# %bb.2:
	movq	%rdx, %r9
	shlq	$4, %r9
	leaq	(%rdx,%rdx,2), %r10
	leaq	(%rdi,%r10,8), %r10
	leaq	(%rdi,%rdx,8), %r11
	leaq	(%rdi,%r9), %rbx
	addq	%rsi, %r9
	leaq	(%rsi,%rdx,8), %r14
	vbroadcastsd	.LCPI30_1(%rip), %ymm0  # ymm0 = [0,2147483648,0,2147483648,0,2147483648,0,2147483648]
	.p2align	4
.LBB30_3:                               # =>This Inner Loop Header: Depth=1
	vmovups	(%rdi,%rcx,8), %ymm1
	vmovups	(%rsi,%rcx,8), %ymm2
	vmovups	(%r11,%rcx,8), %ymm3
	vmovshdup	%ymm3, %ymm4            # ymm4 = ymm3[1,1,3,3,5,5,7,7]
	vmovsldup	%ymm3, %ymm3            # ymm3 = ymm3[0,0,2,2,4,4,6,6]
	vshufps	$177, %ymm2, %ymm2, %ymm5       # ymm5 = ymm2[1,0,3,2,5,4,7,6]
	vmulps	%ymm4, %ymm5, %ymm4
	vfmaddsub231ps	%ymm3, %ymm2, %ymm4     # ymm4 = (ymm2 * ymm3) +/- ymm4
	vmovups	(%r14,%rcx,8), %ymm2
	vmovups	(%rbx,%rcx,8), %ymm3
	vmovshdup	%ymm3, %ymm5            # ymm5 = ymm3[1,1,3,3,5,5,7,7]
	vmovsldup	%ymm3, %ymm3            # ymm3 = ymm3[0,0,2,2,4,4,6,6]
	vshufps	$177, %ymm2, %ymm2, %ymm6       # ymm6 = ymm2[1,0,3,2,5,4,7,6]
	vmulps	%ymm5, %ymm6, %ymm5
	vfmaddsub231ps	%ymm3, %ymm2, %ymm5     # ymm5 = (ymm2 * ymm3) +/- ymm5
	vmovups	(%r9,%rcx,8), %ymm2
	vmovups	(%r10,%rcx,8), %ymm3
	vmovshdup	%ymm3, %ymm6            # ymm6 = ymm3[1,1,3,3,5,5,7,7]
	vmovsldup	%ymm3, %ymm3            # ymm3 = ymm3[0,0,2,2,4,4,6,6]
	vshufps	$177, %ymm2, %ymm2, %ymm7       # ymm7 = ymm2[1,0,3,2,5,4,7,6]
	vmulps	%ymm6, %ymm7, %ymm6
	vfmaddsub231ps	%ymm3, %ymm2, %ymm6     # ymm6 = (ymm2 * ymm3) +/- ymm6
	vaddps	%ymm5, %ymm1, %ymm2
	vsubps	%ymm5, %ymm1, %ymm1
	vaddps	%ymm6, %ymm4, %ymm3
	vsubps	%ymm6, %ymm4, %ymm4
	vshufps	$177, %ymm4, %ymm4, %ymm4       # ymm4 = ymm4[1,0,3,2,5,4,7,6]
	vaddps	%ymm3, %ymm2, %ymm5
	vmovups	%ymm5, (%rdi,%rcx,8)
	vxorps	%ymm0, %ymm4, %ymm4
	vsubps	%ymm3, %ymm2, %ymm2
	vmovups	%ymm2, (%rbx,%rcx,8)
	vaddps	%ymm4, %ymm1, %ymm2
	vmovups	%ymm2, (%r11,%rcx,8)
	vsubps	%ymm4, %ymm1, %ymm1
	vmovups	%ymm1, (%r10,%rcx,8)
	addq	$4, %rcx
	cmpq	%r8, %rcx
	jb	.LBB30_3
.LBB30_4:
	testq	%rax, %rax
	je	.LBB30_8
# %bb.5:
	movq	%rdx, %r9
	shlq	$4, %r9
	leaq	(,%rdx,8), %rcx
	leaq	(%rcx,%rcx,2), %rcx
	leaq	(%rdi,%r8,8), %r10
	vmovsd	(%rdi,%r8,8), %xmm1             # xmm1 = mem[0],zero
	leaq	(%rsi,%r8,8), %r11
	vmovsd	(%rsi,%r8,8), %xmm0             # xmm0 = mem[0],zero
	vmovsd	(%r10,%rdx,8), %xmm3            # xmm3 = mem[0],zero
	vmovshdup	%xmm3, %xmm4            # xmm4 = xmm3[1,1,3,3]
	vmovshdup	%xmm0, %xmm5            # xmm5 = xmm0[1,1,3,3]
	vmulss	%xmm5, %xmm4, %xmm2
	vfmsub231ss	%xmm0, %xmm3, %xmm2     # xmm2 = (xmm3 * xmm0) - xmm2
	vmulss	%xmm4, %xmm0, %xmm0
	vfmadd231ss	%xmm5, %xmm3, %xmm0     # xmm0 = (xmm3 * xmm5) + xmm0
	vmovsd	(%r11,%rdx,8), %xmm3            # xmm3 = mem[0],zero
	vmovsd	(%r9,%r10), %xmm4               # xmm4 = mem[0],zero
	vmovshdup	%xmm4, %xmm5            # xmm5 = xmm4[1,1,3,3]
	vmovshdup	%xmm3, %xmm6            # xmm6 = xmm3[1,1,3,3]
	vmulss	%xmm6, %xmm5, %xmm7
	vfmsub231ss	%xmm3, %xmm4, %xmm7     # xmm7 = (xmm4 * xmm3) - xmm7
	vmulss	%xmm5, %xmm3, %xmm3
	vfmadd231ss	%xmm6, %xmm4, %xmm3     # xmm3 = (xmm4 * xmm6) + xmm3
	vmovsd	(%r9,%r11), %xmm4               # xmm4 = mem[0],zero
	vmovsd	(%r10,%rcx), %xmm5              # xmm5 = mem[0],zero
	vmovshdup	%xmm5, %xmm6            # xmm6 = xmm5[1,1,3,3]
	vmovshdup	%xmm4, %xmm8            # xmm8 = xmm4[1,1,3,3]
	vmulss	%xmm6, %xmm8, %xmm9
	vfmsub231ss	%xmm4, %xmm5, %xmm9     # xmm9 = (xmm5 * xmm4) - xmm9
	vmulss	%xmm6, %xmm4, %xmm4
	vfmadd231ss	%xmm8, %xmm5, %xmm4     # xmm4 = (xmm5 * xmm8) + xmm4
	vaddss	%xmm7, %xmm1, %xmm5
	vmovshdup	%xmm1, %xmm6            # xmm6 = xmm1[1,1,3,3]
	vaddss	%xmm3, %xmm6, %xmm8
	vsubss	%xmm7, %xmm1, %xmm1
	vsubss	%xmm3, %xmm6, %xmm3
	vaddss	%xmm2, %xmm9, %xmm6
	vaddss	%xmm4, %xmm0, %xmm7
	vsubss	%xmm9, %xmm2, %xmm2
	vaddss	%xmm6, %xmm5, %xmm9
	vaddss	%xmm7, %xmm8, %xmm10
	vinsertps	$16, %xmm10, %xmm9, %xmm9 # xmm9 = xmm9[0],xmm10[0],xmm9[2,3]
	vmovlps	%xmm9, (%rdi,%r8,8)
	vsubss	%xmm6, %xmm5, %xmm5
	vsubss	%xmm7, %xmm8, %xmm6
	vinsertps	$16, %xmm6, %xmm5, %xmm5 # xmm5 = xmm5[0],xmm6[0],xmm5[2,3]
	vmovlps	%xmm5, (%r9,%r10)
	vsubss	%xmm4, %xmm0, %xmm0
	vaddss	%xmm0, %xmm1, %xmm4
	vsubss	%xmm2, %xmm3, %xmm5
	vinsertps	$16, %xmm5, %xmm4, %xmm4 # xmm4 = xmm4[0],xmm5[0],xmm4[2,3]
	vmovlps	%xmm4, (%r10,%rdx,8)
	vsubss	%xmm0, %xmm1, %xmm0
	vaddss	%xmm2, %xmm3, %xmm1
	vinsertps	$16, %xmm1, %xmm0, %xmm0 # xmm0 = xmm0[0],xmm1[0],xmm0[2,3]
	vmovlps	%xmm0, (%r10,%rcx)
	cmpl	$1, %eax
	je	.LBB30_8
# %bb.6:
	leaq	(%rdx,%rdx), %r10
	leaq	(%rdi,%r8,8), %r11
	addq	$8, %r11
	vmovsd	8(%rdi,%r8,8), %xmm1            # xmm1 = mem[0],zero
	leaq	8(%rsi,%r8,8), %rbx
	vmovsd	8(%rsi,%r8,8), %xmm0            # xmm0 = mem[0],zero
	vmovsd	(%r11,%rdx,8), %xmm3            # xmm3 = mem[0],zero
	vmovshdup	%xmm3, %xmm4            # xmm4 = xmm3[1,1,3,3]
	vmovshdup	%xmm0, %xmm5            # xmm5 = xmm0[1,1,3,3]
	vmulss	%xmm5, %xmm4, %xmm2
	vfmsub231ss	%xmm0, %xmm3, %xmm2     # xmm2 = (xmm3 * xmm0) - xmm2
	vmulss	%xmm4, %xmm0, %xmm0
	vfmadd231ss	%xmm5, %xmm3, %xmm0     # xmm0 = (xmm3 * xmm5) + xmm0
	vmovsd	(%rbx,%rdx,8), %xmm3            # xmm3 = mem[0],zero
	vmovsd	(%r11,%r10,8), %xmm4            # xmm4 = mem[0],zero
	vmovshdup	%xmm4, %xmm5            # xmm5 = xmm4[1,1,3,3]
	vmovshdup	%xmm3, %xmm6            # xmm6 = xmm3[1,1,3,3]
	vmulss	%xmm6, %xmm5, %xmm7
	vfmsub231ss	%xmm3, %xmm4, %xmm7     # xmm7 = (xmm4 * xmm3) - xmm7
	vmulss	%xmm5, %xmm3, %xmm3
	vfmadd231ss	%xmm6, %xmm4, %xmm3     # xmm3 = (xmm4 * xmm6) + xmm3
	vmovsd	(%rbx,%r10,8), %xmm4            # xmm4 = mem[0],zero
	vmovsd	(%rcx,%r11), %xmm5              # xmm5 = mem[0],zero
	vmovshdup	%xmm5, %xmm6            # xmm6 = xmm5[1,1,3,3]
	vmovshdup	%xmm4, %xmm8            # xmm8 = xmm4[1,1,3,3]
	vmulss	%xmm6, %xmm8, %xmm9
	vfmsub231ss	%xmm4, %xmm5, %xmm9     # xmm9 = (xmm5 * xmm4) - xmm9
	vmulss	%xmm6, %xmm4, %xmm4
	vfmadd231ss	%xmm8, %xmm5, %xmm4     # xmm4 = (xmm5 * xmm8) + xmm4
	vaddss	%xmm7, %xmm1, %xmm5
	vmovshdup	%xmm1, %xmm6            # xmm6 = xmm1[1,1,3,3]
	vaddss	%xmm3, %xmm6, %xmm8
	vsubss	%xmm7, %xmm1, %xmm1
	vsubss	%xmm3, %xmm6, %xmm3
	vaddss	%xmm2, %xmm9, %xmm6
	vaddss	%xmm4, %xmm0, %xmm7
	vsubss	%xmm9, %xmm2, %xmm2
	vaddss	%xmm6, %xmm5, %xmm9
	vaddss	%xmm7, %xmm8, %xmm10
	vinsertps	$16, %xmm10, %xmm9, %xmm9 # xmm9 = xmm9[0],xmm10[0],xmm9[2,3]
	vmovlps	%xmm9, 8(%rdi,%r8,8)
	vsubss	%xmm6, %xmm5, %xmm5
	vsubss	%xmm7, %xmm8, %xmm6
	vinsertps	$16, %xmm6, %xmm5, %xmm5 # xmm5 = xmm5[0],xmm6[0],xmm5[2,3]
	vmovlps	%xmm5, (%r9,%r11)
	vsubss	%xmm4, %xmm0, %xmm0
	vaddss	%xmm0, %xmm1, %xmm4
	vsubss	%xmm2, %xmm3, %xmm5
	vinsertps	$16, %xmm5, %xmm4, %xmm4 # xmm4 = xmm4[0],xmm5[0],xmm4[2,3]
	vmovlps	%xmm4, (%r11,%rdx,8)
	vsubss	%xmm0, %xmm1, %xmm0
	vaddss	%xmm2, %xmm3, %xmm1
	vinsertps	$16, %xmm1, %xmm0, %xmm0 # xmm0 = xmm0[0],xmm1[0],xmm0[2,3]
	vmovlps	%xmm0, (%rcx,%r11)
	cmpl	$2, %eax
	je	.LBB30_8
# %bb.7:
	leaq	(%rdi,%r8,8), %rax
	addq	$16, %rax
	vmovsd	16(%rdi,%r8,8), %xmm1           # xmm1 = mem[0],zero
	leaq	16(%rsi,%r8,8), %r11
	vmovsd	16(%rsi,%r8,8), %xmm0           # xmm0 = mem[0],zero
	vmovsd	(%rax,%rdx,8), %xmm3            # xmm3 = mem[0],zero
	vmovshdup	%xmm3, %xmm4            # xmm4 = xmm3[1,1,3,3]
	vmovshdup	%xmm0, %xmm5            # xmm5 = xmm0[1,1,3,3]
	vmulss	%xmm5, %xmm4, %xmm2
	vfmsub231ss	%xmm0, %xmm3, %xmm2     # xmm2 = (xmm3 * xmm0) - xmm2
	vmulss	%xmm4, %xmm0, %xmm0
	vfmadd231ss	%xmm5, %xmm3, %xmm0     # xmm0 = (xmm3 * xmm5) + xmm0
	vmovsd	(%r11,%rdx,8), %xmm3            # xmm3 = mem[0],zero
	vmovsd	(%rax,%r10,8), %xmm4            # xmm4 = mem[0],zero
	vmovshdup	%xmm4, %xmm5            # xmm5 = xmm4[1,1,3,3]
	vmovshdup	%xmm3, %xmm6            # xmm6 = xmm3[1,1,3,3]
	vmulss	%xmm6, %xmm5, %xmm7
	vfmsub231ss	%xmm3, %xmm4, %xmm7     # xmm7 = (xmm4 * xmm3) - xmm7
	vmulss	%xmm5, %xmm3, %xmm3
	vfmadd231ss	%xmm6, %xmm4, %xmm3     # xmm3 = (xmm4 * xmm6) + xmm3
	vmovsd	(%r11,%r10,8), %xmm4            # xmm4 = mem[0],zero
	vmovsd	(%rcx,%rax), %xmm5              # xmm5 = mem[0],zero
	vmovshdup	%xmm5, %xmm6            # xmm6 = xmm5[1,1,3,3]
	vmovshdup	%xmm4, %xmm8            # xmm8 = xmm4[1,1,3,3]
	vmulss	%xmm6, %xmm8, %xmm9
	vfmsub231ss	%xmm4, %xmm5, %xmm9     # xmm9 = (xmm5 * xmm4) - xmm9
	vmulss	%xmm6, %xmm4, %xmm4
	vfmadd231ss	%xmm8, %xmm5, %xmm4     # xmm4 = (xmm5 * xmm8) + xmm4
	vaddss	%xmm7, %xmm1, %xmm5
	vmovshdup	%xmm1, %xmm6            # xmm6 = xmm1[1,1,3,3]
	vaddss	%xmm3, %xmm6, %xmm8
	vsubss	%xmm7, %xmm1, %xmm1
	vsubss	%xmm3, %xmm6, %xmm3
	vaddss	%xmm2, %xmm9, %xmm6
	vaddss	%xmm4, %xmm0, %xmm7
	vsubss	%xmm9, %xmm2, %xmm2
	vaddss	%xmm6, %xmm5, %xmm9
	vaddss	%xmm7, %xmm8, %xmm10
	vinsertps	$16, %xmm10, %xmm9, %xmm9 # xmm9 = xmm9[0],xmm10[0],xmm9[2,3]
	vmovlps	%xmm9, 16(%rdi,%r8,8)
	vsubss	%xmm4, %xmm0, %xmm0
	vsubss	%xmm6, %xmm5, %xmm4
	vsubss	%xmm7, %xmm8, %xmm5
	vinsertps	$16, %xmm5, %xmm4, %xmm4 # xmm4 = xmm4[0],xmm5[0],xmm4[2,3]
	vmovlps	%xmm4, (%r9,%rax)
	vaddss	%xmm0, %xmm1, %xmm4
	vsubss	%xmm2, %xmm3, %xmm5
	vinsertps	$16, %xmm5, %xmm4, %xmm4 # xmm4 = xmm4[0],xmm5[0],xmm4[2,3]
	vmovlps	%xmm4, (%rax,%rdx,8)
	vsubss	%xmm0, %xmm1, %xmm0
	vaddss	%xmm2, %xmm3, %xmm1
	vinsertps	$16, %xmm1, %xmm0, %xmm0 # xmm0 = xmm0[0],xmm1[0],xmm0[2,3]
	vmovlps	%xmm0, (%rcx,%rax)
.LBB30_8:
	popq	%rbx
	popq	%r14
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	.cfi_restore %rbx
	.cfi_restore %r14
	.cfi_restore %rbp
.LBB30_9:
	vzeroupper
	retq
.Lfunc_end30:
	.size	_ZN6micron4math3fft6__impl7__stageILm4ELb0ETkNS1_10fft_scalarEfEEvPNS0_7complexIT1_EEPKS6_mmm, .Lfunc_end30-_ZN6micron4math3fft6__impl7__stageILm4ELb0ETkNS1_10fft_scalarEfEEvPNS0_7complexIT1_EEPKS6_mmm
