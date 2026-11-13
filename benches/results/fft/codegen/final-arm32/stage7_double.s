_ZN6micron4math3fft6__impl7__stageILj7ELb0EdEEvPNS0_7complexIT1_EEPKS6_jjj:
	.fnstart
.LFB7347:
	@ args = 4, pretend = 0, frame = 168
	@ frame_needed = 1, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	vpush.64	{d8, d9, d10, d11, d12, d13, d14, d15}
	sub	sp, sp, #172
	add	r7, sp, #0
	ldr	r4, [r7, #272]
	cmp	r3, r4
	bcs	.L1245
	lsls	r5, r3, #4
	adds	r6, r3, r2
	rsb	r3, r5, r4, lsl #4
	add	fp, r0, #8
	sub	r4, r3, #16
	vldr.64	d27, .L1250
	vldr.64	d26, .L1250+8
	lsrs	r3, r4, #4
	lsls	r4, r6, #4
	adds	r3, r3, #1
	str	r3, [r7]
	adds	r3, r2, r6
	vldr.64	d25, .L1250+16
	add	r9, r2, r3
	vldr.64	d12, .L1250+24
	add	r6, r2, r9
	lsl	r10, r3, #4
	adds	r3, r2, r6
	lsl	r9, r9, #4
	add	ip, r2, r3
	add	r8, r0, r10
	lsls	r3, r3, #4
	lsls	r2, r6, #4
	vldr.64	d24, .L1250+32
	adds	r6, r1, r4
	vldr.64	d5, .L1250+40
	add	r4, r4, fp
	str	r6, [r7, #8]
	add	ip, r0, ip, lsl #4
	add	r6, r5, fp
	vldr.64	d13, .L1250+48
	vldr.64	d6, .L1250+56
	add	r10, r10, r1
	add	lr, r0, r3
	add	fp, r1, r3
	adds	r5, r1, r5
	str	r6, [r7, #16]
	str	r5, [r7, #4]
	add	r6, r0, r9
	adds	r5, r0, r2
	add	r9, r9, r1
	adds	r2, r1, r2
	str	r9, [r7, #12]
	str	r2, [r7, #20]
.L1247:
	ldr	r1, [r7, #8]
	add	ip, ip, #16
	ldr	r0, [r7, #4]
	add	fp, fp, #16
	ldr	r2, [r7, #16]
	add	r3, r1, #16
	adds	r0, r0, #16
	str	r3, [r7, #8]
	sub	r9, r2, #8
	vldr.64	d16, [r1]
	vldr.64	d18, [r0, #-16]
	add	lr, lr, #16
	vldr.64	d22, [r0, #-8]
	adds	r6, r6, #16
	vldr.64	d23, [r1, #8]
	add	r10, r10, #16
	ldr	r1, [r7, #20]
	adds	r5, r5, #16
	str	r0, [r7, #4]
	add	r8, r8, #16
	ldr	r0, [r7, #12]
	adds	r1, r1, #16
	str	r1, [r7, #20]
	adds	r0, r0, #16
	str	r0, [r7, #12]
	ldm	r9, {r0, r1, r2, r3}
	add	r9, r7, #24
	vldr.64	d17, [r8, #-16]
	vldr.64	d20, [r8, #-8]
	stm	r9, {r0, r1, r2, r3}
	vldr.64	d19, [r4, #-8]
	ldm	r9, {r0, r1, r2, r3}
	add	r9, r7, #40
	vldr.64	d21, [r4]
	vmul.f64	d15, d19, d22
	vmul.f64	d7, d20, d23
	stm	r9, {r0, r1, r2, r3}
	vldr.64	d30, [fp, #-8]
	ldm	r9, {r0, r1, r2, r3}
	add	r9, r7, #56
	vldr.64	d1, [ip, #-8]
	vmla.f64	d15, d21, d18
	vmul.f64	d31, d21, d22
	vldr.64	d29, [fp, #-16]
	stm	r9, {r0, r1, r2, r3}
	vmul.f64	d22, d17, d23
	ldr	r3, [r7, #20]
	vmul.f64	d21, d30, d1
	vldr.64	d10, [lr, #-8]
	vldr.64	d28, [ip, #-16]
	vnmls.f64	d31, d19, d18
	vldr.64	d9, [r10, #-8]
	vmul.f64	d23, d29, d1
	vldr.64	d4, [r3, #-8]
	vnmls.f64	d7, d17, d16
	vnmls.f64	d21, d29, d28
	vldr.64	d0, [r3, #-16]
	vldr.64	d18, [lr, #-16]
	vmla.f64	d23, d30, d28
	vmul.f64	d19, d10, d4
	vldr.64	d3, [r6, #-8]
	vldr.64	d8, [r10, #-16]
	vmla.f64	d22, d20, d16
	ldr	r2, [r7, #12]
	vmul.f64	d16, d10, d0
	vldr.64	d29, [r6, #-16]
	vmul.f64	d30, d3, d9
	vnmls.f64	d19, d18, d0
	vmla.f64	d16, d18, d4
	vmul.f64	d10, d3, d8
	vldr.64	d14, [r5, #-8]
	vldr.64	d2, [r2, #-8]
	vmla.f64	d10, d29, d9
	vldr.64	d17, [r5, #-16]
	vnmls.f64	d30, d29, d8
	vldr.64	d11, [r2, #-16]
	mov	r9, r4
	vadd.f64	d29, d21, d31
	vldr.64	d1, [r7, #56]
	vmul.f64	d20, d2, d14
	vmul.f64	d18, d2, d17
	vmla.f64	d18, d11, d14
	vadd.f64	d28, d23, d15
	vmov.f64	d14, d1
	vsub.f64	d3, d7, d19
	vldr.64	d0, [r7, #64]
	vadd.f64	d19, d19, d7
	vmla.f64	d14, d29, d25
	vsub.f64	d21, d31, d21
	vmov.f64	d31, d1
	vnmls.f64	d20, d11, d17
	vsub.f64	d4, d22, d16
	vmla.f64	d31, d29, d27
	vsub.f64	d17, d15, d23
	vmov.f64	d2, d10
	vmov.f64	d11, d0
	vmla.f64	d14, d19, d27
	vmov.f64	d10, d1
	vmla.f64	d11, d28, d26
	vmov.f64	d15, d0
	vmla.f64	d10, d29, d26
	vadd.f64	d16, d16, d22
	vmla.f64	d15, d28, d25
	vmul.f64	d9, d4, d24
	vmul.f64	d8, d3, d24
	vmov.f64	d7, d0
	ldr	r0, [r7, #16]
	vmla.f64	d7, d28, d27
	vadd.f64	d22, d20, d30
	vmla.f64	d11, d16, d25
	vmla.f64	d15, d16, d27
	vsub.f64	d20, d30, d20
	vmov.f64	d30, d31
	vmov.f64	d31, d9
	vsub.f64	d23, d2, d18
	vmla.f64	d30, d19, d26
	vmla.f64	d31, d17, d12
	vmla.f64	d7, d16, d26
	mov	r1, r0
	vadd.f64	d18, d18, d2
	vmov.f64	d9, d10
	vmov.f64	d2, d8
	vmov.f64	d10, d14
	vmla.f64	d2, d21, d12
	vmla.f64	d9, d19, d25
	vadd.f64	d14, d1, d29
	vmul.f64	d29, d3, d13
	vmul.f64	d1, d4, d13
	vmul.f64	d3, d3, d6
	vmul.f64	d4, d4, d6
	vmla.f64	d29, d21, d24
	vmla.f64	d3, d21, d5
	vmla.f64	d1, d17, d24
	vmla.f64	d4, d17, d5
	vmla.f64	d31, d23, d5
	vmla.f64	d30, d22, d25
	vadd.f64	d0, d0, d28
	vmov.f64	d8, d11
	vmla.f64	d2, d20, d5
	vmla.f64	d9, d22, d27
	vmla.f64	d7, d18, d25
	vmla.f64	d8, d18, d27
	vmla.f64	d10, d22, d26
	vmov.f64	d11, d15
	vmla.f64	d29, d20, d6
	vmla.f64	d11, d18, d26
	vmla.f64	d3, d20, d24
	vmla.f64	d1, d23, d6
	vmla.f64	d4, d23, d24
	adds	r0, r0, #16
	adds	r4, r4, #16
	vadd.f64	d28, d14, d19
	vadd.f64	d19, d0, d16
	vadd.f64	d21, d30, d31
	vadd.f64	d17, d28, d22
	vsub.f64	d15, d7, d2
	vsub.f64	d22, d30, d31
	vadd.f64	d7, d7, d2
	vsub.f64	d23, d8, d29
	vadd.f64	d30, d9, d1
	vadd.f64	d16, d19, d18
	vsub.f64	d2, d9, d1
	vadd.f64	d18, d8, d29
	vadd.f64	d20, d10, d4
	vsub.f64	d9, d11, d3
	vsub.f64	d31, d10, d4
	vadd.f64	d8, d11, d3
	vstr.64	d17, [r0, #-24]
	str	r0, [r7, #16]
	vstr.64	d16, [r1]
	vstr.64	d21, [r4, #-24]
	ldr	r3, [r7]
	vstr.64	d15, [r9]
	subs	r2, r3, #1
	vstr.64	d22, [ip, #-16]
	vstr.64	d7, [ip, #-8]
	str	r2, [r7]
	vstr.64	d30, [r8, #-16]
	vstr.64	d23, [r8, #-8]
	vstr.64	d2, [lr, #-16]
	vstr.64	d18, [lr, #-8]
	vstr.64	d20, [r6, #-16]
	vstr.64	d9, [r6, #-8]
	vstr.64	d31, [r5, #-16]
	vstr.64	d8, [r5, #-8]
	bne	.L1247
.L1245:
	adds	r7, r7, #172
	mov	sp, r7
	@ sp needed
	vldm	sp!, {d8-d15}
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L1251:
	.align	3
.L1250:
	.word	-494146095
	.word	1071903648
	.word	-486390398
	.word	-1077118064
	.word	-1446290319
	.word	-1074998084
	.word	1963318859
	.word	1072235715
	.word	89713001
	.word	1072640668
	.word	1299295169
	.word	1071367360
	.word	1299295169
	.word	-1076116288
	.word	1963318859
	.word	-1075247933
	.cantunwind
	.fnend
	.size	_ZN6micron4math3fft6__impl7__stageILj7ELb0EdEEvPNS0_7complexIT1_EEPKS6_jjj, .-_ZN6micron4math3fft6__impl7__stageILj7ELb0EdEEvPNS0_7complexIT1_EEPKS6_jjj
