_ZN6micron4math3fft6__impl7__stageILm7ELb0ETkNS1_10fft_scalarEdEEvPNS0_7complexIT1_EEPKS6_mmm: # @_ZN6micron4math3fft6__impl7__stageILm7ELb0ETkNS1_10fft_scalarEdEEvPNS0_7complexIT1_EEPKS6_mmm
	.cfi_startproc
# %bb.0:
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
	andq	$-32, %rsp
	subq	$192, %rsp
	.cfi_offset %rbx, -56
	.cfi_offset %r12, -48
	.cfi_offset %r13, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	movq	%rsi, 24(%rsp)                  # 8-byte Spill
	movq	%rdi, 40(%rsp)                  # 8-byte Spill
	cmpq	%rcx, %r8
	jbe	.LBB42_6
# %bb.1:
	movl	%r8d, %r9d
	subl	%ecx, %r9d
	andl	$1, %r9d
	subq	%r9, %r8
	movq	%rdx, 16(%rsp)                  # 8-byte Spill
	leaq	(%rdx,%rdx,2), %rax
	movq	%rax, 32(%rsp)                  # 8-byte Spill
	cmpq	%r8, %rcx
	jae	.LBB42_4
# %bb.2:
	movq	32(%rsp), %r12                  # 8-byte Reload
	movq	%r12, %r10
	shlq	$5, %r10
	movq	%rcx, %r11
	shlq	$4, %r11
	movq	40(%rsp), %rdi                  # 8-byte Reload
	addq	%rdi, %r10
	movq	16(%rsp), %rax                  # 8-byte Reload
	movq	%rax, %rbx
	shlq	$4, %rbx
	leaq	(%rdi,%rbx), %r14
	movq	%rax, %r15
	shlq	$5, %r15
	addq	%rdi, %r15
	shlq	$4, %r12
	addq	%rdi, %r12
	movq	%rax, %r13
	shlq	$6, %r13
	addq	%rdi, %r13
	leaq	(%rax,%rax,4), %rax
	shlq	$4, %rax
	addq	%rdi, %rax
	vbroadcastf128	.LCPI42_6(%rip), %ymm0  # ymm0 = [9223372036854775808,0,9223372036854775808,0]
                                        # ymm0 = mem[0,1,0,1]
	vmovaps	%ymm0, 128(%rsp)                # 32-byte Spill
	movq	24(%rsp), %rsi                  # 8-byte Reload
	vbroadcastsd	.LCPI42_0(%rip), %ymm8  # ymm8 = [6.2348980185873348E-1,6.2348980185873348E-1,6.2348980185873348E-1,6.2348980185873348E-1]
	vbroadcastsd	.LCPI42_2(%rip), %ymm3  # ymm3 = [-2.2252093395631439E-1,-2.2252093395631439E-1,-2.2252093395631439E-1,-2.2252093395631439E-1]
	vbroadcastsd	.LCPI42_3(%rip), %ymm4  # ymm4 = [9.7492791218182361E-1,9.7492791218182361E-1,9.7492791218182361E-1,9.7492791218182361E-1]
	vbroadcastsd	.LCPI42_4(%rip), %ymm6  # ymm6 = [-9.0096886790241915E-1,-9.0096886790241915E-1,-9.0096886790241915E-1,-9.0096886790241915E-1]
	vmovapd	128(%rsp), %ymm0                # 32-byte Reload
	vbroadcastsd	.LCPI42_8(%rip), %ymm2  # ymm2 = [-7.818314824680298E-1,-7.818314824680298E-1,-7.818314824680298E-1,-7.818314824680298E-1]
	.p2align	4
.LBB42_3:                               # =>This Inner Loop Header: Depth=1
	vmovupd	(%rdi,%r11), %ymm9
	leaq	(%rsi,%r11), %rdx
	vmovupd	(%rsi,%r11), %ymm1
	vmovupd	(%r14,%r11), %ymm10
	vmovddup	%ymm10, %ymm11                  # ymm11 = ymm10[0,0,2,2]
	vshufpd	$15, %ymm10, %ymm10, %ymm10     # ymm10 = ymm10[1,1,3,3]
	vshufpd	$5, %ymm1, %ymm1, %ymm12        # ymm12 = ymm1[1,0,3,2]
	vmulpd	%ymm10, %ymm12, %ymm10
	vfmaddsub231pd	%ymm11, %ymm1, %ymm10   # ymm10 = (ymm1 * ymm11) +/- ymm10
	vmovupd	(%rbx,%rdx), %ymm1
	addq	%rbx, %rdx
	vmovupd	(%r15,%r11), %ymm11
	vmovddup	%ymm11, %ymm12                  # ymm12 = ymm11[0,0,2,2]
	vshufpd	$15, %ymm11, %ymm11, %ymm11     # ymm11 = ymm11[1,1,3,3]
	vshufpd	$5, %ymm1, %ymm1, %ymm13        # ymm13 = ymm1[1,0,3,2]
	vmulpd	%ymm11, %ymm13, %ymm11
	vfmaddsub231pd	%ymm12, %ymm1, %ymm11   # ymm11 = (ymm1 * ymm12) +/- ymm11
	vmovupd	(%rbx,%rdx), %ymm12
	addq	%rbx, %rdx
	vmovupd	(%r12,%r11), %ymm1
	vmovddup	%ymm1, %ymm13                   # ymm13 = ymm1[0,0,2,2]
	vshufpd	$15, %ymm1, %ymm1, %ymm1        # ymm1 = ymm1[1,1,3,3]
	vshufpd	$5, %ymm12, %ymm12, %ymm14      # ymm14 = ymm12[1,0,3,2]
	vmulpd	%ymm1, %ymm14, %ymm1
	vfmaddsub231pd	%ymm13, %ymm12, %ymm1   # ymm1 = (ymm12 * ymm13) +/- ymm1
	vmovupd	(%rbx,%rdx), %ymm12
	addq	%rbx, %rdx
	vmovupd	(%r13,%r11), %ymm13
	vmovddup	%ymm13, %ymm14                  # ymm14 = ymm13[0,0,2,2]
	vshufpd	$15, %ymm13, %ymm13, %ymm13     # ymm13 = ymm13[1,1,3,3]
	vshufpd	$5, %ymm12, %ymm12, %ymm15      # ymm15 = ymm12[1,0,3,2]
	vmulpd	%ymm13, %ymm15, %ymm7
	vfmaddsub231pd	%ymm14, %ymm12, %ymm7   # ymm7 = (ymm12 * ymm14) +/- ymm7
	vmovupd	(%rbx,%rdx), %ymm12
	addq	%rbx, %rdx
	vmovupd	(%rax,%r11), %ymm13
	vmovddup	%ymm13, %ymm14                  # ymm14 = ymm13[0,0,2,2]
	vshufpd	$15, %ymm13, %ymm13, %ymm13     # ymm13 = ymm13[1,1,3,3]
	vshufpd	$5, %ymm12, %ymm12, %ymm15      # ymm15 = ymm12[1,0,3,2]
	vmulpd	%ymm13, %ymm15, %ymm13
	vfmaddsub231pd	%ymm14, %ymm12, %ymm13  # ymm13 = (ymm12 * ymm14) +/- ymm13
	vmovupd	(%rbx,%rdx), %ymm12
	vmovupd	(%r10,%r11), %ymm14
	vmovddup	%ymm14, %ymm15                  # ymm15 = ymm14[0,0,2,2]
	vshufpd	$15, %ymm14, %ymm14, %ymm14     # ymm14 = ymm14[1,1,3,3]
	vshufpd	$5, %ymm12, %ymm12, %ymm5       # ymm5 = ymm12[1,0,3,2]
	vmulpd	%ymm5, %ymm14, %ymm5
	vfmaddsub231pd	%ymm15, %ymm12, %ymm5   # ymm5 = (ymm12 * ymm15) +/- ymm5
	vaddpd	%ymm5, %ymm10, %ymm14
	vsubpd	%ymm5, %ymm10, %ymm10
	vaddpd	%ymm14, %ymm9, %ymm5
	vaddpd	%ymm13, %ymm11, %ymm15
	vsubpd	%ymm13, %ymm11, %ymm12
	vaddpd	%ymm5, %ymm15, %ymm5
	vaddpd	%ymm7, %ymm1, %ymm13
	vsubpd	%ymm7, %ymm1, %ymm11
	vaddpd	%ymm5, %ymm13, %ymm1
	vmovupd	%ymm1, (%rdi,%r11)
	vmulpd	%ymm8, %ymm14, %ymm1
	vaddpd	%ymm1, %ymm9, %ymm1
	vbroadcastsd	.LCPI42_1(%rip), %ymm5  # ymm5 = [7.818314824680298E-1,7.818314824680298E-1,7.818314824680298E-1,7.818314824680298E-1]
	vmulpd	%ymm5, %ymm10, %ymm5
	vmulpd	%ymm3, %ymm15, %ymm7
	vaddpd	%ymm7, %ymm1, %ymm1
	vmulpd	%ymm4, %ymm12, %ymm7
	vaddpd	%ymm7, %ymm5, %ymm5
	vmulpd	%ymm6, %ymm13, %ymm7
	vaddpd	%ymm7, %ymm1, %ymm1
	vbroadcastsd	.LCPI42_5(%rip), %ymm7  # ymm7 = [4.3388373911755812E-1,4.3388373911755812E-1,4.3388373911755812E-1,4.3388373911755812E-1]
	vmulpd	%ymm7, %ymm11, %ymm7
	vaddpd	%ymm7, %ymm5, %ymm5
	vxorpd	%ymm0, %ymm5, %ymm5
	vshufps	$78, %ymm5, %ymm5, %ymm5        # ymm5 = ymm5[2,3,0,1,6,7,4,5]
	vaddpd	%ymm5, %ymm1, %ymm7
	vmovupd	%ymm7, (%r14,%r11)
	vsubpd	%ymm5, %ymm1, %ymm1
	vmovupd	%ymm1, (%r10,%r11)
	vmulpd	%ymm3, %ymm14, %ymm1
	vaddpd	%ymm1, %ymm9, %ymm1
	vmulpd	%ymm4, %ymm10, %ymm5
	vmulpd	%ymm6, %ymm15, %ymm7
	vaddpd	%ymm7, %ymm1, %ymm1
	vbroadcastsd	.LCPI42_7(%rip), %ymm7  # ymm7 = [-4.3388373911755812E-1,-4.3388373911755812E-1,-4.3388373911755812E-1,-4.3388373911755812E-1]
	vmulpd	%ymm7, %ymm12, %ymm7
	vaddpd	%ymm7, %ymm5, %ymm5
	vmulpd	%ymm8, %ymm13, %ymm7
	vaddpd	%ymm7, %ymm1, %ymm1
	vmulpd	%ymm2, %ymm11, %ymm7
	vaddpd	%ymm7, %ymm5, %ymm5
	vxorpd	%ymm0, %ymm5, %ymm5
	vshufps	$78, %ymm5, %ymm5, %ymm5        # ymm5 = ymm5[2,3,0,1,6,7,4,5]
	vaddpd	%ymm5, %ymm1, %ymm7
	vmovupd	%ymm7, (%r15,%r11)
	vsubpd	%ymm5, %ymm1, %ymm1
	vmovupd	%ymm1, (%rax,%r11)
	vmulpd	%ymm6, %ymm14, %ymm1
	vaddpd	%ymm1, %ymm9, %ymm1
	vmulpd	%ymm8, %ymm15, %ymm5
	vaddpd	%ymm5, %ymm1, %ymm1
	vbroadcastsd	.LCPI42_5(%rip), %ymm5  # ymm5 = [4.3388373911755812E-1,4.3388373911755812E-1,4.3388373911755812E-1,4.3388373911755812E-1]
	vmulpd	%ymm5, %ymm10, %ymm5
	vmulpd	%ymm2, %ymm12, %ymm7
	vaddpd	%ymm7, %ymm5, %ymm5
	vmulpd	%ymm3, %ymm13, %ymm7
	vaddpd	%ymm7, %ymm1, %ymm1
	vmulpd	%ymm4, %ymm11, %ymm7
	vaddpd	%ymm7, %ymm5, %ymm5
	vxorpd	%ymm0, %ymm5, %ymm5
	vshufps	$78, %ymm5, %ymm5, %ymm5        # ymm5 = ymm5[2,3,0,1,6,7,4,5]
	vaddpd	%ymm5, %ymm1, %ymm7
	vmovupd	%ymm7, (%r12,%r11)
	vsubpd	%ymm5, %ymm1, %ymm1
	vmovupd	%ymm1, (%r13,%r11)
	addq	$2, %rcx
	addq	$32, %r10
	addq	$32, %r14
	addq	$32, %r15
	addq	$32, %r12
	addq	$32, %r13
	addq	$32, %rax
	addq	$32, %rdi
	addq	$32, %rsi
	cmpq	%r8, %rcx
	jb	.LBB42_3
.LBB42_4:
	testq	%r9, %r9
	movq	16(%rsp), %rdx                  # 8-byte Reload
	je	.LBB42_6
# %bb.5:
	shlq	$4, %r8
	movq	40(%rsp), %r11                  # 8-byte Reload
	leaq	(%r11,%r8), %rcx
	movq	24(%rsp), %rax                  # 8-byte Reload
	leaq	(%rax,%r8), %r10
	vmovsd	(%r11,%r8), %xmm0               # xmm0 = mem[0],zero
	vmovsd	%xmm0, 128(%rsp)                # 8-byte Spill
	vmovsd	(%rax,%r8), %xmm0               # xmm0 = mem[0],zero
	vmovsd	8(%rax,%r8), %xmm1              # xmm1 = mem[0],zero
	movq	%rdx, %rdi
	shlq	$4, %rdi
	vmovsd	(%rdi,%rcx), %xmm3              # xmm3 = mem[0],zero
	vmovsd	8(%rdi,%rcx), %xmm2             # xmm2 = mem[0],zero
	vmulsd	%xmm1, %xmm2, %xmm10
	vfmsub231sd	%xmm0, %xmm3, %xmm10    # xmm10 = (xmm3 * xmm0) - xmm10
	vmulsd	%xmm2, %xmm0, %xmm4
	vfmadd231sd	%xmm1, %xmm3, %xmm4     # xmm4 = (xmm3 * xmm1) + xmm4
	vmovsd	(%rdi,%r10), %xmm0              # xmm0 = mem[0],zero
	vmovsd	8(%rdi,%r10), %xmm1             # xmm1 = mem[0],zero
	movq	%rdx, %rsi
	shlq	$5, %rsi
	vmovsd	(%rsi,%rcx), %xmm5              # xmm5 = mem[0],zero
	vmovsd	8(%rsi,%rcx), %xmm6             # xmm6 = mem[0],zero
	vmulsd	%xmm1, %xmm6, %xmm3
	vfmsub231sd	%xmm0, %xmm5, %xmm3     # xmm3 = (xmm5 * xmm0) - xmm3
	vmulsd	%xmm6, %xmm0, %xmm7
	vfmadd231sd	%xmm1, %xmm5, %xmm7     # xmm7 = (xmm5 * xmm1) + xmm7
	vmovsd	(%rsi,%r10), %xmm0              # xmm0 = mem[0],zero
	vmovsd	8(%rsi,%r10), %xmm1             # xmm1 = mem[0],zero
	leaq	(%rdi,%rdi,2), %rax
	vmovsd	(%rcx,%rax), %xmm5              # xmm5 = mem[0],zero
	vmovsd	8(%rcx,%rax), %xmm6             # xmm6 = mem[0],zero
	vmulsd	%xmm1, %xmm6, %xmm12
	vfmsub231sd	%xmm0, %xmm5, %xmm12    # xmm12 = (xmm5 * xmm0) - xmm12
	vmulsd	%xmm6, %xmm0, %xmm14
	vfmadd231sd	%xmm1, %xmm5, %xmm14    # xmm14 = (xmm5 * xmm1) + xmm14
	vmovsd	(%r10,%rax), %xmm0              # xmm0 = mem[0],zero
	vmovsd	8(%r10,%rax), %xmm1             # xmm1 = mem[0],zero
	shlq	$6, %rdx
	vmovsd	(%rdx,%rcx), %xmm5              # xmm5 = mem[0],zero
	vmovsd	8(%rdx,%rcx), %xmm6             # xmm6 = mem[0],zero
	vmulsd	%xmm1, %xmm6, %xmm15
	vfmsub231sd	%xmm0, %xmm5, %xmm15    # xmm15 = (xmm5 * xmm0) - xmm15
	vmulsd	%xmm6, %xmm0, %xmm2
	vfmadd231sd	%xmm1, %xmm5, %xmm2     # xmm2 = (xmm5 * xmm1) + xmm2
	vmovsd	(%rdx,%r10), %xmm1              # xmm1 = mem[0],zero
	vmovsd	8(%rdx,%r10), %xmm5             # xmm5 = mem[0],zero
	leaq	(%rdi,%rdi,4), %r9
	vmovsd	(%rcx,%r9), %xmm6               # xmm6 = mem[0],zero
	vmovsd	8(%rcx,%r9), %xmm8              # xmm8 = mem[0],zero
	vmulsd	%xmm5, %xmm8, %xmm13
	vfmsub231sd	%xmm1, %xmm6, %xmm13    # xmm13 = (xmm6 * xmm1) - xmm13
	vmulsd	%xmm1, %xmm8, %xmm8
	vfmadd231sd	%xmm5, %xmm6, %xmm8     # xmm8 = (xmm6 * xmm5) + xmm8
	vmovsd	(%r10,%r9), %xmm1               # xmm1 = mem[0],zero
	vmovsd	8(%r10,%r9), %xmm5              # xmm5 = mem[0],zero
	movq	32(%rsp), %r10                  # 8-byte Reload
	shlq	$5, %r10
	vmovsd	(%r10,%rcx), %xmm6              # xmm6 = mem[0],zero
	vmovsd	8(%r10,%rcx), %xmm9             # xmm9 = mem[0],zero
	vmulsd	%xmm5, %xmm9, %xmm11
	vfmsub231sd	%xmm1, %xmm6, %xmm11    # xmm11 = (xmm6 * xmm1) - xmm11
	vmulsd	%xmm1, %xmm9, %xmm0
	vmovsd	8(%r11,%r8), %xmm9              # xmm9 = mem[0],zero
	vfmadd231sd	%xmm5, %xmm6, %xmm0     # xmm0 = (xmm6 * xmm5) + xmm0
	vaddsd	%xmm11, %xmm10, %xmm5
	vaddsd	%xmm0, %xmm4, %xmm6
	vsubsd	%xmm11, %xmm10, %xmm1
	vmovsd	%xmm1, 80(%rsp)                 # 8-byte Spill
	vsubsd	%xmm0, %xmm4, %xmm0
	vmovsd	%xmm0, 112(%rsp)                # 8-byte Spill
	vmovsd	128(%rsp), %xmm1                # 8-byte Reload
                                        # xmm1 = mem[0],zero
	vaddsd	%xmm5, %xmm1, %xmm0
	vmovapd	%xmm5, %xmm4
	vmovsd	%xmm5, 120(%rsp)                # 8-byte Spill
	vaddsd	%xmm3, %xmm13, %xmm10
	vsubsd	%xmm13, %xmm3, %xmm3
	vmovsd	%xmm3, 88(%rsp)                 # 8-byte Spill
	vaddsd	%xmm7, %xmm8, %xmm13
	vsubsd	%xmm8, %xmm7, %xmm5
	vmovsd	%xmm5, 16(%rsp)                 # 8-byte Spill
	vaddsd	%xmm15, %xmm12, %xmm5
	vsubsd	%xmm15, %xmm12, %xmm7
	vmovsd	%xmm7, 32(%rsp)                 # 8-byte Spill
	vaddsd	%xmm2, %xmm14, %xmm7
	vsubsd	%xmm2, %xmm14, %xmm2
	vmovsd	%xmm2, 24(%rsp)                 # 8-byte Spill
	vmovapd	%xmm9, %xmm12
	vmovsd	%xmm6, 96(%rsp)                 # 8-byte Spill
	vaddsd	%xmm6, %xmm9, %xmm2
	vaddsd	%xmm0, %xmm10, %xmm0
	vmovapd	%xmm10, %xmm8
	vmovsd	%xmm13, 104(%rsp)               # 8-byte Spill
	vaddsd	%xmm2, %xmm13, %xmm2
	vaddsd	%xmm5, %xmm0, %xmm0
	vmovsd	%xmm5, 64(%rsp)                 # 8-byte Spill
	vaddsd	%xmm7, %xmm2, %xmm2
	vmovsd	%xmm7, 56(%rsp)                 # 8-byte Spill
	vmovsd	%xmm0, (%r11,%r8)
	vmovsd	%xmm2, 8(%r11,%r8)
	vmovsd	.LCPI42_0(%rip), %xmm11         # xmm11 = [6.2348980185873348E-1,0.0E+0]
	vmulsd	%xmm4, %xmm11, %xmm0
	vaddsd	%xmm0, %xmm1, %xmm0
	vmovsd	.LCPI42_2(%rip), %xmm1          # xmm1 = [-2.2252093395631439E-1,0.0E+0]
	vmulsd	%xmm1, %xmm10, %xmm2
	vmovsd	%xmm10, 72(%rsp)                # 8-byte Spill
	vaddsd	%xmm2, %xmm0, %xmm10
	vmulsd	%xmm6, %xmm11, %xmm0
	vaddsd	%xmm0, %xmm9, %xmm0
	vmovsd	%xmm9, 48(%rsp)                 # 8-byte Spill
	vmulsd	%xmm1, %xmm13, %xmm2
	vmovapd	%xmm1, %xmm13
	vaddsd	%xmm2, %xmm0, %xmm1
	vmovsd	.LCPI42_1(%rip), %xmm2          # xmm2 = [7.818314824680298E-1,0.0E+0]
	vmovsd	80(%rsp), %xmm14                # 8-byte Reload
                                        # xmm14 = mem[0],zero
	vmulsd	%xmm2, %xmm14, %xmm0
	vmovsd	.LCPI42_3(%rip), %xmm6          # xmm6 = [9.7492791218182361E-1,0.0E+0]
	vmulsd	%xmm6, %xmm3, %xmm4
	vaddsd	%xmm4, %xmm0, %xmm4
	vmovsd	112(%rsp), %xmm15               # 8-byte Reload
                                        # xmm15 = mem[0],zero
	vmulsd	%xmm2, %xmm15, %xmm0
	vmovsd	16(%rsp), %xmm3                 # 8-byte Reload
                                        # xmm3 = mem[0],zero
	vmulsd	%xmm6, %xmm3, %xmm2
	vaddsd	%xmm2, %xmm0, %xmm2
	vmovsd	.LCPI42_4(%rip), %xmm11         # xmm11 = [-9.0096886790241915E-1,0.0E+0]
	vmulsd	%xmm5, %xmm11, %xmm6
	vaddsd	%xmm6, %xmm10, %xmm6
	vmulsd	%xmm7, %xmm11, %xmm10
	vaddsd	%xmm1, %xmm10, %xmm1
	vmovsd	.LCPI42_5(%rip), %xmm5          # xmm5 = [4.3388373911755812E-1,0.0E+0]
	vmovsd	24(%rsp), %xmm0                 # 8-byte Reload
                                        # xmm0 = mem[0],zero
	vmulsd	%xmm5, %xmm0, %xmm10
	vaddsd	%xmm2, %xmm10, %xmm2
	vaddsd	%xmm6, %xmm2, %xmm10
	vmovsd	%xmm10, (%rdi,%rcx)
	vmovsd	32(%rsp), %xmm9                 # 8-byte Reload
                                        # xmm9 = mem[0],zero
	vmulsd	%xmm5, %xmm9, %xmm10
	vaddsd	%xmm4, %xmm10, %xmm4
	vsubsd	%xmm4, %xmm1, %xmm10
	vmovsd	%xmm10, 8(%rdi,%rcx)
	vsubsd	%xmm2, %xmm6, %xmm2
	vaddsd	%xmm1, %xmm4, %xmm1
	vmovsd	%xmm2, (%r10,%rcx)
	vmovsd	%xmm1, 8(%r10,%rcx)
	vmovapd	%xmm13, %xmm4
	vmovsd	120(%rsp), %xmm13               # 8-byte Reload
                                        # xmm13 = mem[0],zero
	vmulsd	%xmm4, %xmm13, %xmm1
	vaddsd	128(%rsp), %xmm1, %xmm1         # 8-byte Folded Reload
	vmulsd	%xmm11, %xmm8, %xmm2
	vaddsd	%xmm2, %xmm1, %xmm1
	vmovsd	96(%rsp), %xmm5                 # 8-byte Reload
                                        # xmm5 = mem[0],zero
	vmulsd	%xmm4, %xmm5, %xmm2
	vaddsd	%xmm2, %xmm12, %xmm2
	vmovsd	104(%rsp), %xmm12               # 8-byte Reload
                                        # xmm12 = mem[0],zero
	vmulsd	%xmm11, %xmm12, %xmm4
	vaddsd	%xmm4, %xmm2, %xmm2
	vmovsd	.LCPI42_3(%rip), %xmm7          # xmm7 = [9.7492791218182361E-1,0.0E+0]
	vmulsd	%xmm7, %xmm14, %xmm4
	vmovsd	.LCPI42_7(%rip), %xmm6          # xmm6 = [-4.3388373911755812E-1,0.0E+0]
	vmovsd	88(%rsp), %xmm11                # 8-byte Reload
                                        # xmm11 = mem[0],zero
	vmulsd	%xmm6, %xmm11, %xmm10
	vaddsd	%xmm4, %xmm10, %xmm4
	vmulsd	%xmm6, %xmm3, %xmm6
	vmulsd	%xmm7, %xmm15, %xmm10
	vaddsd	%xmm6, %xmm10, %xmm6
	vmovsd	.LCPI42_0(%rip), %xmm7          # xmm7 = [6.2348980185873348E-1,0.0E+0]
	vmovsd	64(%rsp), %xmm3                 # 8-byte Reload
                                        # xmm3 = mem[0],zero
	vmulsd	%xmm7, %xmm3, %xmm10
	vaddsd	%xmm1, %xmm10, %xmm1
	vmovsd	56(%rsp), %xmm8                 # 8-byte Reload
                                        # xmm8 = mem[0],zero
	vmulsd	%xmm7, %xmm8, %xmm10
	vaddsd	%xmm2, %xmm10, %xmm2
	vmovsd	.LCPI42_8(%rip), %xmm10         # xmm10 = [-7.818314824680298E-1,0.0E+0]
	vmulsd	%xmm0, %xmm10, %xmm7
	vaddsd	%xmm7, %xmm6, %xmm6
	vaddsd	%xmm1, %xmm6, %xmm7
	vmovsd	%xmm7, (%rsi,%rcx)
	vmulsd	%xmm10, %xmm9, %xmm7
	vaddsd	%xmm7, %xmm4, %xmm4
	vsubsd	%xmm4, %xmm2, %xmm7
	vmovsd	%xmm7, 8(%rsi,%rcx)
	vsubsd	%xmm6, %xmm1, %xmm1
	vaddsd	%xmm2, %xmm4, %xmm2
	vmovsd	%xmm1, (%rcx,%r9)
	vmovsd	%xmm2, 8(%rcx,%r9)
	vmovsd	.LCPI42_4(%rip), %xmm0          # xmm0 = [-9.0096886790241915E-1,0.0E+0]
	vmulsd	%xmm0, %xmm13, %xmm1
	vmulsd	%xmm0, %xmm5, %xmm0
	vaddsd	128(%rsp), %xmm1, %xmm1         # 8-byte Folded Reload
	vaddsd	48(%rsp), %xmm0, %xmm0          # 8-byte Folded Reload
	vmovsd	.LCPI42_5(%rip), %xmm4          # xmm4 = [4.3388373911755812E-1,0.0E+0]
	vmulsd	%xmm4, %xmm14, %xmm2
	vmulsd	%xmm4, %xmm15, %xmm4
	vmovsd	.LCPI42_0(%rip), %xmm6          # xmm6 = [6.2348980185873348E-1,0.0E+0]
	vmulsd	72(%rsp), %xmm6, %xmm5          # 8-byte Folded Reload
	vaddsd	%xmm5, %xmm1, %xmm1
	vmulsd	%xmm6, %xmm12, %xmm5
	vaddsd	%xmm5, %xmm0, %xmm0
	vmulsd	%xmm10, %xmm11, %xmm5
	vaddsd	%xmm5, %xmm2, %xmm2
	vmulsd	16(%rsp), %xmm10, %xmm5         # 8-byte Folded Reload
	vaddsd	%xmm5, %xmm4, %xmm4
	vmovsd	.LCPI42_2(%rip), %xmm6          # xmm6 = [-2.2252093395631439E-1,0.0E+0]
	vmulsd	%xmm6, %xmm3, %xmm5
	vaddsd	%xmm5, %xmm1, %xmm1
	vmulsd	%xmm6, %xmm8, %xmm5
	vaddsd	%xmm5, %xmm0, %xmm0
	vmovsd	.LCPI42_3(%rip), %xmm3          # xmm3 = [9.7492791218182361E-1,0.0E+0]
	vmulsd	%xmm3, %xmm9, %xmm5
	vaddsd	%xmm5, %xmm2, %xmm2
	vmulsd	24(%rsp), %xmm3, %xmm3          # 8-byte Folded Reload
	vaddsd	%xmm3, %xmm4, %xmm3
	vaddsd	%xmm1, %xmm3, %xmm4
	vmovsd	%xmm4, (%rcx,%rax)
	vsubsd	%xmm2, %xmm0, %xmm4
	vmovsd	%xmm4, 8(%rcx,%rax)
	vsubsd	%xmm3, %xmm1, %xmm1
	vaddsd	%xmm0, %xmm2, %xmm0
	vmovsd	%xmm1, (%rdx,%rcx)
	vmovsd	%xmm0, 8(%rdx,%rcx)
.LBB42_6:
	leaq	-40(%rbp), %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	vzeroupper
	retq
.Lfunc_end42:
	.size	_ZN6micron4math3fft6__impl7__stageILm7ELb0ETkNS1_10fft_scalarEdEEvPNS0_7complexIT1_EEPKS6_mmm, .Lfunc_end42-_ZN6micron4math3fft6__impl7__stageILm7ELb0ETkNS1_10fft_scalarEdEEvPNS0_7complexIT1_EEPKS6_mmm
