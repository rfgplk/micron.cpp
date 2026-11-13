_ZN6micron4math3fft6__impl7__stageILj7ELb0EfEEvPNS0_7complexIT1_EEPKS6_jjj:
	.fnstart
.LFB7342:
	@ args = 4, pretend = 0, frame = 192
	@ frame_needed = 1, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	vpush.64	{d8, d9, d10, d11, d12, d13, d14, d15}
	sub	sp, sp, #196
	add	r7, sp, #0
	ldr	r4, [r7, #296]
	str	r1, [r7, #16]
	cmp	r3, r4
	bcs	.L1189
	subs	r6, r4, r3
	mov	r5, r0
	and	r0, r6, #3
	mov	lr, r1
	subs	r1, r4, r0
	cmp	r3, r1
	bcs	.L1191
	mvn	ip, r3
	mvns	r6, r6
	add	ip, ip, r1
	and	r8, r6, #3
	add	r4, r8, r4
	add	r10, r3, #4
	lsr	r9, ip, #2
	lsls	r3, r3, #3
	add	r6, r9, #1
	str	r6, [r7, #100]
	movs	r6, #24
	lsl	r8, r2, #3
	add	r9, lr, r3
	strd	r1, r0, [r7, #8]
	strd	r2, r5, [r7]
	mul	r6, r6, r2
	str	r6, [r7, #28]
	ldr	r6, [r7, #100]
	cmp	r10, r4
	it	hi
	movhi	r6, #1
	add	r10, r5, r3
	lsls	r4, r2, #5
	lsls	r3, r2, #4
	str	r6, [r7, #100]
	str	r4, [r7, #20]
	str	r3, [r7, #24]
.L1192:
	add	r6, r8, r10
	add	r2, r8, r9
	add	r0, r6, r8
	add	r1, r2, r8
	add	r5, r0, r8
	add	r4, r1, r8
	add	fp, r5, r8
	mov	ip, r10
	vld1.8	{q11}, [r2]!
	str	r6, [r7, #152]
	vld1.8	{q9}, [r0]!
	vld1.8	{q12}, [r1]!
	vld1.8	{q0}, [r5]!
	vld1.8	{q13}, [r2]
	vld1.8	{q8}, [r0]
	add	r0, r4, r8
	vld1.8	{q10}, [r1]
	add	r1, fp, r8
	vld1.8	{q2}, [r4]!
	add	r3, r0, r8
	vld1.8	{q1}, [r5]
	add	r5, r1, r8
	vuzp.32	q11, q13
	str	r3, [r7, #176]
	vuzp.32	q12, q10
	ldr	r2, [r7, #20]
	vuzp.32	q0, q1
	ldr	r3, [r7, #28]
	vuzp.32	q9, q8
	subs	r2, r5, r2
	vld1.8	{q6}, [r0]!
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q14, q9, q11
@ 0 "" 2
	.thumb
	.syntax unified
	vld1.8	{q4}, [r4]
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q15, q0, q12
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q9, q9, q13
@ 0 "" 2
@ 50 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmls.f32 q15, q1, q10
@ 0 "" 2
@ 43 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmla.f32 q9, q8, q11
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q11, q0, q10
@ 0 "" 2
	.thumb
	.syntax unified
	vld1.8	{q10}, [r1]!
	.syntax unified
@ 43 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmla.f32 q11, q1, q12
@ 0 "" 2
	.thumb
	.syntax unified
	vld1.8	{q12}, [fp]!
	.syntax unified
@ 50 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmls.f32 q14, q8, q13
@ 0 "" 2
	.thumb
	.syntax unified
	vuzp.32	q2, q4
	ldr	r4, [r7, #176]
	vld1.8	{q7}, [r0]
	add	lr, r3, r2
	vld1.8	{q8}, [r1]
	vstr	d8, [r7, #120]
	vstr	d9, [r7, #128]
	vld1.8	{q3}, [fp]
	mov	fp, r9
	vuzp.32	q6, q7
	str	r5, [r7, #156]
	vuzp.32	q10, q8
	str	r2, [r7, #96]
	vld1.8	{q4}, [fp]!
	add	r9, r9, #32
	vld1.8	{q13}, [r6]!
	vstr	d14, [r7, #64]
	vstr	d15, [r7, #72]
	ldr	r2, [r7, #24]
	vld1.8	{q0}, [ip]!
	vstr	d16, [r7, #48]
	vstr	d17, [r7, #56]
	sub	r3, lr, r2
	vld1.8	{q5}, [fp]
	vstr	d12, [r7, #160]
	vstr	d13, [r7, #168]
	add	r2, r3, r8
	vld1.8	{q6}, [r4]!
	vld1.8	{q7}, [ip]
	vld1.8	{q1}, [r4]
	vuzp.32	q0, q7
	ldr	r1, [r7, #152]
	vld1.8	{q8}, [r5]!
	vuzp.32	q6, q1
	ldr	r0, [r7, #156]
	vuzp.32	q4, q5
	vstr	d14, [r7, #176]
	vstr	d15, [r7, #184]
	vuzp.32	q12, q3
	vstr	d12, [r7, #80]
	vstr	d13, [r7, #88]
	vmov	q6, q0  @ v4sf
	vstr	d2, [r7, #32]
	vstr	d3, [r7, #40]
	vld1.8	{q0}, [r6]
	vuzp.32	q13, q0
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q1, q13, q5
@ 0 "" 2
	.thumb
	.syntax unified
	vstr	d0, [r7, #136]
	vstr	d1, [r7, #144]
	vld1.8	{q0}, [r5]
	vmov	q7, q0  @ v4sf
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q0, q13, q4
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d26, [r7, #136]
	vldr	d27, [r7, #144]
	.syntax unified
@ 50 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmls.f32 q0, q13, q5
@ 0 "" 2
	.thumb
	.syntax unified
	vuzp.32	q8, q7
	.syntax unified
@ 43 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmla.f32 q1, q13, q4
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q5, q12, q2
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d8, [r7, #120]
	vldr	d9, [r7, #128]
	.syntax unified
@ 50 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmls.f32 q5, q3, q4
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d26, [r7, #64]
	vldr	d27, [r7, #72]
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q12, q12, q4
@ 0 "" 2
@ 43 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmla.f32 q12, q3, q2
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q3, q15, q5
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d4, [r7, #160]
	vldr	d5, [r7, #168]
	.syntax unified
@ 28 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vsub.f32 q15, q15, q5
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q4, q10, q2
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q5, q11, q12
@ 0 "" 2
	.thumb
	.syntax unified
	vstr	d30, [r7, #120]
	vstr	d31, [r7, #128]
	.syntax unified
@ 28 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vsub.f32 q11, q11, q12
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q10, q10, q13
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d24, [r7, #48]
	vldr	d25, [r7, #56]
	vstr	d22, [r7, #104]
	vstr	d23, [r7, #112]
	.syntax unified
@ 43 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmla.f32 q10, q12, q2
@ 0 "" 2
@ 50 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmls.f32 q4, q12, q13
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d4, [r7, #32]
	vldr	d5, [r7, #40]
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q12, q14, q4
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q13, q9, q10
@ 0 "" 2
@ 28 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vsub.f32 q11, q14, q4
@ 0 "" 2
@ 28 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vsub.f32 q14, q9, q10
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d18, [r7, #80]
	vldr	d19, [r7, #88]
	vstr	d28, [r7, #136]
	vstr	d29, [r7, #144]
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q4, q8, q9
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q8, q8, q2
@ 0 "" 2
@ 50 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmls.f32 q4, q7, q2
@ 0 "" 2
@ 43 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmla.f32 q8, q7, q9
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q2, q0, q4
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q15, q1, q8
@ 0 "" 2
@ 28 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vsub.f32 q10, q0, q4
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d14, [r7, #176]
	vldr	d15, [r7, #184]
	.syntax unified
@ 28 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vsub.f32 q8, q1, q8
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q0, q6, q2
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q4, q7, q15
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q1, q0, q12
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q0, q4, q13
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q14, q1, q3
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q1, q0, q5
@ 0 "" 2
	.thumb
	.syntax unified
	vzip.32	q14, q1
	vstr	d12, [r7, #160]
	vstr	d13, [r7, #168]
	vldr	d0, .L1201
	vldr	d1, .L1201+8
	vst1.8	{q14}, [r10]
	add	r10, r10, #32
	vst1.8	{q1}, [ip]
	vldr	d28, .L1201+16
	vldr	d29, .L1201+24
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q9, q2, q14
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q4, q15, q14
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d2, .L1201
	vldr	d3, .L1201+8
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q6, q6, q9
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q14, q8, q1
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d18, .L1201+32
	vldr	d19, .L1201+40
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q4, q7, q4
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d2, .L1201+32
	vldr	d3, .L1201+40
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q7, q10, q0
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q0, q12, q9
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q9, q13, q1
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q6, q6, q0
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q4, q4, q9
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d0, .L1201+48
	vldr	d1, .L1201+56
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q1, q11, q0
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d18, .L1201+48
	vldr	d19, .L1201+56
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q0, q7, q1
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d14, [r7, #136]
	vldr	d15, [r7, #144]
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q1, q7, q9
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d18, .L1201+64
	vldr	d19, .L1201+72
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q14, q14, q1
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q7, q3, q9
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d2, .L1201+64
	vldr	d3, .L1201+72
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q6, q6, q7
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q9, q5, q1
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d14, .L1201+80
	vldr	d15, .L1201+88
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q4, q4, q9
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d2, [r7, #120]
	vldr	d3, [r7, #128]
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q9, q1, q7
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q1, q0, q9
@ 0 "" 2
	.thumb
	.syntax unified
	vmov.i32	q0, #2147483648  @ v4si
	veor	q9, q1, q0
	vldr	d2, [r7, #104]
	vldr	d3, [r7, #112]
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q0, q4, q9
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q7, q1, q7
@ 0 "" 2
@ 28 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vsub.f32 q9, q4, q9
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q14, q14, q7
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q4, q6, q14
@ 0 "" 2
@ 28 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vsub.f32 q6, q6, q14
@ 0 "" 2
	.thumb
	.syntax unified
	vzip.32	q4, q0
	vzip.32	q6, q9
	vldr	d2, .L1201+64
	vldr	d3, .L1201+72
	vst1.8	{q4}, [r1]
	vldr	d14, .L1201+32
	vldr	d15, .L1201+40
	vst1.8	{q0}, [r6]
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q0, q2, q7
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d8, .L1201+48
	vldr	d9, .L1201+56
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q2, q2, q1
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q7, q15, q7
@ 0 "" 2
	.thumb
	.syntax unified
	vst1.8	{q6}, [r0]
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q15, q15, q1
@ 0 "" 2
	.thumb
	.syntax unified
	vst1.8	{q9}, [r5]
	ldr	r6, [r7, #96]
	vldr	d28, [r7, #160]
	vldr	d29, [r7, #168]
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q9, q14, q0
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q6, q14, q2
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d0, [r7, #176]
	vldr	d1, [r7, #184]
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q2, q0, q7
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q15, q0, q15
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d2, .L1201+48
	vldr	d3, .L1201+56
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q0, q10, q1
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d28, .L1201+80
	vldr	d29, .L1201+88
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q1, q8, q4
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q10, q10, q14
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d14, .L1201+80
	vldr	d15, .L1201+88
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q8, q8, q7
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d28, .L1201+64
	vldr	d29, .L1201+72
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q4, q12, q14
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d28, .L1201+16
	vldr	d29, .L1201+24
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q4, q9, q4
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q12, q12, q14
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d18, .L1201+64
	vldr	d19, .L1201+72
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q6, q6, q12
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q7, q13, q9
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q13, q13, q14
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q2, q2, q7
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q12, q15, q13
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d14, .L1201+96
	vldr	d15, .L1201+104
	vldr	d30, .L1201+112
	vldr	d31, .L1201+120
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q9, q11, q15
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q11, q11, q7
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q14, q0, q9
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q11, q10, q11
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d0, [r7, #136]
	vldr	d1, [r7, #144]
	vldr	d20, .L1201+112
	vldr	d21, .L1201+120
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q13, q0, q10
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q10, q1, q13
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q1, q0, q7
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d0, .L1201+16
	vldr	d1, .L1201+24
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q13, q5, q0
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q2, q2, q13
@ 0 "" 2
	.thumb
	.syntax unified
	vmov.i32	q13, #2147483648  @ v4si
	ldr	r5, [r7, #100]
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q7, q8, q1
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d18, .L1201+32
	vldr	d19, .L1201+40
	vldr	d16, .L1201+16
	vldr	d17, .L1201+24
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q15, q3, q8
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d2, .L1201+32
	vldr	d3, .L1201+40
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q4, q4, q15
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q3, q3, q9
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q5, q5, q1
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q6, q6, q3
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q0, q12, q5
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d16, .L1201+96
	vldr	d17, .L1201+104
	vldr	d24, [r7, #120]
	vldr	d25, [r7, #128]
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q15, q12, q8
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d18, .L1201+48
	vldr	d19, .L1201+56
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q14, q14, q15
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q3, q12, q9
@ 0 "" 2
	.thumb
	.syntax unified
	veor	q1, q14, q13
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q11, q11, q3
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d24, [r7, #104]
	vldr	d25, [r7, #112]
	.syntax unified
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q9, q12, q9
@ 0 "" 2
	.thumb
	.syntax unified
	veor	q5, q11, q13
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q7, q7, q9
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q11, q2, q1
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q13, q6, q7
@ 0 "" 2
@ 28 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vsub.f32 q2, q2, q1
@ 0 "" 2
	.thumb
	.syntax unified
	vldr	d16, .L1201+96
	vldr	d17, .L1201+104
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q1, q0, q5
@ 0 "" 2
@ 36 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vmul.f32 q15, q12, q8
@ 0 "" 2
	.thumb
	.syntax unified
	vzip.32	q13, q1
	.syntax unified
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q10, q10, q15
@ 0 "" 2
@ 28 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vsub.f32 q6, q6, q7
@ 0 "" 2
@ 20 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vadd.f32 q3, q4, q10
@ 0 "" 2
@ 28 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vsub.f32 q0, q0, q5
@ 0 "" 2
	.thumb
	.syntax unified
	vzip.32	q3, q11
	vzip.32	q6, q0
	.syntax unified
@ 28 "tests/build/../../src/math/fft/arch/packet_neon.hpp" 1
	vsub.f32 q4, q4, q10
@ 0 "" 2
	.thumb
	.syntax unified
	vzip.32	q4, q2
	vst1.8	{q3}, [r6]!
	vst1.8	{q11}, [r6]
	vst1.8	{q4}, [lr]!
	vst1.8	{q2}, [lr]
	vst1.8	{q13}, [r3]!
	vst1.8	{q1}, [r3]
	subs	r3, r5, #1
	vst1.8	{q6}, [r2]!
	str	r3, [r7, #100]
	vst1.8	{q0}, [r2]
	bne	.L1192
	ldr	r4, [r7, #296]
	ldrd	r1, r0, [r7, #8]
	ldrd	r2, r5, [r7]
.L1191:
	cmp	r0, #0
	beq	.L1189
	lsl	r10, r1, #3
	add	r1, r1, r2
	adds	r3, r2, r1
	add	r6, r5, r4, lsl #3
	str	r6, [r7, #156]
	lsls	r6, r1, #3
	adds	r1, r2, r3
	lsl	r9, r3, #3
	adds	r0, r2, r1
	add	r3, r5, r10
	adds	r4, r2, r0
	lsl	r8, r1, #3
	adds	r1, r2, r4
	add	r0, r5, r0, lsl #3
	sub	lr, r1, r2, lsl #1
	ldr	r2, [r7, #16]
	lsls	r4, r4, #3
	add	r1, r5, r1, lsl #3
	add	ip, r2, #4
	vldr.32	s22, .L1201+128
	add	fp, ip, lr, lsl #3
	vldr.32	s21, .L1201+132
	add	lr, r5, r6
	add	ip, r5, r9
	adds	r6, r2, r6
	vldr.32	s20, .L1201+136
	str	r6, [r7, #176]
	add	r10, r10, r2
	add	r6, r5, r8
	vldr.32	s31, .L1201+140
	add	r5, r5, r4
	vldr.32	s19, .L1201+144
	vldr.32	s29, .L1201+148
	add	r9, r2, r9
	vldr.32	s30, .L1201+152
	add	r8, r8, r2
	vldr.32	s28, .L1201+156
	add	r4, r4, r2
	b	.L1202
.L1203:
	.align	3
.L1201:
	.word	1061692956
	.word	1061692956
	.word	1061692956
	.word	1061692956
	.word	1059036423
	.word	1059036423
	.word	1059036423
	.word	1059036423
	.word	-1100751737
	.word	-1100751737
	.word	-1100751737
	.word	-1100751737
	.word	1064932576
	.word	1064932576
	.word	1064932576
	.word	1064932576
	.word	-1083791899
	.word	-1083791899
	.word	-1083791899
	.word	-1083791899
	.word	1054746114
	.word	1054746114
	.word	1054746114
	.word	1054746114
	.word	-1085790692
	.word	-1085790692
	.word	-1085790692
	.word	-1085790692
	.word	-1092737534
	.word	-1092737534
	.word	-1092737534
	.word	-1092737534
	.word	1059036423
	.word	-1100751737
	.word	-1083791899
	.word	1061692956
	.word	1064932576
	.word	1054746114
	.word	-1092737534
	.word	-1085790692
.L1202:
.L1193:
	mov	r2, fp
	vldr.32	s26, [r5, #4]
	vldr.32	s15, [ip, #4]
	adds	r3, r3, #8
	vldr.32	s18, [r10, #4]
	add	fp, fp, #8
	vldr.32	s5, [r2]
	add	r10, r10, #8
	ldr	r2, [r7, #176]
	add	r9, r9, #8
	vldr.32	s11, [r4, #4]
	add	r8, r8, #8
	vldr.32	s13, [lr, #4]
	vmul.f32	s6, s5, s26
	vldr.32	s9, [r1, #4]
	adds	r2, r2, #8
	vldr.32	s25, [r2, #-4]
	adds	r4, r4, #8
	vldr.32	s17, [r2, #-8]
	vmul.f32	s2, s18, s13
	vldr.32	s16, [ip]
	vmul.f32	s7, s11, s9
	vmul.f32	s10, s25, s15
	vldr.32	s12, [r5]
	vldr.32	s3, [fp, #-12]
	vmul.f32	s15, s17, s15
	vldr.32	s23, [r10, #-8]
	vmla.f32	s15, s25, s16
	vldr.32	s27, [r4, #-8]
	add	lr, lr, #8
	vldr.32	s14, [r1]
	vnmls.f32	s6, s3, s12
	vldr.32	s1, [lr, #-8]
	vnmls.f32	s10, s17, s16
	vmul.f32	s8, s3, s26
	vmul.f32	s13, s23, s13
	vmla.f32	s8, s5, s12
	vmul.f32	s9, s27, s9
	vmla.f32	s9, s11, s14
	vldr.32	s24, [r6, #4]
	vldr.32	s11, [r0, #4]
	vmla.f32	s13, s18, s1
	vldr.32	s4, [r9, #-4]
	vnmls.f32	s7, s27, s14
	vldr.32	s18, [r8, #-4]
	vnmls.f32	s2, s23, s1
	vldr.32	s5, [r8, #-8]
	adds	r1, r1, #8
	vldr.32	s23, [r9, #-8]
	vmul.f32	s3, s4, s24
	vmul.f32	s27, s18, s11
	vldr.32	s0, [r6]
	vldr.32	s17, [r0]
	vmul.f32	s14, s5, s11
	vmul.f32	s1, s23, s24
	vsub.f32	s24, s10, s6
	vmla.f32	s1, s4, s0
	vnmls.f32	s3, s23, s0
	vnmls.f32	s27, s5, s17
	vmla.f32	s14, s18, s17
	vsub.f32	s23, s15, s8
	vldr.32	s11, [r3, #-8]
	vldr.32	s12, [r3, #-4]
	vadd.f32	s26, s2, s7
	vmul.f32	s17, s24, s19
	vmul.f32	s0, s24, s30
	vmul.f32	s24, s24, s28
	add	ip, ip, #8
	vadd.f32	s25, s13, s9
	vmul.f32	s18, s23, s19
	vmul.f32	s16, s23, s30
	vmul.f32	s23, s23, s28
	vadd.f32	s10, s10, s6
	vadd.f32	s15, s15, s8
	vsub.f32	s2, s2, s7
	vsub.f32	s13, s13, s9
	vmov.f32	s4, s11
	vmov.f32	s5, s12
	vmov.f32	s6, s11
	vmov.f32	s7, s12
	vmov.f32	s8, s11
	vmov.f32	s9, s12
	vmla.f32	s4, s26, s22
	vmla.f32	s5, s25, s22
	vmla.f32	s6, s26, s21
	vmla.f32	s7, s25, s21
	vmla.f32	s8, s26, s20
	vmla.f32	s9, s25, s20
	vstr.32	s24, [r7, #176]
	vadd.f32	s24, s3, s27
	vstr.32	s23, [r7, #160]
	vmla.f32	s17, s2, s31
	vmla.f32	s0, s2, s19
	vmla.f32	s4, s10, s21
	vmla.f32	s5, s15, s21
	vmla.f32	s18, s13, s31
	vadd.f32	s23, s1, s14
	vsub.f32	s3, s3, s27
	vsub.f32	s27, s1, s14
	vldr.32	s1, [r7, #176]
	vmla.f32	s1, s2, s29
	vldr.32	s2, [r7, #160]
	vmla.f32	s6, s10, s20
	vmla.f32	s7, s15, s20
	vmla.f32	s16, s13, s19
	vmla.f32	s8, s10, s22
	vmla.f32	s9, s15, s22
	vmla.f32	s2, s13, s29
	vadd.f32	s14, s11, s26
	vmla.f32	s18, s27, s29
	vmla.f32	s17, s3, s29
	vmla.f32	s0, s3, s28
	vmla.f32	s16, s27, s28
	vmla.f32	s4, s24, s20
	vmla.f32	s2, s27, s19
	vmla.f32	s5, s23, s20
	vadd.f32	s26, s12, s25
	vmla.f32	s6, s24, s22
	vmla.f32	s7, s23, s22
	vmla.f32	s8, s24, s21
	vmla.f32	s9, s23, s21
	vmla.f32	s1, s3, s19
	str	r2, [r7, #176]
	adds	r5, r5, #8
	vadd.f32	s25, s14, s10
	adds	r6, r6, #8
	adds	r0, r0, #8
	vadd.f32	s12, s26, s15
	vadd.f32	s13, s4, s18
	vadd.f32	s11, s25, s24
	vsub.f32	s27, s5, s17
	vsub.f32	s4, s4, s18
	vadd.f32	s5, s5, s17
	vsub.f32	s18, s7, s0
	vadd.f32	s17, s6, s16
	vadd.f32	s15, s12, s23
	vadd.f32	s7, s7, s0
	vadd.f32	s10, s8, s2
	vsub.f32	s0, s9, s1
	vsub.f32	s6, s6, s16
	vsub.f32	s8, s8, s2
	vadd.f32	s9, s9, s1
	vstr.32	s11, [r3, #-8]
	ldr	r2, [r7, #156]
	vstr.32	s15, [r3, #-4]
	vstr.32	s13, [lr, #-8]
	cmp	r2, r3
	vstr.32	s27, [lr, #-4]
	vstr.32	s4, [r1, #-8]
	vstr.32	s5, [r1, #-4]
	vstr.32	s17, [ip, #-8]
	vstr.32	s18, [ip, #-4]
	vstr.32	s6, [r5, #-8]
	vstr.32	s7, [r5, #-4]
	vstr.32	s10, [r6, #-8]
	vstr.32	s0, [r6, #-4]
	vstr.32	s8, [r0, #-8]
	vstr.32	s9, [r0, #-4]
	bne	.L1193
.L1189:
	adds	r7, r7, #196
	mov	sp, r7
	@ sp needed
	vldm	sp!, {d8-d15}
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
	.cantunwind
	.fnend
	.size	_ZN6micron4math3fft6__impl7__stageILj7ELb0EfEEvPNS0_7complexIT1_EEPKS6_jjj, .-_ZN6micron4math3fft6__impl7__stageILj7ELb0EfEEvPNS0_7complexIT1_EEPKS6_jjj
