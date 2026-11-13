_ZN6micron4math3fft6__impl7__stageILm7ELb0EdEEvPNS0_7complexIT1_EEPKS6_mmm:
.LFB6944:
	.cfi_startproc
	cmp	x3, x4
	bcs	.L2206
	stp	x29, x30, [sp, -80]!
	.cfi_def_cfa_offset 80
	.cfi_offset 29, -80
	.cfi_offset 30, -72
	sub	x6, x4, x3
	and	x18, x6, 1
	mov	x29, sp
	sub	x17, x4, x18
	stp	d10, d11, [sp, 32]
	stp	d12, d13, [sp, 48]
	stp	d14, d15, [sp, 64]
	cmp	x3, x17
	.cfi_offset 74, -48
	.cfi_offset 75, -40
	.cfi_offset 76, -32
	.cfi_offset 77, -24
	.cfi_offset 78, -16
	.cfi_offset 79, -8
	bcs	.L2199
	add	x11, x3, x2
	mvn	x5, x3
	add	x12, x2, x11
	mvn	x7, x6
	add	x8, x5, x17
	add	x13, x2, x12
	and	x9, x7, 1
	add	x14, x2, x13
	add	x10, x9, x4
	add	x16, x3, 2
	lsr	x30, x8, 1
	add	x15, x2, x14
	add	x5, x30, 1
	cmp	x16, x10
	adrp	x30, .LC60
	add	x4, x2, x15
	adrp	x6, .LC56
	adrp	x7, .LC57
	adrp	x8, .LC58
	adrp	x9, .LC59
	csinc	x16, x5, xzr, ls
	adrp	x5, .LC61
	ldr	q5, [x30, #:lo12:.LC60]
	add	x10, x0, x15, lsl 4
	ldr	q23, [x6, #:lo12:.LC56]
	add	x4, x0, x4, lsl 4
	ldr	q24, [x7, #:lo12:.LC57]
	add	x6, x0, x11, lsl 4
	ldr	q25, [x8, #:lo12:.LC58]
	add	x7, x0, x12, lsl 4
	ldr	q26, [x9, #:lo12:.LC59]
	add	x8, x0, x13, lsl 4
	ldr	q6, [x5, #:lo12:.LC61]
	add	x9, x0, x14, lsl 4
	add	x5, x0, x3, lsl 4
	add	x15, x1, x15, lsl 4
	add	x14, x1, x14, lsl 4
	add	x13, x1, x13, lsl 4
	add	x12, x1, x12, lsl 4
	add	x11, x1, x11, lsl 4
	add	x3, x1, x3, lsl 4
	stp	d8, d9, [sp, 16]
	.cfi_offset 73, -56
	.cfi_offset 72, -64
	.p2align 5,,15
.L2200:
	adrp	x30, .LC62
	ldp	q10, q19, [x3], 32
	subs	x16, x16, #1
	ldp	q17, q13, [x6]
	ldp	q31, q16, [x4]
	ldp	q27, q18, [x7]
	ldp	q30, q20, [x10]
	ldp	q15, q29, [x15], 32
	ldp	q22, q14, [x11], 32
	ldp	q28, q21, [x14], 32
	zip1	v0.2d, v10.2d, v19.2d
	zip1	v2.2d, v17.2d, v13.2d
	zip2	v10.2d, v10.2d, v19.2d
	zip1	v3.2d, v15.2d, v29.2d
	zip1	v1.2d, v22.2d, v14.2d
	zip1	v7.2d, v28.2d, v21.2d
	zip2	v15.2d, v15.2d, v29.2d
	zip2	v22.2d, v22.2d, v14.2d
	zip1	v29.2d, v31.2d, v16.2d
	zip1	v14.2d, v27.2d, v18.2d
	zip2	v28.2d, v28.2d, v21.2d
	zip2	v17.2d, v17.2d, v13.2d
	zip1	v21.2d, v30.2d, v20.2d
	fmul	v12.2d, v0.2d, v2.2d
	fmul	v19.2d, v10.2d, v2.2d
	ldp	q11, q8, [x12], 32
	zip2	v31.2d, v31.2d, v16.2d
	zip2	v27.2d, v27.2d, v18.2d
	ldp	q16, q13, [x8]
	ldp	q18, q2, [x13], 32
	zip2	v30.2d, v30.2d, v20.2d
	fmls	v12.2d, v17.2d, v10.2d
	ldp	q20, q9, [x9]
	fmla	v19.2d, v17.2d, v0.2d
	fmul	v4.2d, v3.2d, v29.2d
	fmul	v0.2d, v1.2d, v14.2d
	fmul	v10.2d, v7.2d, v21.2d
	fmul	v29.2d, v15.2d, v29.2d
	fmul	v14.2d, v22.2d, v14.2d
	fmul	v21.2d, v28.2d, v21.2d
	fmls	v4.2d, v31.2d, v15.2d
	fmla	v29.2d, v31.2d, v3.2d
	fmls	v10.2d, v30.2d, v28.2d
	ldp	q17, q3, [x5]
	fmla	v21.2d, v30.2d, v7.2d
	fmla	v14.2d, v27.2d, v1.2d
	zip2	v7.2d, v11.2d, v8.2d
	zip1	v1.2d, v11.2d, v8.2d
	fmls	v0.2d, v27.2d, v22.2d
	zip1	v11.2d, v18.2d, v2.2d
	zip2	v8.2d, v18.2d, v2.2d
	zip1	v22.2d, v16.2d, v13.2d
	zip1	v15.2d, v20.2d, v9.2d
	zip1	v30.2d, v17.2d, v3.2d
	zip2	v31.2d, v17.2d, v3.2d
	fsub	v27.2d, v0.2d, v10.2d
	zip2	v18.2d, v20.2d, v9.2d
	zip2	v16.2d, v16.2d, v13.2d
	fmul	v28.2d, v7.2d, v22.2d
	fmul	v13.2d, v1.2d, v22.2d
	fmul	v9.2d, v11.2d, v15.2d
	fmul	v15.2d, v8.2d, v15.2d
	fadd	v2.2d, v12.2d, v4.2d
	fadd	v3.2d, v19.2d, v29.2d
	fsub	v22.2d, v14.2d, v21.2d
	fadd	v0.2d, v0.2d, v10.2d
	fadd	v14.2d, v14.2d, v21.2d
	fsub	v12.2d, v12.2d, v4.2d
	fmls	v9.2d, v18.2d, v8.2d
	fmla	v15.2d, v18.2d, v11.2d
	fmls	v13.2d, v16.2d, v7.2d
	fmla	v28.2d, v16.2d, v1.2d
	ldr	q10, [x30, #:lo12:.LC62]
	fmul	v17.2d, v27.2d, v25.2d
	mov	v4.16b, v30.16b
	mov	v21.16b, v31.16b
	fsub	v19.2d, v19.2d, v29.2d
	mov	v8.16b, v30.16b
	ldr	q29, [x30, #:lo12:.LC62]
	adrp	x30, .LC63
	mov	v18.16b, v31.16b
	fmul	v11.2d, v27.2d, v10.2d
	ldr	q1, [x30, #:lo12:.LC63]
	fadd	v10.2d, v13.2d, v9.2d
	fmla	v4.2d, v2.2d, v23.2d
	fsub	v13.2d, v13.2d, v9.2d
	fmul	v20.2d, v22.2d, v25.2d
	fsub	v9.2d, v28.2d, v15.2d
	fmla	v21.2d, v3.2d, v23.2d
	fadd	v28.2d, v28.2d, v15.2d
	mov	v15.16b, v17.16b
	ldr	q17, [x30, #:lo12:.LC63]
	fmla	v8.2d, v2.2d, v24.2d
	fmul	v29.2d, v22.2d, v29.2d
	fmla	v18.2d, v3.2d, v24.2d
	fadd	v16.2d, v30.2d, v2.2d
	fadd	v7.2d, v31.2d, v3.2d
	fmla	v20.2d, v19.2d, v1.2d
	fmla	v15.2d, v12.2d, v17.2d
	fmla	v4.2d, v0.2d, v24.2d
	fmla	v21.2d, v14.2d, v24.2d
	fmla	v30.2d, v2.2d, v26.2d
	fmla	v31.2d, v3.2d, v26.2d
	fmul	v2.2d, v22.2d, v6.2d
	fmul	v27.2d, v27.2d, v6.2d
	fmla	v8.2d, v0.2d, v26.2d
	fmla	v29.2d, v19.2d, v25.2d
	fmla	v18.2d, v14.2d, v26.2d
	fmla	v11.2d, v12.2d, v25.2d
	fadd	v22.2d, v16.2d, v0.2d
	fadd	v3.2d, v7.2d, v14.2d
	fmla	v20.2d, v9.2d, v5.2d
	fmla	v21.2d, v28.2d, v26.2d
	fmla	v15.2d, v13.2d, v5.2d
	fmla	v4.2d, v10.2d, v26.2d
	fmla	v30.2d, v0.2d, v23.2d
	fmla	v2.2d, v19.2d, v5.2d
	fmla	v31.2d, v14.2d, v23.2d
	fmla	v27.2d, v12.2d, v5.2d
	fmla	v29.2d, v9.2d, v6.2d
	fmla	v18.2d, v28.2d, v23.2d
	fmla	v11.2d, v13.2d, v6.2d
	fmla	v8.2d, v10.2d, v23.2d
	fadd	v0.2d, v3.2d, v28.2d
	fadd	v12.2d, v22.2d, v10.2d
	fadd	v7.2d, v4.2d, v20.2d
	fsub	v14.2d, v21.2d, v15.2d
	fmla	v2.2d, v9.2d, v25.2d
	fmla	v27.2d, v13.2d, v25.2d
	fmla	v30.2d, v10.2d, v24.2d
	fmla	v31.2d, v28.2d, v24.2d
	fsub	v4.2d, v4.2d, v20.2d
	fadd	v20.2d, v21.2d, v15.2d
	fadd	v19.2d, v8.2d, v29.2d
	fsub	v21.2d, v18.2d, v11.2d
	zip1	v10.2d, v12.2d, v0.2d
	zip2	v16.2d, v12.2d, v0.2d
	fsub	v8.2d, v8.2d, v29.2d
	fadd	v29.2d, v18.2d, v11.2d
	fadd	v13.2d, v30.2d, v2.2d
	fsub	v1.2d, v31.2d, v27.2d
	stp	q10, q16, [x5], 32
	zip1	v18.2d, v7.2d, v14.2d
	zip2	v11.2d, v7.2d, v14.2d
	fsub	v30.2d, v30.2d, v2.2d
	fadd	v31.2d, v31.2d, v27.2d
	zip1	v9.2d, v4.2d, v20.2d
	zip2	v28.2d, v4.2d, v20.2d
	stp	q18, q11, [x6], 32
	zip1	v15.2d, v19.2d, v21.2d
	zip2	v17.2d, v19.2d, v21.2d
	zip1	v2.2d, v8.2d, v29.2d
	stp	q9, q28, [x4], 32
	zip2	v22.2d, v8.2d, v29.2d
	zip1	v27.2d, v13.2d, v1.2d
	stp	q15, q17, [x7], 32
	zip2	v3.2d, v13.2d, v1.2d
	zip1	v12.2d, v30.2d, v31.2d
	stp	q2, q22, [x10], 32
	zip2	v0.2d, v30.2d, v31.2d
	stp	q27, q3, [x8], 32
	stp	q12, q0, [x9], 32
	bne	.L2200
	ldp	d8, d9, [sp, 16]
	.cfi_restore 73
	.cfi_restore 72
.L2199:
	cbz	x18, .L2196
	add	x18, x17, x2
	lsl	x8, x17, 4
	add	x4, x2, x18
	add	x10, x1, x8
	add	x15, x2, x4
	add	x14, x0, x18, lsl 4
	add	x7, x2, x15
	lsl	x3, x18, 4
	add	x6, x2, x7
	ldr	d4, [x10, 8]
	add	x9, x2, x6
	ldr	d23, [x1, x8]
	lsl	x12, x6, 4
	ldr	d26, [x14, 8]
	add	x11, x0, x9, lsl 4
	add	x10, x1, x12
	lsl	x30, x9, 4
	ldr	d5, [x0, x3]
	ldr	d7, [x1, x12]
	fmul	d25, d4, d26
	ldr	d22, [x11, 8]
	fmul	d20, d23, d26
	ldr	d29, [x0, x30]
	fmadd	d30, d4, d5, d20
	ldr	d11, [x10, 8]
	fnmsub	d15, d23, d5, d25
	fmul	d0, d7, d22
	add	x16, x0, x15, lsl 4
	lsl	x13, x15, 4
	lsl	x11, x7, 4
	fmadd	d5, d11, d29, d0
	lsl	x15, x4, 4
	add	x9, x1, x3
	add	x4, x0, x4, lsl 4
	ldr	d16, [x16, 8]
	add	x5, x0, x6, lsl 4
	add	x16, x1, x11
	fmul	d27, d11, d22
	ldr	d24, [x1, x3]
	fnmsub	d3, d7, d29, d27
	ldr	d21, [x4, 8]
	add	x18, x1, x13
	ldr	d1, [x9, 8]
	add	x7, x0, x7, lsl 4
	ldr	d13, [x0, x15]
	add	x17, x0, x17, lsl 4
	ldr	d7, [x1, x11]
	fadd	d23, d30, d5
	ldr	d14, [x5, 8]
	fsub	d25, d30, d5
	ldr	d30, [x16, 8]
	fmul	d28, d1, d21
	ldr	d18, [x0, x12]
	fmul	d17, d24, d21
	fmadd	d12, d1, d13, d17
	fnmsub	d2, d24, d13, d28
	fmul	d13, d30, d14
	fmul	d14, d7, d14
	fmadd	d29, d30, d18, d14
	add	x12, x1, x15
	fadd	d24, d15, d3
	fsub	d26, d15, d3
	fnmsub	d15, d7, d18, d13
	ldr	d19, [x1, x15]
	ldr	d18, [x12, 8]
	adrp	x6, .LC64
	ldr	d10, [x0, x13]
	adrp	x12, .LC68
	ldr	d20, [x7, 8]
	add	x15, x2, x2, lsl 1
	fmul	d1, d18, d16
	fmul	d16, d19, d16
	fnmsub	d17, d19, d10, d1
	ldr	d4, [x0, x11]
	fadd	d19, d12, d29
	fsub	d27, d12, d29
	ldr	d12, [x18, 8]
	fmadd	d10, d18, d10, d16
	ldr	d6, [x1, x13]
	fadd	d28, d2, d15
	fsub	d22, d2, d15
	ldr	d31, [x0, x8]
	fmul	d2, d12, d20
	adrp	x1, .LC66
	fnmsub	d0, d6, d4, d2
	fmul	d6, d6, d20
	fadd	d7, d31, d24
	fmadd	d3, d12, d4, d6
	ldr	d20, [x1, #:lo12:.LC66]
	adrp	x18, .LC67
	ldr	d30, [x17, 8]
	adrp	x13, .LC65
	ldr	d2, [x18, #:lo12:.LC67]
	neg	x30, x2, lsl 6
	fadd	d14, d7, d28
	fmadd	d5, d24, d20, d31
	fmadd	d4, d23, d20, d30
	ldr	d1, [x12, #:lo12:.LC68]
	ldr	d11, [x6, #:lo12:.LC64]
	fmul	d29, d22, d2
	fadd	d13, d17, d0
	fmul	d15, d27, d2
	fsub	d18, d17, d0
	fmadd	d0, d28, d1, d5
	fadd	d17, d10, d3
	fmadd	d12, d26, d11, d29
	fsub	d10, d10, d3
	fmadd	d11, d25, d11, d15
	fadd	d6, d14, d13
	fmadd	d3, d19, d1, d4
	ldr	d21, [x13, #:lo12:.LC65]
	fadd	d16, d30, d23
	lsl	x13, x15, 5
	lsl	x10, x15, 4
	add	x6, x17, x13
	neg	x9, x2, lsl 5
	str	d6, [x0, x8]
	adrp	x8, .LC69
	fmadd	d5, d13, d21, d0
	fmadd	d4, d17, d21, d3
	ldr	d7, [x8, #:lo12:.LC69]
	fadd	d14, d16, d19
	add	x5, x6, x30
	lsl	x7, x2, 4
	add	x4, x5, x15, lsl 4
	fmadd	d29, d10, d7, d11
	fmadd	d0, d18, d7, d12
	fmadd	d11, d23, d1, d30
	fadd	d6, d14, d17
	add	x11, x4, x9
	add	x2, x11, x2, lsl 4
	fmadd	d16, d19, d21, d11
	str	d6, [x17, 8]
	fsub	d15, d4, d0
	fadd	d12, d4, d0
	fadd	d14, d5, d29
	fmadd	d3, d17, d20, d16
	fsub	d4, d5, d29
	fmadd	d5, d24, d1, d31
	fmadd	d31, d24, d21, d31
	str	d14, [x0, x3]
	adrp	x0, .LC70
	fmadd	d24, d28, d21, d5
	fmadd	d21, d23, d21, d30
	fmadd	d28, d28, d20, d31
	ldr	d0, [x0, #:lo12:.LC70]
	adrp	x3, .LC71
	str	d15, [x14, 8]
	str	d4, [x17, x13]
	fmadd	d19, d19, d20, d21
	fmadd	d6, d13, d20, d24
	fmul	d20, d27, d0
	fmadd	d23, d13, d1, d28
	fmadd	d13, d25, d2, d20
	str	d12, [x6, 8]
	fmadd	d30, d17, d1, d19
	ldr	d1, [x3, #:lo12:.LC71]
	fmul	d17, d22, d0
	fmadd	d29, d26, d2, d17
	fmul	d27, d27, d1
	fmadd	d15, d10, d1, d13
	fmadd	d25, d25, d7, d27
	fmul	d22, d22, d1
	fmadd	d26, d26, d7, d22
	fmadd	d16, d18, d1, d29
	fmadd	d10, d10, d2, d25
	fmadd	d18, d18, d2, d26
	fadd	d2, d6, d15
	fsub	d7, d6, d15
	fsub	d12, d3, d16
	fadd	d11, d3, d16
	str	d2, [x6, x30]
	fadd	d3, d23, d10
	fsub	d14, d23, d10
	fsub	d4, d30, d18
	fadd	d5, d30, d18
	str	d12, [x5, 8]
	str	d7, [x5, x10]
	str	d11, [x4, 8]
	str	d3, [x4, x9]
	str	d4, [x11, 8]
	str	d14, [x11, x7]
	str	d5, [x2, 8]
.L2196:
	ldp	d10, d11, [sp, 32]
	ldp	d12, d13, [sp, 48]
	ldp	d14, d15, [sp, 64]
	ldp	x29, x30, [sp], 80
	.cfi_restore 30
	.cfi_restore 29
	.cfi_restore 78
	.cfi_restore 79
	.cfi_restore 76
	.cfi_restore 77
	.cfi_restore 74
	.cfi_restore 75
	.cfi_def_cfa_offset 0
	ret
.L2206:
	ret
	.cfi_endproc
.LFE6944:
	.size	_ZN6micron4math3fft6__impl7__stageILm7ELb0EdEEvPNS0_7complexIT1_EEPKS6_mmm, .-_ZN6micron4math3fft6__impl7__stageILm7ELb0EdEEvPNS0_7complexIT1_EEPKS6_mmm
