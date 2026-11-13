_ZN6micron4math3fft6__impl7__stageILm7ELb0ETkNS1_10fft_scalarEfEEvPNS0_7complexIT1_EEPKS6_mmm: # @_ZN6micron4math3fft6__impl7__stageILm7ELb0ETkNS1_10fft_scalarEfEEvPNS0_7complexIT1_EEPKS6_mmm
	.cfi_startproc
# %bb.0:
	cmpq	%rcx, %r8
	jbe	.LBB34_8
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
	movl	%r8d, %eax
	subl	%ecx, %eax
	andl	$3, %eax
	movq	%r8, %r10
	movq	%rax, -112(%rbp)                # 8-byte Spill
	subq	%rax, %r10
	cmpq	%r10, %rcx
	jae	.LBB34_4
# %bb.2:
	leaq	(%rdx,%rdx,2), %rax
	movq	%rax, %r11
	shlq	$4, %r11
	addq	%rdi, %r11
	leaq	(,%rdx,8), %rbx
	leaq	(%rdi,%rdx,8), %r14
	movq	%rdx, %r15
	shlq	$4, %r15
	addq	%rdi, %r15
	leaq	(%rdi,%rax,8), %r12
	movq	%rdx, %r13
	shlq	$5, %r13
	addq	%rdi, %r13
	leaq	(%rdx,%rdx,4), %rax
	leaq	(%rdi,%rax,8), %r9
	vbroadcastss	.LCPI34_0(%rip), %ymm8  # ymm8 = [6.23489797E-1,6.23489797E-1,6.23489797E-1,6.23489797E-1,6.23489797E-1,6.23489797E-1,6.23489797E-1,6.23489797E-1]
	vbroadcastss	.LCPI34_2(%rip), %ymm3  # ymm3 = [-2.22520933E-1,-2.22520933E-1,-2.22520933E-1,-2.22520933E-1,-2.22520933E-1,-2.22520933E-1,-2.22520933E-1,-2.22520933E-1]
	vbroadcastss	.LCPI34_3(%rip), %ymm4  # ymm4 = [9.74927902E-1,9.74927902E-1,9.74927902E-1,9.74927902E-1,9.74927902E-1,9.74927902E-1,9.74927902E-1,9.74927902E-1]
	vbroadcastss	.LCPI34_4(%rip), %ymm6  # ymm6 = [-9.00968849E-1,-9.00968849E-1,-9.00968849E-1,-9.00968849E-1,-9.00968849E-1,-9.00968849E-1,-9.00968849E-1,-9.00968849E-1]
	vbroadcastsd	.LCPI34_9(%rip), %ymm2  # ymm2 = [0,2147483648,0,2147483648,0,2147483648,0,2147483648]
	vbroadcastss	.LCPI34_8(%rip), %ymm0  # ymm0 = [-7.81831503E-1,-7.81831503E-1,-7.81831503E-1,-7.81831503E-1,-7.81831503E-1,-7.81831503E-1,-7.81831503E-1,-7.81831503E-1]
	.p2align	4
.LBB34_3:                               # =>This Inner Loop Header: Depth=1
	vmovups	(%rdi,%rcx,8), %ymm9
	leaq	(%rsi,%rcx,8), %rax
	vmovups	(%rsi,%rcx,8), %ymm1
	vmovups	(%r14,%rcx,8), %ymm10
	vmovshdup	%ymm10, %ymm11          # ymm11 = ymm10[1,1,3,3,5,5,7,7]
	vmovsldup	%ymm10, %ymm12          # ymm12 = ymm10[0,0,2,2,4,4,6,6]
	vshufps	$177, %ymm1, %ymm1, %ymm10      # ymm10 = ymm1[1,0,3,2,5,4,7,6]
	vmulps	%ymm11, %ymm10, %ymm10
	vfmaddsub231ps	%ymm12, %ymm1, %ymm10   # ymm10 = (ymm1 * ymm12) +/- ymm10
	vmovups	(%rbx,%rax), %ymm1
	addq	%rbx, %rax
	vmovups	(%r15,%rcx,8), %ymm11
	vmovshdup	%ymm11, %ymm12          # ymm12 = ymm11[1,1,3,3,5,5,7,7]
	vmovsldup	%ymm11, %ymm13          # ymm13 = ymm11[0,0,2,2,4,4,6,6]
	vshufps	$177, %ymm1, %ymm1, %ymm11      # ymm11 = ymm1[1,0,3,2,5,4,7,6]
	vmulps	%ymm12, %ymm11, %ymm11
	vfmaddsub231ps	%ymm13, %ymm1, %ymm11   # ymm11 = (ymm1 * ymm13) +/- ymm11
	vmovups	(%rbx,%rax), %ymm12
	addq	%rbx, %rax
	vmovups	(%r12,%rcx,8), %ymm1
	vmovshdup	%ymm1, %ymm13           # ymm13 = ymm1[1,1,3,3,5,5,7,7]
	vmovsldup	%ymm1, %ymm14           # ymm14 = ymm1[0,0,2,2,4,4,6,6]
	vshufps	$177, %ymm12, %ymm12, %ymm1     # ymm1 = ymm12[1,0,3,2,5,4,7,6]
	vmulps	%ymm1, %ymm13, %ymm1
	vfmaddsub231ps	%ymm14, %ymm12, %ymm1   # ymm1 = (ymm12 * ymm14) +/- ymm1
	vmovups	(%rbx,%rax), %ymm12
	addq	%rbx, %rax
	vmovups	(%r13,%rcx,8), %ymm13
	vmovshdup	%ymm13, %ymm14          # ymm14 = ymm13[1,1,3,3,5,5,7,7]
	vmovsldup	%ymm13, %ymm13          # ymm13 = ymm13[0,0,2,2,4,4,6,6]
	vshufps	$177, %ymm12, %ymm12, %ymm15    # ymm15 = ymm12[1,0,3,2,5,4,7,6]
	vmulps	%ymm14, %ymm15, %ymm7
	vfmaddsub231ps	%ymm13, %ymm12, %ymm7   # ymm7 = (ymm12 * ymm13) +/- ymm7
	vmovups	(%rbx,%rax), %ymm12
	addq	%rbx, %rax
	vmovups	(%r9,%rcx,8), %ymm13
	vmovshdup	%ymm13, %ymm14          # ymm14 = ymm13[1,1,3,3,5,5,7,7]
	vmovsldup	%ymm13, %ymm13          # ymm13 = ymm13[0,0,2,2,4,4,6,6]
	vshufps	$177, %ymm12, %ymm12, %ymm15    # ymm15 = ymm12[1,0,3,2,5,4,7,6]
	vmulps	%ymm14, %ymm15, %ymm5
	vfmaddsub231ps	%ymm13, %ymm12, %ymm5   # ymm5 = (ymm12 * ymm13) +/- ymm5
	vmovups	(%rbx,%rax), %ymm12
	vmovups	(%r11,%rcx,8), %ymm13
	vmovshdup	%ymm13, %ymm14          # ymm14 = ymm13[1,1,3,3,5,5,7,7]
	vmovsldup	%ymm13, %ymm13          # ymm13 = ymm13[0,0,2,2,4,4,6,6]
	vshufps	$177, %ymm12, %ymm12, %ymm15    # ymm15 = ymm12[1,0,3,2,5,4,7,6]
	vmulps	%ymm14, %ymm15, %ymm15
	vfmaddsub231ps	%ymm13, %ymm12, %ymm15  # ymm15 = (ymm12 * ymm13) +/- ymm15
	vaddps	%ymm15, %ymm10, %ymm14
	vsubps	%ymm15, %ymm10, %ymm10
	vaddps	%ymm14, %ymm9, %ymm13
	vaddps	%ymm5, %ymm11, %ymm15
	vsubps	%ymm5, %ymm11, %ymm12
	vaddps	%ymm15, %ymm13, %ymm5
	vaddps	%ymm7, %ymm1, %ymm13
	vsubps	%ymm7, %ymm1, %ymm11
	vaddps	%ymm5, %ymm13, %ymm1
	vmovups	%ymm1, (%rdi,%rcx,8)
	vmulps	%ymm8, %ymm14, %ymm1
	vaddps	%ymm1, %ymm9, %ymm1
	vbroadcastss	.LCPI34_1(%rip), %ymm5  # ymm5 = [7.81831503E-1,7.81831503E-1,7.81831503E-1,7.81831503E-1,7.81831503E-1,7.81831503E-1,7.81831503E-1,7.81831503E-1]
	vmulps	%ymm5, %ymm10, %ymm5
	vmulps	%ymm3, %ymm15, %ymm7
	vaddps	%ymm7, %ymm1, %ymm1
	vmulps	%ymm4, %ymm12, %ymm7
	vaddps	%ymm7, %ymm5, %ymm5
	vmulps	%ymm6, %ymm13, %ymm7
	vaddps	%ymm7, %ymm1, %ymm1
	vbroadcastss	.LCPI34_5(%rip), %ymm7  # ymm7 = [4.33883727E-1,4.33883727E-1,4.33883727E-1,4.33883727E-1,4.33883727E-1,4.33883727E-1,4.33883727E-1,4.33883727E-1]
	vmulps	%ymm7, %ymm11, %ymm7
	vaddps	%ymm7, %ymm5, %ymm5
	vshufps	$177, %ymm5, %ymm5, %ymm5       # ymm5 = ymm5[1,0,3,2,5,4,7,6]
	vxorps	%ymm2, %ymm5, %ymm5
	vaddps	%ymm5, %ymm1, %ymm7
	vmovups	%ymm7, (%r14,%rcx,8)
	vsubps	%ymm5, %ymm1, %ymm1
	vmovups	%ymm1, (%r11,%rcx,8)
	vmulps	%ymm3, %ymm14, %ymm1
	vaddps	%ymm1, %ymm9, %ymm1
	vmulps	%ymm4, %ymm10, %ymm5
	vmulps	%ymm6, %ymm15, %ymm7
	vaddps	%ymm7, %ymm1, %ymm1
	vbroadcastss	.LCPI34_7(%rip), %ymm7  # ymm7 = [-4.33883727E-1,-4.33883727E-1,-4.33883727E-1,-4.33883727E-1,-4.33883727E-1,-4.33883727E-1,-4.33883727E-1,-4.33883727E-1]
	vmulps	%ymm7, %ymm12, %ymm7
	vaddps	%ymm7, %ymm5, %ymm5
	vmulps	%ymm8, %ymm13, %ymm7
	vaddps	%ymm7, %ymm1, %ymm1
	vmulps	%ymm0, %ymm11, %ymm7
	vaddps	%ymm7, %ymm5, %ymm5
	vshufps	$177, %ymm5, %ymm5, %ymm5       # ymm5 = ymm5[1,0,3,2,5,4,7,6]
	vxorps	%ymm2, %ymm5, %ymm5
	vaddps	%ymm5, %ymm1, %ymm7
	vmovups	%ymm7, (%r15,%rcx,8)
	vsubps	%ymm5, %ymm1, %ymm1
	vmovups	%ymm1, (%r9,%rcx,8)
	vmulps	%ymm6, %ymm14, %ymm1
	vaddps	%ymm1, %ymm9, %ymm1
	vmulps	%ymm8, %ymm15, %ymm5
	vaddps	%ymm5, %ymm1, %ymm1
	vbroadcastss	.LCPI34_5(%rip), %ymm5  # ymm5 = [4.33883727E-1,4.33883727E-1,4.33883727E-1,4.33883727E-1,4.33883727E-1,4.33883727E-1,4.33883727E-1,4.33883727E-1]
	vmulps	%ymm5, %ymm10, %ymm5
	vmulps	%ymm0, %ymm12, %ymm7
	vaddps	%ymm7, %ymm5, %ymm5
	vmulps	%ymm3, %ymm13, %ymm7
	vaddps	%ymm7, %ymm1, %ymm1
	vmulps	%ymm4, %ymm11, %ymm7
	vaddps	%ymm7, %ymm5, %ymm5
	vshufps	$177, %ymm5, %ymm5, %ymm5       # ymm5 = ymm5[1,0,3,2,5,4,7,6]
	vxorps	%ymm2, %ymm5, %ymm5
	vaddps	%ymm5, %ymm1, %ymm7
	vmovups	%ymm7, (%r12,%rcx,8)
	vsubps	%ymm5, %ymm1, %ymm1
	vmovups	%ymm1, (%r13,%rcx,8)
	addq	$4, %rcx
	cmpq	%r10, %rcx
	jb	.LBB34_3
.LBB34_4:
	movq	-112(%rbp), %r12                # 8-byte Reload
	testq	%r12, %r12
	je	.LBB34_7
# %bb.5:
	leaq	(%rdx,%rdx,2), %rcx
	movq	%rcx, %rax
	shlq	$4, %rax
	shll	$3, %r12d
	leaq	(,%r8,8), %r10
	leaq	(%r10,%rcx,8), %rcx
	leaq	(%rdx,%rdx,4), %r9
	leaq	(%r10,%r9,8), %r9
	subq	%r12, %r10
	addq	%r10, %rsi
	leaq	(%rax,%r8,8), %r11
	subq	%r12, %r11
	addq	%rdi, %r11
	leaq	(,%rdx,8), %rbx
	leaq	(%rbx,%r8,8), %r14
	subq	%r12, %r14
	addq	%rdi, %r14
	movq	%rdx, %rax
	shlq	$4, %rax
	leaq	(%rax,%r8,8), %r15
	subq	%r12, %r15
	addq	%rdi, %r15
	subq	%r12, %rcx
	addq	%rdi, %rcx
	shlq	$5, %rdx
	leaq	(%rdx,%r8,8), %rdx
	subq	%r12, %rdx
	addq	%rdi, %rdx
	subq	%r12, %r9
	addq	%rdi, %r9
	addq	%rdi, %r10
	xorl	%edi, %edi
	.p2align	4
.LBB34_6:                               # =>This Inner Loop Header: Depth=1
	vmovsd	(%r10,%rdi), %xmm0              # xmm0 = mem[0],zero
	vmovaps	%xmm0, -112(%rbp)               # 16-byte Spill
	leaq	(%rsi,%rdi), %r8
	vmovsd	(%rsi,%rdi), %xmm0              # xmm0 = mem[0],zero
	vmovsd	(%r14,%rdi), %xmm3              # xmm3 = mem[0],zero
	vmovshdup	%xmm3, %xmm2            # xmm2 = xmm3[1,1,3,3]
	vmovshdup	%xmm0, %xmm4            # xmm4 = xmm0[1,1,3,3]
	vmulss	%xmm4, %xmm2, %xmm9
	vfmsub231ss	%xmm0, %xmm3, %xmm9     # xmm9 = (xmm3 * xmm0) - xmm9
	vmulss	%xmm2, %xmm0, %xmm2
	vfmadd231ss	%xmm4, %xmm3, %xmm2     # xmm2 = (xmm3 * xmm4) + xmm2
	vmovsd	(%rbx,%r8), %xmm0               # xmm0 = mem[0],zero
	addq	%rbx, %r8
	vmovsd	(%r15,%rdi), %xmm3              # xmm3 = mem[0],zero
	vmovshdup	%xmm3, %xmm4            # xmm4 = xmm3[1,1,3,3]
	vmovshdup	%xmm0, %xmm5            # xmm5 = xmm0[1,1,3,3]
	vmulss	%xmm5, %xmm4, %xmm6
	vfmsub231ss	%xmm0, %xmm3, %xmm6     # xmm6 = (xmm3 * xmm0) - xmm6
	vmulss	%xmm4, %xmm0, %xmm11
	vfmadd231ss	%xmm5, %xmm3, %xmm11    # xmm11 = (xmm3 * xmm5) + xmm11
	vmovsd	(%rbx,%r8), %xmm0               # xmm0 = mem[0],zero
	addq	%rbx, %r8
	vmovsd	(%rcx,%rdi), %xmm4              # xmm4 = mem[0],zero
	vmovshdup	%xmm4, %xmm3            # xmm3 = xmm4[1,1,3,3]
	vmovshdup	%xmm0, %xmm5            # xmm5 = xmm0[1,1,3,3]
	vmulss	%xmm5, %xmm3, %xmm12
	vfmsub231ss	%xmm0, %xmm4, %xmm12    # xmm12 = (xmm4 * xmm0) - xmm12
	vmulss	%xmm3, %xmm0, %xmm3
	vfmadd231ss	%xmm5, %xmm4, %xmm3     # xmm3 = (xmm4 * xmm5) + xmm3
	vmovsd	(%rbx,%r8), %xmm0               # xmm0 = mem[0],zero
	addq	%rbx, %r8
	vmovsd	(%rdx,%rdi), %xmm5              # xmm5 = mem[0],zero
	vmovshdup	%xmm5, %xmm7            # xmm7 = xmm5[1,1,3,3]
	vmovshdup	%xmm0, %xmm8            # xmm8 = xmm0[1,1,3,3]
	vmulss	%xmm7, %xmm8, %xmm4
	vfmsub231ss	%xmm0, %xmm5, %xmm4     # xmm4 = (xmm5 * xmm0) - xmm4
	vmulss	%xmm7, %xmm0, %xmm1
	vfmadd231ss	%xmm8, %xmm5, %xmm1     # xmm1 = (xmm5 * xmm8) + xmm1
	vmovsd	(%rbx,%r8), %xmm5               # xmm5 = mem[0],zero
	addq	%rbx, %r8
	vmovsd	(%r9,%rdi), %xmm7               # xmm7 = mem[0],zero
	vmovshdup	%xmm7, %xmm8            # xmm8 = xmm7[1,1,3,3]
	vmovshdup	%xmm5, %xmm10           # xmm10 = xmm5[1,1,3,3]
	vmulss	%xmm10, %xmm8, %xmm13
	vfmsub231ss	%xmm5, %xmm7, %xmm13    # xmm13 = (xmm7 * xmm5) - xmm13
	vmulss	%xmm5, %xmm8, %xmm15
	vfmadd231ss	%xmm10, %xmm7, %xmm15   # xmm15 = (xmm7 * xmm10) + xmm15
	vmovsd	(%rbx,%r8), %xmm5               # xmm5 = mem[0],zero
	vmovsd	(%r11,%rdi), %xmm7              # xmm7 = mem[0],zero
	vmovshdup	%xmm7, %xmm8            # xmm8 = xmm7[1,1,3,3]
	vmovshdup	%xmm5, %xmm10           # xmm10 = xmm5[1,1,3,3]
	vmulss	%xmm10, %xmm8, %xmm14
	vfmsub231ss	%xmm5, %xmm7, %xmm14    # xmm14 = (xmm7 * xmm5) - xmm14
	vmulss	%xmm5, %xmm8, %xmm0
	vfmadd231ss	%xmm10, %xmm7, %xmm0    # xmm0 = (xmm7 * xmm10) + xmm0
	vaddss	%xmm14, %xmm9, %xmm7
	vaddss	%xmm0, %xmm2, %xmm10
	vsubss	%xmm14, %xmm9, %xmm5
	vmovss	%xmm5, -56(%rbp)                # 4-byte Spill
	vsubss	%xmm0, %xmm2, %xmm5
	vmovss	%xmm5, -80(%rbp)                # 4-byte Spill
	vmovaps	-112(%rbp), %xmm0               # 16-byte Reload
	vaddss	%xmm7, %xmm0, %xmm9
	vmovshdup	%xmm0, %xmm14           # xmm14 = xmm0[1,1,3,3]
	vaddss	%xmm6, %xmm13, %xmm0
	vsubss	%xmm13, %xmm6, %xmm2
	vmovss	%xmm2, -44(%rbp)                # 4-byte Spill
	vaddss	%xmm15, %xmm11, %xmm13
	vsubss	%xmm15, %xmm11, %xmm2
	vmovss	%xmm2, -48(%rbp)                # 4-byte Spill
	vaddss	%xmm4, %xmm12, %xmm6
	vsubss	%xmm4, %xmm12, %xmm11
	vmovss	%xmm11, -76(%rbp)               # 4-byte Spill
	vaddss	%xmm1, %xmm3, %xmm8
	vsubss	%xmm1, %xmm3, %xmm1
	vmovss	%xmm1, -52(%rbp)                # 4-byte Spill
	vmovaps	%xmm10, %xmm15
	vaddss	%xmm10, %xmm14, %xmm1
	vmovss	%xmm0, -68(%rbp)                # 4-byte Spill
	vaddss	%xmm0, %xmm9, %xmm3
	vaddss	%xmm1, %xmm13, %xmm1
	vmovaps	%xmm13, %xmm9
	vmovss	%xmm13, -84(%rbp)               # 4-byte Spill
	vaddss	%xmm6, %xmm3, %xmm3
	vmovss	%xmm6, -72(%rbp)                # 4-byte Spill
	vaddss	%xmm1, %xmm8, %xmm1
	vmovss	%xmm8, -64(%rbp)                # 4-byte Spill
	vinsertps	$16, %xmm1, %xmm3, %xmm1 # xmm1 = xmm3[0],xmm1[0],xmm3[2,3]
	vmovlps	%xmm1, (%r10,%rdi)
	vmovss	.LCPI34_0(%rip), %xmm2          # xmm2 = [6.23489797E-1,0.0E+0,0.0E+0,0.0E+0]
	vmovss	%xmm7, -88(%rbp)                # 4-byte Spill
	vmulss	%xmm2, %xmm7, %xmm1
	vmovaps	-112(%rbp), %xmm12              # 16-byte Reload
	vaddss	%xmm1, %xmm12, %xmm1
	vmovss	.LCPI34_2(%rip), %xmm13         # xmm13 = [-2.22520933E-1,0.0E+0,0.0E+0,0.0E+0]
	vmulss	%xmm0, %xmm13, %xmm3
	vaddss	%xmm3, %xmm1, %xmm1
	vmulss	%xmm2, %xmm10, %xmm3
	vmovss	%xmm10, -60(%rbp)               # 4-byte Spill
	vaddss	%xmm3, %xmm14, %xmm3
	vmulss	%xmm13, %xmm9, %xmm4
	vaddss	%xmm4, %xmm3, %xmm3
	vmovss	.LCPI34_1(%rip), %xmm10         # xmm10 = [7.81831503E-1,0.0E+0,0.0E+0,0.0E+0]
	vmovss	-56(%rbp), %xmm2                # 4-byte Reload
                                        # xmm2 = mem[0],zero,zero,zero
	vmulss	%xmm2, %xmm10, %xmm4
	vmovss	.LCPI34_3(%rip), %xmm0          # xmm0 = [9.74927902E-1,0.0E+0,0.0E+0,0.0E+0]
	vmovss	-44(%rbp), %xmm13               # 4-byte Reload
                                        # xmm13 = mem[0],zero,zero,zero
	vmulss	%xmm0, %xmm13, %xmm9
	vaddss	%xmm4, %xmm9, %xmm4
	vmulss	%xmm5, %xmm10, %xmm9
	vmovss	-48(%rbp), %xmm5                # 4-byte Reload
                                        # xmm5 = mem[0],zero,zero,zero
	vmulss	%xmm0, %xmm5, %xmm10
	vaddss	%xmm10, %xmm9, %xmm9
	vmovss	.LCPI34_4(%rip), %xmm0          # xmm0 = [-9.00968849E-1,0.0E+0,0.0E+0,0.0E+0]
	vmulss	%xmm0, %xmm6, %xmm10
	vaddss	%xmm1, %xmm10, %xmm1
	vmulss	%xmm0, %xmm8, %xmm10
	vmovaps	%xmm0, %xmm8
	vaddss	%xmm3, %xmm10, %xmm3
	vmovss	.LCPI34_5(%rip), %xmm6          # xmm6 = [4.33883727E-1,0.0E+0,0.0E+0,0.0E+0]
	vmulss	%xmm6, %xmm11, %xmm10
	vaddss	%xmm4, %xmm10, %xmm4
	vmovss	-52(%rbp), %xmm0                # 4-byte Reload
                                        # xmm0 = mem[0],zero,zero,zero
	vmulss	%xmm6, %xmm0, %xmm10
	vaddss	%xmm10, %xmm9, %xmm9
	vaddss	%xmm1, %xmm9, %xmm10
	vsubss	%xmm4, %xmm3, %xmm11
	vinsertps	$16, %xmm11, %xmm10, %xmm10 # xmm10 = xmm10[0],xmm11[0],xmm10[2,3]
	vmovlps	%xmm10, (%r14,%rdi)
	vsubss	%xmm9, %xmm1, %xmm1
	vaddss	%xmm3, %xmm4, %xmm3
	vinsertps	$16, %xmm3, %xmm1, %xmm1 # xmm1 = xmm1[0],xmm3[0],xmm1[2,3]
	vmovlps	%xmm1, (%r11,%rdi)
	vmovss	.LCPI34_2(%rip), %xmm4          # xmm4 = [-2.22520933E-1,0.0E+0,0.0E+0,0.0E+0]
	vmulss	%xmm4, %xmm7, %xmm1
	vaddss	%xmm1, %xmm12, %xmm1
	vmovss	-68(%rbp), %xmm12               # 4-byte Reload
                                        # xmm12 = mem[0],zero,zero,zero
	vmovaps	%xmm8, %xmm6
	vmulss	%xmm8, %xmm12, %xmm3
	vaddss	%xmm3, %xmm1, %xmm1
	vmulss	%xmm4, %xmm15, %xmm3
	vaddss	%xmm3, %xmm14, %xmm3
	vmovss	-84(%rbp), %xmm8                # 4-byte Reload
                                        # xmm8 = mem[0],zero,zero,zero
	vmulss	%xmm6, %xmm8, %xmm4
	vaddss	%xmm4, %xmm3, %xmm3
	vmovss	.LCPI34_3(%rip), %xmm7          # xmm7 = [9.74927902E-1,0.0E+0,0.0E+0,0.0E+0]
	vmulss	%xmm7, %xmm2, %xmm4
	vmovss	.LCPI34_7(%rip), %xmm10         # xmm10 = [-4.33883727E-1,0.0E+0,0.0E+0,0.0E+0]
	vmulss	%xmm10, %xmm13, %xmm9
	vaddss	%xmm4, %xmm9, %xmm4
	vmovss	-80(%rbp), %xmm6                # 4-byte Reload
                                        # xmm6 = mem[0],zero,zero,zero
	vmulss	%xmm7, %xmm6, %xmm9
	vmulss	%xmm5, %xmm10, %xmm10
	vaddss	%xmm10, %xmm9, %xmm9
	vmovss	.LCPI34_0(%rip), %xmm2          # xmm2 = [6.23489797E-1,0.0E+0,0.0E+0,0.0E+0]
	vmovss	-72(%rbp), %xmm7                # 4-byte Reload
                                        # xmm7 = mem[0],zero,zero,zero
	vmulss	%xmm2, %xmm7, %xmm10
	vaddss	%xmm1, %xmm10, %xmm1
	vmovss	-64(%rbp), %xmm15               # 4-byte Reload
                                        # xmm15 = mem[0],zero,zero,zero
	vmulss	%xmm2, %xmm15, %xmm10
	vaddss	%xmm3, %xmm10, %xmm3
	vmovss	.LCPI34_8(%rip), %xmm13         # xmm13 = [-7.81831503E-1,0.0E+0,0.0E+0,0.0E+0]
	vmovss	-76(%rbp), %xmm5                # 4-byte Reload
                                        # xmm5 = mem[0],zero,zero,zero
	vmulss	%xmm5, %xmm13, %xmm10
	vaddss	%xmm4, %xmm10, %xmm4
	vmulss	%xmm0, %xmm13, %xmm10
	vaddss	%xmm10, %xmm9, %xmm9
	vaddss	%xmm1, %xmm9, %xmm10
	vsubss	%xmm4, %xmm3, %xmm11
	vinsertps	$16, %xmm11, %xmm10, %xmm10 # xmm10 = xmm10[0],xmm11[0],xmm10[2,3]
	vmovlps	%xmm10, (%r15,%rdi)
	vsubss	%xmm9, %xmm1, %xmm1
	vaddss	%xmm3, %xmm4, %xmm3
	vinsertps	$16, %xmm3, %xmm1, %xmm1 # xmm1 = xmm1[0],xmm3[0],xmm1[2,3]
	vmovlps	%xmm1, (%r9,%rdi)
	vmovss	.LCPI34_4(%rip), %xmm0          # xmm0 = [-9.00968849E-1,0.0E+0,0.0E+0,0.0E+0]
	vmulss	-88(%rbp), %xmm0, %xmm1         # 4-byte Folded Reload
	vaddss	-112(%rbp), %xmm1, %xmm1        # 16-byte Folded Reload
	vmulss	-60(%rbp), %xmm0, %xmm3         # 4-byte Folded Reload
	vaddss	%xmm3, %xmm14, %xmm0
	vmulss	%xmm2, %xmm12, %xmm3
	vaddss	%xmm3, %xmm1, %xmm1
	vmulss	%xmm2, %xmm8, %xmm2
	vaddss	%xmm2, %xmm0, %xmm0
	vmovss	.LCPI34_5(%rip), %xmm4          # xmm4 = [4.33883727E-1,0.0E+0,0.0E+0,0.0E+0]
	vmulss	-56(%rbp), %xmm4, %xmm2         # 4-byte Folded Reload
	vmulss	-44(%rbp), %xmm13, %xmm3        # 4-byte Folded Reload
	vaddss	%xmm3, %xmm2, %xmm2
	vmulss	%xmm4, %xmm6, %xmm3
	vmulss	-48(%rbp), %xmm13, %xmm4        # 4-byte Folded Reload
	vaddss	%xmm4, %xmm3, %xmm3
	vmovss	.LCPI34_2(%rip), %xmm6          # xmm6 = [-2.22520933E-1,0.0E+0,0.0E+0,0.0E+0]
	vmulss	%xmm6, %xmm7, %xmm4
	vaddss	%xmm4, %xmm1, %xmm1
	vmulss	%xmm6, %xmm15, %xmm4
	vaddss	%xmm4, %xmm0, %xmm0
	vmovss	.LCPI34_3(%rip), %xmm6          # xmm6 = [9.74927902E-1,0.0E+0,0.0E+0,0.0E+0]
	vmulss	%xmm6, %xmm5, %xmm4
	vaddss	%xmm4, %xmm2, %xmm2
	vmulss	-52(%rbp), %xmm6, %xmm4         # 4-byte Folded Reload
	vaddss	%xmm4, %xmm3, %xmm3
	vaddss	%xmm1, %xmm3, %xmm4
	vsubss	%xmm2, %xmm0, %xmm5
	vinsertps	$16, %xmm5, %xmm4, %xmm4 # xmm4 = xmm4[0],xmm5[0],xmm4[2,3]
	vmovlps	%xmm4, (%rcx,%rdi)
	vsubss	%xmm3, %xmm1, %xmm1
	vaddss	%xmm0, %xmm2, %xmm0
	vinsertps	$16, %xmm0, %xmm1, %xmm0 # xmm0 = xmm1[0],xmm0[0],xmm1[2,3]
	vmovlps	%xmm0, (%rdx,%rdi)
	addq	$8, %rdi
	cmpq	%rdi, %r12
	jne	.LBB34_6
.LBB34_7:
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
.LBB34_8:
	vzeroupper
	retq
.Lfunc_end34:
	.size	_ZN6micron4math3fft6__impl7__stageILm7ELb0ETkNS1_10fft_scalarEfEEvPNS0_7complexIT1_EEPKS6_mmm, .Lfunc_end34-_ZN6micron4math3fft6__impl7__stageILm7ELb0ETkNS1_10fft_scalarEfEEvPNS0_7complexIT1_EEPKS6_mmm
