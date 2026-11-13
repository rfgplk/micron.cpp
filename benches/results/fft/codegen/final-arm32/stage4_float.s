_ZN6micron4math3fft6__impl7__stageILj4ELb0EfEEvPNS0_7complexIT1_EEPKS6_jjj:
	.fnstart
.LFB7340:
	@ args = 4, pretend = 0, frame = 32
	@ frame_needed = 1, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	vpush.64	{d8, d9, d10, d11, d12, d13}
	sub	sp, sp, #36
	add	r7, sp, #0
	ldr	r10, [r7, #120]
	cmp	r3, r10
	bcs	.L1157
	sub	lr, r10, r3
	mov	r5, r0
	and	r8, lr, #3
	mov	r6, r1
	sub	ip, r10, r8
	mov	r4, r2
	cmp	r3, ip
	bcs	.L1159
	mvn	r9, r3
	mvn	r1, lr
	add	r9, r9, ip
	and	fp, r1, #3
	add	r2, r3, r2, lsl #1
	add	fp, fp, r10
	lsr	r0, r9, #2
	lsl	r10, r4, #3
	add	r9, r0, #1
	adds	r0, r3, #4
	vmov.i32	q13, #2147483648  @ v4si
	add	r2, r5, r2, lsl #3
	cmp	r0, fp
	it	hi
	movhi	r9, #1
	add	r1, r6, r3, lsl #3
	strd	r8, lr, [r7, #20]
	lsls	r3, r4, #4
	strd	r5, ip, [r7, #12]
	str	r3, [r7, #28]
	strd	r4, r6, [r7, #4]
.L1160:
	ldr	r4, [r7, #28]
	mov	r8, r1
	add	r3, r10, r1
	mov	r5, r2
	sub	lr, r2, r4
	adds	r1, r1, #32
	add	ip, r10, lr
	mov	r6, lr
	add	fp, ip, r4
	mov	r4, ip
	vld1.8	{q2}, [r8]!
	mov	r0, fp
	vld1.8	{q10}, [r4]!
	subs	r9, r9, #1
	vld1.8	{q9}, [r8]
	add	r8, r3, r10
	vld1.8	{q11}, [r4]
	vld1.8	{q14}, [r5]!
	vld1.8	{q12}, [r6]!
	vld1.8	{q5}, [r3]!
	vld1.8	{q1}, [r8]!
	vuzp.32	q2, q9
	vuzp.32	q10, q11
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q8, q10, q2
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q10, q10, q9
@ 0 "" 2
@ 50 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmls.f32 q8, q11, q9
@ 0 "" 2
	.thumb
	.syntax unified
	vld1.8	{q9}, [r0]!
	.syntax unified
@ 43 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmla.f32 q10, q11, q2
@ 0 "" 2
	.thumb
	.syntax unified
	vld1.8	{q15}, [r5]
	vld1.8	{q3}, [r6]
	vld1.8	{q2}, [r0]
	vld1.8	{q6}, [r3]
	vld1.8	{q0}, [r8]
	vuzp.32	q14, q15
	vuzp.32	q5, q6
	vuzp.32	q1, q0
	vuzp.32	q12, q3
	vuzp.32	q9, q2
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q4, q14, q5
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q11, q14, q6
@ 0 "" 2
@ 50 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmls.f32 q4, q15, q6
@ 0 "" 2
@ 43 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmla.f32 q11, q15, q5
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q14, q12, q4
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q15, q3, q11
@ 0 "" 2
@ 28 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vsub.f32 q5, q3, q11
@ 0 "" 2
@ 28 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vsub.f32 q12, q12, q4
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q3, q9, q1
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q9, q9, q0
@ 0 "" 2
@ 50 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmls.f32 q3, q2, q0
@ 0 "" 2
@ 43 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmla.f32 q9, q2, q1
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q6, q8, q3
@ 0 "" 2
@ 28 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vsub.f32 q1, q10, q9
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q2, q10, q9
@ 0 "" 2
@ 28 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vsub.f32 q8, q8, q3
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q0, q14, q6
@ 0 "" 2
	.thumb
	.syntax unified
	veor	q4, q8, q13
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q10, q15, q2
@ 0 "" 2
@ 28 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vsub.f32 q11, q14, q6
@ 0 "" 2
	.thumb
	.syntax unified
	vzip.32	q0, q10
	.syntax unified
@ 28 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vsub.f32 q14, q15, q2
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q3, q12, q1
@ 0 "" 2
	.thumb
	.syntax unified
	vzip.32	q11, q14
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q15, q5, q4
@ 0 "" 2
@ 28 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vsub.f32 q12, q12, q1
@ 0 "" 2
	.thumb
	.syntax unified
	vzip.32	q3, q15
	vst1.8	{q0}, [lr]
	.syntax unified
@ 28 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vsub.f32 q5, q5, q4
@ 0 "" 2
	.thumb
	.syntax unified
	vzip.32	q12, q5
	vst1.8	{q10}, [r6]
	vst1.8	{q11}, [r2]
	add	r2, r2, #32
	vst1.8	{q14}, [r5]
	vst1.8	{q3}, [ip]
	vst1.8	{q15}, [r4]
	vst1.8	{q12}, [fp]
	vst1.8	{q5}, [r0]
	bne	.L1160
	ldrd	r8, lr, [r7, #20]
	ldrd	r5, ip, [r7, #12]
	ldrd	r4, r6, [r7, #4]
.L1159:
	cmp	r8, #0
	beq	.L1157
	add	r2, ip, r4
	lsl	r1, ip, #3
	adds	r0, r4, r2
	lsl	r9, r4, #4
	lsls	r3, r2, #3
	adds	r2, r4, r0
	lsls	r0, r0, #3
	tst	lr, #2
	add	r10, r5, r2, lsl #3
	add	r2, r5, r3
	add	r3, r3, r6
	add	fp, r5, r0
	add	r0, r6, r0
	vldr.32	s5, [r10, #4]
	vldr.32	s1, [r3, #4]
	vldr.32	s7, [r3]
	add	r3, r6, r1
	vldr.32	s17, [r0, #4]
	vldr.32	s10, [r2, #4]
	vldr.32	s15, [r3, #4]
	vldr.32	s3, [fp, #4]
	vmul.f32	s22, s5, s17
	vldr.32	s9, [r10]
	vldr.32	s11, [r2]
	vmul.f32	s14, s10, s15
	vldr.32	s6, [fp]
	vmul.f32	s19, s3, s1
	vldr.32	s18, [r0]
	vmul.f32	s23, s9, s17
	vldr.32	s13, [r3]
	vmul.f32	s2, s11, s15
	vmul.f32	s21, s6, s1
	add	r3, r5, r1
	vmla.f32	s21, s3, s7
	vmla.f32	s23, s5, s18
	vmla.f32	s2, s10, s13
	vnmls.f32	s14, s11, s13
	vnmls.f32	s19, s6, s7
	vnmls.f32	s22, s9, s18
	vldr.32	s8, [r3]
	mov	r0, #24
	vldr.32	s4, [r3, #4]
	add	r10, r3, r9
	mul	r0, r4, r0
	vadd.f32	s16, s21, s4
	vsub.f32	s12, s4, s21
	vadd.f32	s25, s23, s2
	vsub.f32	s26, s2, s23
	vadd.f32	s27, s22, s14
	vadd.f32	s0, s19, s8
	vsub.f32	s24, s14, s22
	vsub.f32	s20, s8, s19
	vadd.f32	s5, s25, s16
	vsub.f32	s10, s16, s25
	vadd.f32	s1, s27, s0
	vsub.f32	s9, s0, s27
	vadd.f32	s7, s26, s20
	vsub.f32	s11, s12, s24
	vsub.f32	s15, s20, s26
	vadd.f32	s13, s24, s12
	vstr.32	s5, [r3, #4]
	vstr.32	s1, [r3]
	add	r3, r3, r0
	vstr.32	s9, [r10]
	vstr.32	s10, [r10, #4]
	vstr.32	s7, [r2]
	vstr.32	s11, [r2, #4]
	vstr.32	s15, [r3]
	vstr.32	s13, [r3, #4]
	beq	.L1157
	add	lr, ip, #1
	add	fp, r1, #8
	add	lr, lr, r4
	cmp	r8, #3
	add	r2, r4, lr
	add	r10, r4, r2
	lsl	r3, lr, #3
	lsl	r2, r2, #3
	add	lr, r5, r10, lsl #3
	add	r10, r5, r3
	add	r3, r3, r6
	vldr.32	s3, [lr, #4]
	vldr.32	s6, [lr]
	add	lr, r5, r2
	vldr.32	s17, [r3, #4]
	add	r2, r2, r6
	vldr.32	s18, [r3]
	add	r3, r6, fp
	vldr.32	s2, [r10, #4]
	add	fp, fp, r5
	vldr.32	s27, [lr, #4]
	vldr.32	s21, [r3, #4]
	vldr.32	s4, [r2, #4]
	vldr.32	s19, [r10]
	vmul.f32	s16, s27, s17
	vldr.32	s24, [lr]
	vmul.f32	s14, s2, s21
	vmul.f32	s8, s3, s4
	vldr.32	s22, [r3]
	vldr.32	s0, [r2]
	vmul.f32	s26, s19, s21
	vmul.f32	s20, s24, s17
	vmul.f32	s12, s6, s4
	vmla.f32	s26, s2, s22
	vnmls.f32	s14, s19, s22
	vmla.f32	s20, s27, s18
	vmla.f32	s12, s3, s0
	vnmls.f32	s16, s24, s18
	vnmls.f32	s8, s6, s0
	vldr.32	s23, [fp]
	add	r2, fp, r9
	vldr.32	s25, [fp, #4]
	vadd.f32	s9, s12, s26
	vadd.f32	s1, s20, s25
	vadd.f32	s15, s16, s23
	vadd.f32	s10, s8, s14
	vsub.f32	s11, s25, s20
	vsub.f32	s7, s26, s12
	vsub.f32	s13, s23, s16
	vsub.f32	s3, s14, s8
	vadd.f32	s5, s9, s1
	vsub.f32	s6, s1, s9
	vadd.f32	s18, s10, s15
	vsub.f32	s19, s15, s10
	vadd.f32	s2, s7, s13
	vsub.f32	s17, s13, s7
	vsub.f32	s21, s11, s3
	vadd.f32	s22, s3, s11
	vstr.32	s5, [fp, #4]
	vstr.32	s18, [fp]
	add	fp, fp, r0
	vstr.32	s19, [r2]
	vstr.32	s6, [r2, #4]
	vstr.32	s2, [r10]
	vstr.32	s21, [r10, #4]
	vstr.32	s17, [fp]
	vstr.32	s22, [fp, #4]
	bne	.L1157
	add	r2, ip, #2
	adds	r1, r1, #16
	add	r2, r2, r4
	adds	r3, r6, r1
	add	r8, r4, r2
	add	r1, r1, r5
	add	r4, r4, r8
	lsls	r2, r2, #3
	lsl	ip, r8, #3
	vldr.32	s23, [r3, #4]
	vldr.32	s25, [r3]
	add	fp, r5, r4, lsl #3
	adds	r3, r5, r2
	add	r2, r2, r6
	add	r5, r5, ip
	add	r6, r6, ip
	vldr.32	s27, [fp, #4]
	add	r9, r9, r1
	vldr.32	s4, [r2, #4]
	add	r0, r0, r1
	vldr.32	s16, [r5, #4]
	vldr.32	s1, [r6, #4]
	vldr.32	s11, [r3, #4]
	vldr.32	s0, [r2]
	vmul.f32	s7, s4, s16
	vldr.32	s9, [r6]
	vmul.f32	s8, s1, s27
	vmul.f32	s12, s23, s11
	vldr.32	s24, [fp]
	vldr.32	s20, [r5]
	vmul.f32	s10, s0, s16
	vldr.32	s13, [r3]
	vmul.f32	s15, s25, s11
	vmul.f32	s3, s9, s27
	vnmls.f32	s8, s9, s24
	vmla.f32	s10, s4, s20
	vnmls.f32	s7, s0, s20
	vmla.f32	s3, s1, s24
	vnmls.f32	s12, s25, s13
	vmla.f32	s15, s23, s13
	vldr.32	s26, [r1]
	vldr.32	s14, [r1, #4]
	vsub.f32	s19, s26, s7
	vadd.f32	s2, s14, s10
	vsub.f32	s17, s14, s10
	vadd.f32	s18, s26, s7
	vadd.f32	s6, s12, s8
	vadd.f32	s21, s15, s3
	vsub.f32	s22, s12, s8
	vsub.f32	s23, s15, s3
	vadd.f32	s25, s18, s6
	vadd.f32	s5, s2, s21
	vsub.f32	s26, s18, s6
	vsub.f32	s27, s2, s21
	vadd.f32	s24, s19, s23
	vsub.f32	s4, s17, s22
	vsub.f32	s0, s19, s23
	vadd.f32	s14, s17, s22
	vstr.32	s25, [r1]
	vstr.32	s5, [r1, #4]
	vstr.32	s26, [r9]
	vstr.32	s27, [r9, #4]
	vstr.32	s24, [r3]
	vstr.32	s4, [r3, #4]
	vstr.32	s0, [r0]
	vstr.32	s14, [r0, #4]
.L1157:
	adds	r7, r7, #36
	mov	sp, r7
	@ sp needed
	vldm	sp!, {d8-d13}
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
	.cantunwind
	.fnend
	.size	_ZN6micron4math3fft6__impl7__stageILj4ELb0EfEEvPNS0_7complexIT1_EEPKS6_jjj, .-_ZN6micron4math3fft6__impl7__stageILj4ELb0EfEEvPNS0_7complexIT1_EEPKS6_jjj
