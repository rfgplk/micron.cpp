_ZN6micron4math3fft6__impl7__stageILj4ELb0EdEEvPNS0_7complexIT1_EEPKS6_jjj:
	.fnstart
.LFB7345:
	@ args = 4, pretend = 0, frame = 40
	@ frame_needed = 1, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	sub	sp, sp, #44
	add	r7, sp, #0
	ldr	r5, [r7, #80]
	cmp	r3, r5
	bcs	.L1229
	lsl	ip, r3, #4
	add	r3, r3, r2
	rsb	r10, ip, r5, lsl #4
	add	r9, r0, #8
	sub	r4, r10, #16
	add	r8, r1, ip
	lsrs	r6, r4, #4
	adds	r4, r2, r3
	add	r2, r2, r4
	adds	r5, r6, #1
	lsls	r3, r3, #4
	lsls	r6, r4, #4
	adds	r4, r0, r6
	str	r5, [r7, #4]
	add	lr, r0, r2, lsl #4
	add	r5, r9, r3
	add	r6, r6, r1
	add	r9, r9, ip
	add	fp, r1, r3
	add	ip, r7, #8
.L1231:
	vldr.64	d17, [lr, #8]
	sub	r0, r9, #8
	vldr.64	d29, [r8, #8]
	add	r10, r7, #24
	vldr.64	d26, [fp, #8]
	adds	r4, r4, #16
	vldr.64	d21, [r6, #8]
	add	lr, lr, #16
	vldr.64	d20, [r4, #-8]
	add	r8, r8, #16
	vldr.64	d16, [r5]
	add	fp, fp, #16
	vldr.64	d24, [r6]
	vmul.f64	d18, d17, d21
	vldr.64	d30, [r8, #-16]
	vmul.f64	d23, d26, d20
	vldr.64	d27, [fp, #-16]
	vmul.f64	d19, d29, d16
	vldr.64	d22, [lr, #-16]
	vmul.f64	d2, d17, d24
	vldr.64	d28, [r5, #-8]
	vmul.f64	d0, d30, d16
	vldr.64	d25, [r4, #-16]
	vmul.f64	d1, d27, d20
	vnmls.f64	d18, d22, d24
	vmla.f64	d2, d22, d21
	vnmls.f64	d19, d30, d28
	vmla.f64	d0, d29, d28
	vnmls.f64	d23, d27, d25
	vmla.f64	d1, d26, d25
	ldm	r0, {r0, r1, r2, r3}
	stm	r10, {r0, r1, r2, r3}
	adds	r6, r6, #16
	vldr.64	d7, [r7, #24]
	vldr.64	d31, [r7, #32]
	stm	ip, {r0, r1, r2, r3}
	mov	r1, r9
	vadd.f64	d3, d19, d18
	vsub.f64	d4, d19, d18
	vadd.f64	d29, d7, d23
	vadd.f64	d5, d0, d2
	vsub.f64	d6, d0, d2
	vadd.f64	d16, d31, d1
	vsub.f64	d30, d7, d23
	vsub.f64	d17, d31, d1
	mov	r2, r5
	vadd.f64	d27, d29, d3
	vsub.f64	d24, d29, d3
	add	r9, r9, #16
	adds	r5, r5, #16
	vadd.f64	d26, d16, d5
	vsub.f64	d21, d16, d5
	vadd.f64	d19, d30, d6
	vsub.f64	d20, d17, d4
	vsub.f64	d23, d30, d6
	vadd.f64	d18, d17, d4
	vstr.64	d27, [r9, #-24]
	vstr.64	d26, [r1]
	vstr.64	d24, [r4, #-16]
	vstr.64	d21, [r4, #-8]
	vstr.64	d19, [r5, #-24]
	vstr.64	d20, [r2]
	vstr.64	d23, [lr, #-16]
	ldr	r3, [r7, #4]
	vstr.64	d18, [lr, #-8]
	subs	r0, r3, #1
	str	r0, [r7, #4]
	bne	.L1231
.L1229:
	adds	r7, r7, #44
	mov	sp, r7
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
	.cantunwind
	.fnend
	.size	_ZN6micron4math3fft6__impl7__stageILj4ELb0EdEEvPNS0_7complexIT1_EEPKS6_jjj, .-_ZN6micron4math3fft6__impl7__stageILj4ELb0EdEEvPNS0_7complexIT1_EEPKS6_jjj
