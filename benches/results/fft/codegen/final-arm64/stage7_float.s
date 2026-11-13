_ZN6micron4math3fft6__impl7__stageILm7ELb0EfEEvPNS0_7complexIT1_EEPKS6_mmm:
.LFB6939:
	.cfi_startproc
	cmp	x3, x4
	bcs	.L2126
	stp	x29, x30, [sp, -96]!
	.cfi_def_cfa_offset 96
	.cfi_offset 29, -96
	.cfi_offset 30, -88
	sub	x5, x4, x3
	and	x30, x5, 3
	mov	x29, sp
	sub	x17, x4, x30
	stp	d8, d9, [sp, 32]
	stp	d10, d11, [sp, 48]
	stp	d12, d13, [sp, 64]
	stp	d14, d15, [sp, 80]
	cmp	x3, x17
	.cfi_offset 72, -64
	.cfi_offset 73, -56
	.cfi_offset 74, -48
	.cfi_offset 75, -40
	.cfi_offset 76, -32
	.cfi_offset 77, -24
	.cfi_offset 78, -16
	.cfi_offset 79, -8
	bcs	.L2116
	add	x12, x3, x2
	mvn	x6, x5
	add	x13, x2, x12
	mvn	x18, x3
	add	x14, x2, x13
	and	x7, x6, 3
	add	x15, x2, x14
	adrp	x6, .LC39
	add	x16, x2, x15
	add	x9, x7, x4
	add	x10, x3, 4
	add	x8, x18, x17
	add	x11, x2, x16
	cmp	x10, x9
	adrp	x5, .LC38
	adrp	x7, .LC40
	adrp	x9, .LC41
	adrp	x10, .LC42
	ldr	q24, [x6, #:lo12:.LC39]
	adrp	x6, .LC43
	lsr	x18, x8, 2
	mov	x8, 1
	ldr	q23, [x5, #:lo12:.LC38]
	csinc	x18, x8, x18, hi
	ldr	q25, [x7, #:lo12:.LC40]
	add	x5, x0, x11, lsl 3
	ldr	q26, [x9, #:lo12:.LC41]
	add	x11, x0, x16, lsl 3
	ldr	q0, [x10, #:lo12:.LC42]
	add	x9, x0, x14, lsl 3
	ldr	q1, [x6, #:lo12:.LC43]
	add	x10, x0, x15, lsl 3
	add	x8, x0, x13, lsl 3
	add	x7, x0, x12, lsl 3
	add	x6, x0, x3, lsl 3
	add	x16, x1, x16, lsl 3
	add	x15, x1, x15, lsl 3
	add	x14, x1, x14, lsl 3
	add	x13, x1, x13, lsl 3
	add	x12, x1, x12, lsl 3
	add	x3, x1, x3, lsl 3
	str	x19, [sp, 16]
	.cfi_offset 19, -80
	.p2align 5,,15
.L2117:
	ldp	q10, q19, [x3], 32
	adrp	x19, .LC44
	ldp	q17, q13, [x7]
	subs	x18, x18, #1
	ldp	q31, q16, [x5]
	ldp	q27, q18, [x8]
	ldp	q30, q20, [x11]
	ldp	q15, q29, [x16], 32
	ldp	q22, q14, [x12], 32
	ldp	q28, q21, [x15], 32
	uzp1	v2.4s, v10.4s, v19.4s
	uzp1	v4.4s, v17.4s, v13.4s
	uzp2	v10.4s, v10.4s, v19.4s
	uzp1	v5.4s, v15.4s, v29.4s
	uzp1	v3.4s, v22.4s, v14.4s
	uzp1	v7.4s, v28.4s, v21.4s
	uzp2	v15.4s, v15.4s, v29.4s
	uzp2	v22.4s, v22.4s, v14.4s
	uzp1	v29.4s, v31.4s, v16.4s
	uzp1	v14.4s, v27.4s, v18.4s
	uzp2	v28.4s, v28.4s, v21.4s
	uzp2	v17.4s, v17.4s, v13.4s
	uzp1	v21.4s, v30.4s, v20.4s
	fmul	v12.4s, v2.4s, v4.4s
	fmul	v19.4s, v10.4s, v4.4s
	ldp	q11, q8, [x13], 32
	uzp2	v31.4s, v31.4s, v16.4s
	uzp2	v27.4s, v27.4s, v18.4s
	ldp	q16, q13, [x9]
	ldp	q18, q4, [x14], 32
	uzp2	v30.4s, v30.4s, v20.4s
	fmls	v12.4s, v17.4s, v10.4s
	ldp	q20, q9, [x10]
	fmla	v19.4s, v17.4s, v2.4s
	fmul	v6.4s, v5.4s, v29.4s
	fmul	v2.4s, v3.4s, v14.4s
	fmul	v10.4s, v7.4s, v21.4s
	fmul	v29.4s, v15.4s, v29.4s
	fmul	v14.4s, v22.4s, v14.4s
	fmul	v21.4s, v28.4s, v21.4s
	fmls	v6.4s, v31.4s, v15.4s
	fmla	v29.4s, v31.4s, v5.4s
	fmls	v10.4s, v30.4s, v28.4s
	ldp	q17, q5, [x6]
	fmla	v21.4s, v30.4s, v7.4s
	fmla	v14.4s, v27.4s, v3.4s
	uzp2	v7.4s, v11.4s, v8.4s
	uzp1	v3.4s, v11.4s, v8.4s
	fmls	v2.4s, v27.4s, v22.4s
	uzp1	v11.4s, v18.4s, v4.4s
	uzp2	v8.4s, v18.4s, v4.4s
	uzp1	v22.4s, v16.4s, v13.4s
	uzp1	v15.4s, v20.4s, v9.4s
	uzp1	v30.4s, v17.4s, v5.4s
	uzp2	v31.4s, v17.4s, v5.4s
	fsub	v27.4s, v2.4s, v10.4s
	uzp2	v18.4s, v20.4s, v9.4s
	uzp2	v16.4s, v16.4s, v13.4s
	fmul	v28.4s, v7.4s, v22.4s
	fmul	v13.4s, v3.4s, v22.4s
	fmul	v9.4s, v11.4s, v15.4s
	fmul	v15.4s, v8.4s, v15.4s
	fadd	v4.4s, v12.4s, v6.4s
	fadd	v5.4s, v19.4s, v29.4s
	fsub	v22.4s, v14.4s, v21.4s
	fadd	v2.4s, v2.4s, v10.4s
	fadd	v14.4s, v14.4s, v21.4s
	fsub	v12.4s, v12.4s, v6.4s
	fmls	v9.4s, v18.4s, v8.4s
	fmla	v15.4s, v18.4s, v11.4s
	fmls	v13.4s, v16.4s, v7.4s
	fmla	v28.4s, v16.4s, v3.4s
	ldr	q10, [x19, #:lo12:.LC44]
	fmul	v17.4s, v27.4s, v25.4s
	mov	v6.16b, v30.16b
	mov	v21.16b, v31.16b
	fsub	v19.4s, v19.4s, v29.4s
	mov	v8.16b, v30.16b
	ldr	q29, [x19, #:lo12:.LC44]
	adrp	x19, .LC45
	mov	v18.16b, v31.16b
	fmul	v11.4s, v27.4s, v10.4s
	ldr	q3, [x19, #:lo12:.LC45]
	fadd	v10.4s, v13.4s, v9.4s
	fmla	v6.4s, v4.4s, v23.4s
	fsub	v13.4s, v13.4s, v9.4s
	fmul	v20.4s, v22.4s, v25.4s
	fsub	v9.4s, v28.4s, v15.4s
	fmla	v21.4s, v5.4s, v23.4s
	fadd	v28.4s, v28.4s, v15.4s
	mov	v15.16b, v17.16b
	ldr	q17, [x19, #:lo12:.LC45]
	fmla	v8.4s, v4.4s, v24.4s
	fmul	v29.4s, v22.4s, v29.4s
	fmla	v18.4s, v5.4s, v24.4s
	fadd	v16.4s, v30.4s, v4.4s
	fadd	v7.4s, v31.4s, v5.4s
	fmla	v20.4s, v19.4s, v3.4s
	fmla	v15.4s, v12.4s, v17.4s
	fmla	v6.4s, v2.4s, v24.4s
	fmla	v21.4s, v14.4s, v24.4s
	fmla	v30.4s, v4.4s, v26.4s
	fmla	v31.4s, v5.4s, v26.4s
	fmul	v4.4s, v22.4s, v1.4s
	fmul	v27.4s, v27.4s, v1.4s
	fmla	v8.4s, v2.4s, v26.4s
	fmla	v29.4s, v19.4s, v25.4s
	fmla	v18.4s, v14.4s, v26.4s
	fmla	v11.4s, v12.4s, v25.4s
	fadd	v22.4s, v16.4s, v2.4s
	fadd	v5.4s, v7.4s, v14.4s
	fmla	v20.4s, v9.4s, v0.4s
	fmla	v21.4s, v28.4s, v26.4s
	fmla	v15.4s, v13.4s, v0.4s
	fmla	v6.4s, v10.4s, v26.4s
	fmla	v30.4s, v2.4s, v23.4s
	fmla	v4.4s, v19.4s, v0.4s
	fmla	v31.4s, v14.4s, v23.4s
	fmla	v27.4s, v12.4s, v0.4s
	fmla	v29.4s, v9.4s, v1.4s
	fmla	v18.4s, v28.4s, v23.4s
	fmla	v11.4s, v13.4s, v1.4s
	fmla	v8.4s, v10.4s, v23.4s
	fadd	v2.4s, v5.4s, v28.4s
	fadd	v12.4s, v22.4s, v10.4s
	fadd	v7.4s, v6.4s, v20.4s
	fsub	v14.4s, v21.4s, v15.4s
	fmla	v4.4s, v9.4s, v25.4s
	fmla	v27.4s, v13.4s, v25.4s
	fmla	v30.4s, v10.4s, v24.4s
	fmla	v31.4s, v28.4s, v24.4s
	fsub	v6.4s, v6.4s, v20.4s
	fadd	v20.4s, v21.4s, v15.4s
	fadd	v19.4s, v8.4s, v29.4s
	fsub	v21.4s, v18.4s, v11.4s
	zip1	v10.4s, v12.4s, v2.4s
	zip2	v16.4s, v12.4s, v2.4s
	fsub	v8.4s, v8.4s, v29.4s
	fadd	v29.4s, v18.4s, v11.4s
	fadd	v13.4s, v30.4s, v4.4s
	fsub	v3.4s, v31.4s, v27.4s
	stp	q10, q16, [x6], 32
	zip1	v18.4s, v7.4s, v14.4s
	zip2	v11.4s, v7.4s, v14.4s
	fsub	v30.4s, v30.4s, v4.4s
	fadd	v31.4s, v31.4s, v27.4s
	zip1	v9.4s, v6.4s, v20.4s
	zip2	v28.4s, v6.4s, v20.4s
	stp	q18, q11, [x7], 32
	zip1	v15.4s, v19.4s, v21.4s
	zip2	v17.4s, v19.4s, v21.4s
	zip1	v4.4s, v8.4s, v29.4s
	stp	q9, q28, [x5], 32
	zip2	v22.4s, v8.4s, v29.4s
	zip1	v27.4s, v13.4s, v3.4s
	stp	q15, q17, [x8], 32
	zip2	v5.4s, v13.4s, v3.4s
	zip1	v12.4s, v30.4s, v31.4s
	stp	q4, q22, [x11], 32
	zip2	v2.4s, v30.4s, v31.4s
	stp	q27, q5, [x9], 32
	stp	q12, q2, [x10], 32
	bne	.L2117
	ldr	x19, [sp, 16]
	.cfi_restore 19
.L2116:
	cbz	x30, .L2114
	add	x30, x17, x2
	lsl	x16, x2, 4
	add	x11, x30, x2, lsl 1
	lsl	x10, x2, 3
	add	x15, x2, x11
	add	x5, x0, x30, lsl 3
	add	x14, x1, x11, lsl 3
	add	x4, x0, x4, lsl 3
	add	x8, x0, x15, lsl 3
	add	x1, x1, x17, lsl 3
	add	x0, x0, x17, lsl 3
	mov	w13, 40199
	mov	w7, 56455
	mov	w12, 38112
	mov	w6, 9756
	mov	w3, 42469
	mov	w18, 9730
	mov	w30, 9730
	mov	w11, 9756
	add	x17, x16, 4
	lsl	x2, x2, 5
	add	x9, x10, 4
	movk	w13, 0x3f1f, lsl 16
	movk	w7, 0xbe63, lsl 16
	movk	w12, 0x3f79, lsl 16
	movk	w6, 0x3f48, lsl 16
	movk	w3, 0xbf66, lsl 16
	movk	w18, 0x3ede, lsl 16
	movk	w30, 0xbede, lsl 16
	movk	w11, 0xbf48, lsl 16
	fmov	s0, w13
	fmov	s18, w7
	fmov	s19, w12
	fmov	s13, w6
	fmov	s20, w3
	fmov	s15, w18
	fmov	s14, w30
	fmov	s16, w11
.L2118:
	ldp	s24, s7, [x1]
	ldp	s6, s25, [x5]
	ldr	s23, [x14, x17]
	ldr	s1, [x8, x17]
	ldr	s26, [x14, x16]
	fmul	s29, s7, s25
	ldr	s3, [x8, x16]
	fnmsub	s17, s24, s6, s29
	fmul	s8, s23, s1
	ldr	s10, [x1, x10]
	ldr	s11, [x1, x9]
	fmul	s21, s24, s25
	ldr	s28, [x0, x17]
	fnmsub	s30, s26, s3, s8
	ldr	s4, [x1, x2]
	fmul	s9, s26, s1
	ldr	s2, [x14, x9]
	fmadd	s27, s23, s3, s9
	ldr	s26, [x8, x9]
	fmadd	s5, s7, s6, s21
	ldr	s23, [x0, x16]
	fmul	s22, s11, s28
	ldr	s31, [x8, x10]
	fmul	s1, s10, s28
	fmul	s12, s2, s26
	fmul	s24, s4, s26
	fnmsub	s21, s10, s23, s22
	fmadd	s28, s11, s23, s1
	fmadd	s26, s2, s31, s24
	fnmsub	s22, s4, s31, s12
	ldp	s3, s11, [x14], 8
	fadd	s2, s17, s30
	ldp	s1, s29, [x8]
	ldr	s6, [x1, x16]
	fsub	s17, s17, s30
	ldr	s8, [x1, x17]
	fadd	s7, s5, s27
	ldr	s25, [x5, x17]
	fmul	s4, s11, s29
	ldr	s10, [x5, x16]
	fmul	s30, s3, s29
	fmadd	s29, s11, s1, s30
	fsub	s24, s21, s22
	fmul	s23, s6, s25
	fmul	s9, s8, s25
	fnmsub	s25, s6, s10, s9
	fmadd	s31, s8, s10, s23
	fnmsub	s9, s3, s1, s4
	fsub	s30, s28, s26
	ldp	s1, s23, [x0]
	fadd	s22, s21, s22
	add	x1, x1, 8
	fsub	s21, s5, s27
	fmul	s27, s30, s19
	fadd	s28, s28, s26
	fmadd	s11, s7, s0, s23
	fmadd	s10, s2, s0, s1
	fmadd	s3, s2, s18, s1
	fmul	s26, s24, s19
	fsub	s4, s31, s29
	fadd	s5, s25, s9
	fadd	s31, s31, s29
	fsub	s25, s25, s9
	fmadd	s12, s7, s18, s23
	fmadd	s8, s17, s13, s26
	fmadd	s6, s21, s13, s27
	fmadd	s9, s28, s18, s11
	fmadd	s10, s22, s18, s10
	fmadd	s29, s22, s20, s3
	fadd	s27, s1, s2
	fmadd	s3, s2, s20, s1
	fmul	s2, s30, s14
	fmul	s30, s30, s16
	fmadd	s2, s21, s19, s2
	fmadd	s30, s21, s15, s30
	fmul	s1, s24, s14
	fmadd	s21, s7, s20, s23
	fadd	s7, s23, s7
	fmadd	s11, s31, s20, s9
	fmadd	s6, s4, s15, s6
	fmadd	s8, s25, s15, s8
	fmadd	s9, s17, s19, s1
	fmadd	s10, s5, s20, s10
	fmadd	s12, s28, s20, s12
	fmadd	s26, s22, s0, s3
	fmul	s24, s24, s16
	fmadd	s3, s5, s0, s29
	fmadd	s17, s17, s15, s24
	fmadd	s29, s28, s0, s21
	fmadd	s12, s31, s0, s12
	fadd	s23, s27, s22
	fmadd	s22, s4, s16, s2
	fmadd	s2, s25, s16, s9
	fmadd	s1, s5, s18, s26
	fadd	s28, s7, s28
	fmadd	s4, s4, s19, s30
	fmadd	s9, s31, s18, s29
	fmadd	s26, s25, s19, s17
	fadd	s5, s23, s5
	fadd	s31, s28, s31
	fadd	s27, s10, s6
	fsub	s30, s11, s8
	fsub	s10, s10, s6
	fadd	s6, s11, s8
	fadd	s11, s3, s22
	stp	s5, s31, [x0]
	fsub	s25, s12, s2
	stp	s27, s30, [x5]
	fsub	s8, s3, s22
	fadd	s21, s12, s2
	str	s10, [x8, x16]
	fadd	s24, s1, s4
	str	s6, [x8, x17]
	fsub	s3, s9, s26
	str	s11, [x0, x16]
	fsub	s7, s1, s4
	fadd	s23, s9, s26
	str	s25, [x0, x17]
	str	s8, [x8, x10]
	add	x0, x0, 8
	str	s21, [x8, x9]
	str	s24, [x5, x16]
	str	s3, [x5, x17]
	add	x5, x5, 8
	stp	s7, s23, [x8], 8
	cmp	x0, x4
	bne	.L2118
.L2114:
	ldp	d8, d9, [sp, 32]
	ldp	d10, d11, [sp, 48]
	ldp	d12, d13, [sp, 64]
	ldp	d14, d15, [sp, 80]
	ldp	x29, x30, [sp], 96
	.cfi_restore 30
	.cfi_restore 29
	.cfi_restore 78
	.cfi_restore 79
	.cfi_restore 76
	.cfi_restore 77
	.cfi_restore 74
	.cfi_restore 75
	.cfi_restore 72
	.cfi_restore 73
	.cfi_def_cfa_offset 0
	ret
.L2126:
	ret
	.cfi_endproc
.LFE6939:
	.size	_ZN6micron4math3fft6__impl7__stageILm7ELb0EfEEvPNS0_7complexIT1_EEPKS6_mmm, .-_ZN6micron4math3fft6__impl7__stageILm7ELb0EfEEvPNS0_7complexIT1_EEPKS6_mmm
