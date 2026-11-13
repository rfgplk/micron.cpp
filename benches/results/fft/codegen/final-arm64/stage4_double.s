_ZN6micron4math3fft6__impl7__stageILm4ELb0EdEEvPNS0_7complexIT1_EEPKS6_mmm:
.LFB6942:
	.cfi_startproc
	cmp	x3, x4
	bcs	.L2168
	sub	x6, x4, x3
	and	x13, x6, 1
	sub	x12, x4, x13
	cmp	x3, x12
	bcs	.L2171
	mvn	x5, x3
	mvn	x7, x6
	add	x10, x5, x12
	add	x8, x3, x2
	and	x11, x7, 1
	add	x9, x2, x8
	add	x14, x11, x4
	add	x15, x3, 2
	lsr	x16, x10, 1
	add	x4, x2, x9
	cmp	x15, x14
	add	x17, x16, 1
	csinc	x10, x17, xzr, ls
	add	x5, x0, x9, lsl 4
	add	x7, x0, x8, lsl 4
	add	x6, x0, x3, lsl 4
	add	x4, x0, x4, lsl 4
	add	x9, x1, x9, lsl 4
	add	x8, x1, x8, lsl 4
	add	x3, x1, x3, lsl 4
	cmp	x10, 2
	ble	.L2180
	sub	x10, x10, #1
	ldp	q30, q19, [x6]
	ldp	q28, q16, [x4]
	ldp	q23, q18, [x9]
	ldp	q29, q20, [x5]
	ldp	q26, q21, [x8]
	ldp	q31, q22, [x7]
	ldp	q24, q25, [x3]
	zip2	v27.2d, v30.2d, v19.2d
	.p2align 5,,15
.L2172:
	zip1	v30.2d, v30.2d, v19.2d
	zip2	v7.2d, v28.2d, v16.2d
	zip2	v19.2d, v29.2d, v20.2d
	zip2	v17.2d, v31.2d, v22.2d
	zip1	v28.2d, v28.2d, v16.2d
	zip2	v16.2d, v23.2d, v18.2d
	zip1	v23.2d, v23.2d, v18.2d
	zip1	v29.2d, v29.2d, v20.2d
	zip2	v20.2d, v26.2d, v21.2d
	zip1	v26.2d, v26.2d, v21.2d
	zip1	v31.2d, v31.2d, v22.2d
	zip2	v18.2d, v24.2d, v25.2d
	zip1	v24.2d, v24.2d, v25.2d
	fmul	v21.2d, v16.2d, v28.2d
	fmul	v28.2d, v23.2d, v28.2d
	fmul	v22.2d, v20.2d, v29.2d
	fmul	v29.2d, v26.2d, v29.2d
	fmul	v25.2d, v18.2d, v31.2d
	fmul	v31.2d, v24.2d, v31.2d
	fmla	v21.2d, v7.2d, v23.2d
	fmls	v28.2d, v7.2d, v16.2d
	fmla	v25.2d, v17.2d, v24.2d
	fmls	v31.2d, v17.2d, v18.2d
	mov	v23.16b, v22.16b
	fmla	v23.2d, v19.2d, v26.2d
	fmls	v29.2d, v19.2d, v20.2d
	fsub	v20.2d, v25.2d, v21.2d
	fsub	v19.2d, v31.2d, v28.2d
	fsub	v24.2d, v27.2d, v23.2d
	fsub	v26.2d, v30.2d, v29.2d
	fadd	v25.2d, v25.2d, v21.2d
	fadd	v31.2d, v31.2d, v28.2d
	fadd	v28.2d, v27.2d, v23.2d
	fadd	v30.2d, v30.2d, v29.2d
	fadd	v22.2d, v24.2d, v19.2d
	fsub	v23.2d, v26.2d, v20.2d
	fsub	v27.2d, v24.2d, v19.2d
	fadd	v21.2d, v28.2d, v25.2d
	fadd	v24.2d, v30.2d, v31.2d
	fsub	v29.2d, v28.2d, v25.2d
	fsub	v31.2d, v30.2d, v31.2d
	fadd	v30.2d, v26.2d, v20.2d
	zip1	v25.2d, v24.2d, v21.2d
	zip2	v24.2d, v24.2d, v21.2d
	mov	x11, x7
	zip1	v26.2d, v31.2d, v29.2d
	add	x8, x8, 32
	zip2	v31.2d, v31.2d, v29.2d
	add	x7, x7, 32
	zip1	v28.2d, v30.2d, v27.2d
	stp	q25, q24, [x6], 32
	zip2	v30.2d, v30.2d, v27.2d
	zip1	v29.2d, v23.2d, v22.2d
	stp	q26, q31, [x5], 32
	zip2	v23.2d, v23.2d, v22.2d
	add	x3, x3, 32
	stp	q28, q30, [x11]
	stp	q29, q23, [x4], 32
	add	x9, x9, 32
	subs	x10, x10, #1
	ldp	q30, q19, [x6]
	ldp	q28, q16, [x4]
	ldp	q23, q18, [x9]
	ldp	q29, q20, [x5]
	ldp	q26, q21, [x8]
	ldp	q31, q22, [x7]
	ldp	q24, q25, [x3]
	zip2	v27.2d, v30.2d, v19.2d
	bne	.L2172
	zip1	v30.2d, v30.2d, v19.2d
	zip2	v7.2d, v28.2d, v16.2d
	zip2	v19.2d, v29.2d, v20.2d
	zip2	v17.2d, v31.2d, v22.2d
	zip1	v28.2d, v28.2d, v16.2d
	zip2	v16.2d, v23.2d, v18.2d
	zip1	v23.2d, v23.2d, v18.2d
	zip1	v29.2d, v29.2d, v20.2d
	zip2	v20.2d, v26.2d, v21.2d
	zip1	v26.2d, v26.2d, v21.2d
	zip1	v31.2d, v31.2d, v22.2d
	zip2	v18.2d, v24.2d, v25.2d
	zip1	v24.2d, v24.2d, v25.2d
	fmul	v21.2d, v16.2d, v28.2d
	fmul	v28.2d, v23.2d, v28.2d
	fmul	v22.2d, v20.2d, v29.2d
	fmul	v29.2d, v26.2d, v29.2d
	fmul	v25.2d, v18.2d, v31.2d
	fmul	v31.2d, v24.2d, v31.2d
	fmla	v21.2d, v7.2d, v23.2d
	fmls	v28.2d, v7.2d, v16.2d
	fmla	v25.2d, v17.2d, v24.2d
	fmls	v31.2d, v17.2d, v18.2d
	mov	v23.16b, v22.16b
	fmla	v23.2d, v19.2d, v26.2d
	fmls	v29.2d, v19.2d, v20.2d
	fsub	v20.2d, v25.2d, v21.2d
	fsub	v19.2d, v31.2d, v28.2d
	fsub	v24.2d, v27.2d, v23.2d
	fsub	v26.2d, v30.2d, v29.2d
	fadd	v25.2d, v25.2d, v21.2d
	fadd	v31.2d, v31.2d, v28.2d
	fadd	v28.2d, v27.2d, v23.2d
	fadd	v30.2d, v30.2d, v29.2d
	fadd	v22.2d, v24.2d, v19.2d
	fsub	v23.2d, v26.2d, v20.2d
	fsub	v27.2d, v24.2d, v19.2d
	fadd	v21.2d, v28.2d, v25.2d
	fadd	v24.2d, v30.2d, v31.2d
	fsub	v29.2d, v28.2d, v25.2d
	fsub	v31.2d, v30.2d, v31.2d
	fadd	v30.2d, v26.2d, v20.2d
	zip1	v25.2d, v24.2d, v21.2d
	zip2	v24.2d, v24.2d, v21.2d
	zip1	v26.2d, v31.2d, v29.2d
	zip2	v31.2d, v31.2d, v29.2d
	zip1	v28.2d, v30.2d, v27.2d
	stp	q25, q24, [x6]
	zip2	v30.2d, v30.2d, v27.2d
	zip1	v29.2d, v23.2d, v22.2d
	stp	q26, q31, [x5]
	zip2	v23.2d, v23.2d, v22.2d
	stp	q28, q30, [x7]
	stp	q29, q23, [x4]
.L2171:
	cbz	x13, .L2168
	add	x3, x12, x2
	lsl	x18, x12, 4
	add	x6, x2, x3
	add	x13, x1, x18
	add	x7, x2, x6
	lsl	x5, x3, 4
	lsl	x9, x6, 4
	add	x15, x0, x3, lsl 4
	add	x8, x1, x9
	add	x10, x0, x7, lsl 4
	add	x11, x1, x5
	add	x14, x0, x6, lsl 4
	lsl	x16, x7, 4
	ldr	d7, [x1, x9]
	ldr	d20, [x8, 8]
	add	x12, x0, x12, lsl 4
	ldr	d3, [x10, 8]
	lsl	x4, x2, 5
	ldr	d30, [x1, x5]
	add	x17, x12, x4
	ldr	d1, [x0, x16]
	ldr	d26, [x15, 8]
	fmul	d6, d20, d3
	ldr	d23, [x14, 8]
	fmul	d16, d7, d3
	ldr	d19, [x11, 8]
	fnmsub	d24, d7, d1, d6
	ldr	d2, [x13, 8]
	fmadd	d27, d20, d1, d16
	ldr	d29, [x1, x18]
	fmul	d21, d30, d23
	ldr	d18, [x0, x9]
	fmul	d22, d19, d23
	ldr	d17, [x0, x5]
	fmul	d1, d2, d26
	ldr	d31, [x0, x18]
	lsl	x1, x2, 4
	fnmsub	d28, d30, d18, d22
	fmadd	d5, d19, d18, d21
	fnmsub	d18, d29, d17, d1
	fmul	d29, d29, d26
	fmadd	d17, d2, d17, d29
	ldr	d25, [x12, 8]
	add	x2, x17, x2, lsl 4
	fadd	d4, d31, d28
	fsub	d7, d31, d28
	fadd	d31, d18, d24
	fadd	d0, d25, d5
	fadd	d23, d17, d27
	fsub	d20, d17, d27
	fsub	d30, d25, d5
	fsub	d26, d18, d24
	fadd	d3, d4, d31
	fsub	d6, d4, d31
	fadd	d16, d0, d23
	fsub	d19, d0, d23
	fadd	d22, d7, d20
	fsub	d28, d30, d26
	fsub	d27, d7, d20
	fadd	d24, d30, d26
	str	d3, [x0, x18]
	str	d16, [x12, 8]
	str	d6, [x12, x4]
	str	d19, [x17, 8]
	str	d22, [x0, x5]
	str	d28, [x15, 8]
	str	d27, [x17, x1]
	str	d24, [x2, 8]
.L2168:
	ret
	.p2align 2,,3
.L2180:
	ldp	q5, q27, [x3], 32
	subs	x10, x10, #1
	ldp	q21, q1, [x8], 32
	ldp	q4, q2, [x5]
	ldp	q0, q7, [x9], 32
	ldp	q18, q20, [x4]
	ldp	q6, q30, [x6]
	ldp	q25, q24, [x7]
	zip1	v26.2d, v5.2d, v27.2d
	zip2	v17.2d, v5.2d, v27.2d
	zip1	v23.2d, v25.2d, v24.2d
	zip1	v3.2d, v21.2d, v1.2d
	zip1	v16.2d, v0.2d, v7.2d
	zip1	v19.2d, v18.2d, v20.2d
	zip1	v29.2d, v6.2d, v30.2d
	zip1	v31.2d, v4.2d, v2.2d
	zip2	v27.2d, v0.2d, v7.2d
	zip2	v30.2d, v6.2d, v30.2d
	zip2	v6.2d, v21.2d, v1.2d
	zip2	v5.2d, v18.2d, v20.2d
	zip2	v25.2d, v25.2d, v24.2d
	fmul	v22.2d, v26.2d, v23.2d
	zip2	v24.2d, v4.2d, v2.2d
	fmul	v21.2d, v17.2d, v23.2d
	fmul	v28.2d, v3.2d, v31.2d
	fmul	v4.2d, v6.2d, v31.2d
	fmul	v2.2d, v16.2d, v19.2d
	fmul	v0.2d, v27.2d, v19.2d
	fmla	v21.2d, v25.2d, v26.2d
	fmls	v28.2d, v24.2d, v6.2d
	fmla	v4.2d, v24.2d, v3.2d
	fmls	v2.2d, v5.2d, v27.2d
	fmla	v0.2d, v5.2d, v16.2d
	fmls	v22.2d, v25.2d, v17.2d
	fadd	v7.2d, v29.2d, v28.2d
	fadd	v26.2d, v30.2d, v4.2d
	fadd	v1.2d, v21.2d, v0.2d
	fadd	v17.2d, v22.2d, v2.2d
	fsub	v20.2d, v21.2d, v0.2d
	fsub	v29.2d, v29.2d, v28.2d
	fsub	v18.2d, v22.2d, v2.2d
	fsub	v30.2d, v30.2d, v4.2d
	fadd	v23.2d, v7.2d, v17.2d
	fadd	v31.2d, v26.2d, v1.2d
	fsub	v3.2d, v7.2d, v17.2d
	fsub	v6.2d, v26.2d, v1.2d
	fadd	v16.2d, v29.2d, v20.2d
	fsub	v27.2d, v30.2d, v18.2d
	fsub	v19.2d, v29.2d, v20.2d
	fadd	v22.2d, v30.2d, v18.2d
	zip1	v24.2d, v23.2d, v31.2d
	zip2	v21.2d, v23.2d, v31.2d
	zip1	v28.2d, v3.2d, v6.2d
	zip2	v5.2d, v3.2d, v6.2d
	zip1	v25.2d, v16.2d, v27.2d
	zip2	v4.2d, v16.2d, v27.2d
	stp	q24, q21, [x6], 32
	zip1	v2.2d, v19.2d, v22.2d
	stp	q28, q5, [x5], 32
	zip2	v0.2d, v19.2d, v22.2d
	stp	q25, q4, [x7], 32
	stp	q2, q0, [x4], 32
	bne	.L2180
	b	.L2171
	.cfi_endproc
.LFE6942:
	.size	_ZN6micron4math3fft6__impl7__stageILm4ELb0EdEEvPNS0_7complexIT1_EEPKS6_mmm, .-_ZN6micron4math3fft6__impl7__stageILm4ELb0EdEEvPNS0_7complexIT1_EEPKS6_mmm
