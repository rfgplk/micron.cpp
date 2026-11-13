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
