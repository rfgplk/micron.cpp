_ZN6micron4math3fft6__impl7__stageILm4ELb0EfEEvPNS0_7complexIT1_EEPKS6_mmm:
.LFB6937:
	.cfi_startproc
	cmp	x3, x4
	bcs	.L2080
	sub	x13, x4, x3
	and	x14, x13, 3
	sub	x12, x4, x14
	cmp	x3, x12
	bcs	.L2082
	mvn	x10, x3
	mvn	x5, x13
	add	x8, x3, x2
	add	x6, x10, x12
	and	x7, x5, 3
	add	x9, x2, x8
	add	x15, x7, x4
	add	x4, x3, 4
	lsr	x11, x6, 2
	add	x16, x2, x9
	cmp	x4, x15
	mov	x17, 1
	csinc	x10, x17, x11, hi
	add	x5, x0, x9, lsl 3
	add	x4, x0, x8, lsl 3
	add	x6, x0, x3, lsl 3
	add	x7, x0, x16, lsl 3
	add	x9, x1, x9, lsl 3
	add	x8, x1, x8, lsl 3
	add	x3, x1, x3, lsl 3
	cmp	x10, 2
	ble	.L2096
	sub	x10, x10, #1
	ldp	q30, q19, [x6]
	ldp	q28, q16, [x7]
	ldp	q23, q18, [x9]
	ldp	q29, q20, [x5]
	ldp	q26, q21, [x8]
	ldp	q31, q22, [x4]
	ldp	q24, q25, [x3]
	uzp2	v27.4s, v30.4s, v19.4s
	.p2align 5,,15
.L2083:
	uzp1	v30.4s, v30.4s, v19.4s
	uzp2	v7.4s, v28.4s, v16.4s
	uzp2	v19.4s, v29.4s, v20.4s
	uzp2	v17.4s, v31.4s, v22.4s
	uzp1	v28.4s, v28.4s, v16.4s
	uzp2	v16.4s, v23.4s, v18.4s
	uzp1	v23.4s, v23.4s, v18.4s
	uzp1	v29.4s, v29.4s, v20.4s
	uzp2	v20.4s, v26.4s, v21.4s
	uzp1	v26.4s, v26.4s, v21.4s
	uzp1	v31.4s, v31.4s, v22.4s
	uzp2	v18.4s, v24.4s, v25.4s
	uzp1	v24.4s, v24.4s, v25.4s
	fmul	v21.4s, v16.4s, v28.4s
	fmul	v28.4s, v23.4s, v28.4s
	fmul	v22.4s, v20.4s, v29.4s
	fmul	v29.4s, v26.4s, v29.4s
	fmul	v25.4s, v18.4s, v31.4s
	fmul	v31.4s, v24.4s, v31.4s
	fmla	v21.4s, v7.4s, v23.4s
	fmls	v28.4s, v7.4s, v16.4s
	fmla	v25.4s, v17.4s, v24.4s
	fmls	v31.4s, v17.4s, v18.4s
	mov	v23.16b, v22.16b
	fmla	v23.4s, v19.4s, v26.4s
	fmls	v29.4s, v19.4s, v20.4s
	fsub	v20.4s, v25.4s, v21.4s
	fsub	v19.4s, v31.4s, v28.4s
	fsub	v24.4s, v27.4s, v23.4s
	fsub	v26.4s, v30.4s, v29.4s
	fadd	v25.4s, v25.4s, v21.4s
	fadd	v31.4s, v31.4s, v28.4s
	fadd	v28.4s, v27.4s, v23.4s
	fadd	v30.4s, v30.4s, v29.4s
	fadd	v22.4s, v24.4s, v19.4s
	fsub	v23.4s, v26.4s, v20.4s
	fsub	v27.4s, v24.4s, v19.4s
	fadd	v21.4s, v28.4s, v25.4s
	fadd	v24.4s, v30.4s, v31.4s
	fsub	v29.4s, v28.4s, v25.4s
	fsub	v31.4s, v30.4s, v31.4s
	fadd	v30.4s, v26.4s, v20.4s
	zip1	v25.4s, v24.4s, v21.4s
	zip2	v24.4s, v24.4s, v21.4s
	mov	x11, x7
	zip1	v26.4s, v31.4s, v29.4s
	add	x7, x7, 32
	zip2	v31.4s, v31.4s, v29.4s
	add	x9, x9, 32
	zip1	v28.4s, v30.4s, v27.4s
	stp	q25, q24, [x6], 32
	zip2	v30.4s, v30.4s, v27.4s
	zip1	v29.4s, v23.4s, v22.4s
	stp	q26, q31, [x5], 32
	zip2	v23.4s, v23.4s, v22.4s
	stp	q28, q30, [x4], 32
	add	x8, x8, 32
	stp	q29, q23, [x11]
	add	x3, x3, 32
	subs	x10, x10, #1
	ldp	q30, q19, [x6]
	ldp	q28, q16, [x7]
	ldp	q23, q18, [x9]
	ldp	q29, q20, [x5]
	ldp	q26, q21, [x8]
	ldp	q31, q22, [x4]
	ldp	q24, q25, [x3]
	uzp2	v27.4s, v30.4s, v19.4s
	bne	.L2083
	uzp1	v30.4s, v30.4s, v19.4s
	uzp2	v7.4s, v28.4s, v16.4s
	uzp2	v19.4s, v29.4s, v20.4s
	uzp2	v17.4s, v31.4s, v22.4s
	uzp1	v28.4s, v28.4s, v16.4s
	uzp2	v16.4s, v23.4s, v18.4s
	uzp1	v23.4s, v23.4s, v18.4s
	uzp1	v29.4s, v29.4s, v20.4s
	uzp2	v20.4s, v26.4s, v21.4s
	uzp1	v26.4s, v26.4s, v21.4s
	uzp1	v31.4s, v31.4s, v22.4s
	uzp2	v18.4s, v24.4s, v25.4s
	uzp1	v24.4s, v24.4s, v25.4s
	fmul	v21.4s, v16.4s, v28.4s
	fmul	v28.4s, v23.4s, v28.4s
	fmul	v22.4s, v20.4s, v29.4s
	fmul	v29.4s, v26.4s, v29.4s
	fmul	v25.4s, v18.4s, v31.4s
	fmul	v31.4s, v24.4s, v31.4s
	fmla	v21.4s, v7.4s, v23.4s
	fmls	v28.4s, v7.4s, v16.4s
	fmla	v25.4s, v17.4s, v24.4s
	fmls	v31.4s, v17.4s, v18.4s
	mov	v23.16b, v22.16b
	fmla	v23.4s, v19.4s, v26.4s
	fmls	v29.4s, v19.4s, v20.4s
	fsub	v20.4s, v25.4s, v21.4s
	fsub	v19.4s, v31.4s, v28.4s
	fsub	v24.4s, v27.4s, v23.4s
	fsub	v26.4s, v30.4s, v29.4s
	fadd	v25.4s, v25.4s, v21.4s
	fadd	v31.4s, v31.4s, v28.4s
	fadd	v28.4s, v27.4s, v23.4s
	fadd	v30.4s, v30.4s, v29.4s
	fadd	v22.4s, v24.4s, v19.4s
	fsub	v23.4s, v26.4s, v20.4s
	fsub	v27.4s, v24.4s, v19.4s
	fadd	v21.4s, v28.4s, v25.4s
	fadd	v24.4s, v30.4s, v31.4s
	fsub	v29.4s, v28.4s, v25.4s
	fsub	v31.4s, v30.4s, v31.4s
	fadd	v30.4s, v26.4s, v20.4s
	zip1	v25.4s, v24.4s, v21.4s
	zip2	v24.4s, v24.4s, v21.4s
	zip1	v26.4s, v31.4s, v29.4s
	zip2	v31.4s, v31.4s, v29.4s
	zip1	v28.4s, v30.4s, v27.4s
	stp	q25, q24, [x6]
	zip2	v30.4s, v30.4s, v27.4s
	zip1	v29.4s, v23.4s, v22.4s
	stp	q26, q31, [x5]
	zip2	v23.4s, v23.4s, v22.4s
	stp	q28, q30, [x4]
	stp	q29, q23, [x7]
.L2082:
	cbz	x14, .L2080
	add	x3, x12, x2
	lsl	x5, x12, 3
	add	x18, x2, x3
	add	x16, x1, x5
	add	x8, x2, x18
	lsl	x7, x3, 3
	lsl	x10, x18, 3
	add	x6, x0, x18, lsl 3
	add	x9, x1, x10
	add	x11, x0, x8, lsl 3
	add	x4, x0, x3, lsl 3
	lsl	x17, x8, 3
	add	x15, x1, x7
	ldr	s7, [x1, x10]
	ldr	s20, [x9, 4]
	add	x3, x0, x12, lsl 3
	ldr	s6, [x11, 4]
	lsl	x9, x2, 4
	ldr	s1, [x0, x17]
	add	x18, x2, x2, lsl 1
	ldr	s23, [x1, x7]
	ldr	s3, [x6, 4]
	fmul	s16, s6, s20
	ldr	s26, [x4, 4]
	fmul	s27, s6, s7
	ldr	s19, [x15, 4]
	fnmsub	s24, s1, s7, s16
	ldr	s2, [x16, 4]
	fmadd	s28, s1, s20, s27
	ldr	s18, [x0, x10]
	fmul	s21, s3, s23
	ldr	s17, [x0, x7]
	fmul	s22, s3, s19
	ldr	s29, [x1, x5]
	fmul	s1, s26, s2
	fnmsub	s5, s18, s23, s22
	fmadd	s25, s18, s19, s21
	ldr	s31, [x0, x5]
	add	x6, x3, x2, lsl 4
	fnmsub	s18, s17, s29, s1
	fmul	s29, s26, s29
	fmadd	s17, s17, s2, s29
	ldr	s30, [x3, 4]
	lsl	x8, x18, 3
	add	x10, x3, x18, lsl 3
	fadd	s4, s5, s31
	fsub	s7, s31, s5
	fadd	s0, s25, s30
	fsub	s23, s30, s25
	fadd	s31, s24, s18
	fsub	s30, s18, s24
	fadd	s26, s28, s17
	fsub	s3, s17, s28
	fadd	s20, s31, s4
	fsub	s6, s4, s31
	fadd	s16, s26, s0
	fsub	s28, s0, s26
	fadd	s19, s3, s7
	fsub	s22, s23, s30
	fsub	s27, s7, s3
	fadd	s24, s30, s23
	str	s20, [x0, x5]
	str	s16, [x3, 4]
	str	s6, [x3, x9]
	str	s28, [x6, 4]
	str	s19, [x0, x7]
	str	s22, [x4, 4]
	str	s27, [x3, x8]
	str	s24, [x10, 4]
	tbz	x13, 1, .L2080
	add	x13, x12, 1
	add	x4, x5, 8
	add	x17, x13, x2
	add	x18, x1, x4
	add	x16, x2, x17
	add	x6, x0, x4
	add	x11, x2, x16
	lsl	x10, x17, 3
	lsl	x13, x16, 3
	add	x7, x0, x16, lsl 3
	add	x3, x0, x17, lsl 3
	add	x16, x0, x11, lsl 3
	add	x15, x1, x13
	add	x17, x1, x10
	lsl	x11, x11, 3
	ldr	s5, [x1, x13]
	ldr	s21, [x7, 4]
	add	x7, x6, x9
	ldr	s29, [x3, 4]
	ldr	s4, [x15, 4]
	ldr	s7, [x16, 4]
	ldr	s26, [x17, 4]
	ldr	s27, [x18, 4]
	ldr	s23, [x0, x13]
	fmul	s31, s7, s4
	ldr	s25, [x0, x11]
	fmul	s30, s7, s5
	ldr	s1, [x1, x10]
	fmul	s20, s21, s26
	ldr	s0, [x0, x10]
	fmul	s24, s29, s27
	ldr	s2, [x1, x4]
	fnmsub	s3, s25, s5, s31
	fmadd	s6, s25, s4, s30
	fnmsub	s16, s23, s1, s20
	fmul	s28, s21, s1
	ldr	s18, [x0, x4]
	fnmsub	s25, s0, s2, s24
	fmul	s2, s29, s2
	fmadd	s19, s23, s26, s28
	fmadd	s0, s0, s27, s2
	ldr	s17, [x6, 4]
	add	x13, x6, x8
	fadd	s5, s16, s18
	fsub	s23, s18, s16
	fadd	s18, s3, s25
	fsub	s21, s25, s3
	fadd	s22, s19, s17
	fadd	s1, s6, s0
	fsub	s4, s0, s6
	fsub	s17, s17, s19
	fadd	s7, s18, s5
	fsub	s31, s5, s18
	fadd	s30, s1, s22
	fsub	s3, s22, s1
	fadd	s6, s4, s23
	fsub	s29, s17, s21
	fsub	s26, s23, s4
	fadd	s20, s21, s17
	str	s7, [x0, x4]
	str	s30, [x6, 4]
	str	s31, [x6, x9]
	str	s3, [x7, 4]
	str	s6, [x0, x10]
	str	s29, [x3, 4]
	str	s26, [x6, x8]
	str	s20, [x13, 4]
	cmp	x14, 3
	bne	.L2080
	add	x12, x12, 2
	add	x5, x5, 16
	add	x18, x12, x2
	add	x14, x1, x5
	add	x6, x2, x18
	add	x4, x0, x5
	lsl	x16, x18, 3
	add	x2, x2, x6
	add	x13, x1, x16
	add	x15, x0, x6, lsl 3
	lsl	x10, x6, 3
	add	x17, x0, x2, lsl 3
	ldr	s16, [x1, x16]
	add	x11, x1, x10
	ldr	s0, [x15, 4]
	lsl	x7, x2, 3
	ldr	s7, [x13, 4]
	add	x3, x0, x18, lsl 3
	ldr	s23, [x0, x10]
	add	x12, x4, x9
	ldr	s19, [x1, x10]
	fmul	s3, s16, s0
	fmul	s31, s7, s0
	ldr	s18, [x11, 4]
	ldr	s27, [x17, 4]
	fnmsub	s30, s16, s23, s31
	ldr	s28, [x0, x7]
	fmadd	s6, s7, s23, s3
	ldr	s2, [x3, 4]
	fmul	s24, s18, s27
	fmul	s21, s19, s27
	fnmsub	s1, s19, s28, s24
	fmadd	s4, s18, s28, s21
	ldr	s28, [x14, 4]
	ldr	s17, [x1, x5]
	add	x1, x4, x8
	ldr	s22, [x0, x5]
	ldr	s5, [x0, x16]
	fmul	s26, s28, s2
	fmul	s19, s17, s2
	ldr	s25, [x4, 4]
	fadd	s29, s22, s30
	fsub	s20, s22, s30
	fnmsub	s22, s17, s5, s26
	fmadd	s5, s28, s5, s19
	fadd	s23, s25, s6
	fsub	s16, s25, s6
	fadd	s17, s22, s1
	fadd	s0, s5, s4
	fsub	s18, s5, s4
	fsub	s25, s22, s1
	fadd	s2, s29, s17
	fadd	s4, s23, s0
	fsub	s21, s29, s17
	fsub	s1, s23, s0
	fadd	s7, s20, s18
	fsub	s31, s16, s25
	fsub	s27, s20, s18
	fadd	s24, s16, s25
	str	s2, [x0, x5]
	str	s4, [x4, 4]
	str	s21, [x4, x9]
	str	s1, [x12, 4]
	str	s7, [x0, x16]
	str	s31, [x3, 4]
	str	s27, [x4, x8]
	str	s24, [x1, 4]
.L2080:
	ret
	.p2align 2,,3
.L2096:
	ldp	q5, q27, [x3], 32
	subs	x10, x10, #1
	ldp	q21, q1, [x8], 32
	ldp	q4, q2, [x5]
	ldp	q0, q7, [x9], 32
	ldp	q18, q20, [x7]
	ldp	q6, q30, [x6]
	ldp	q25, q24, [x4]
	uzp1	v26.4s, v5.4s, v27.4s
	uzp2	v17.4s, v5.4s, v27.4s
	uzp1	v23.4s, v25.4s, v24.4s
	uzp1	v3.4s, v21.4s, v1.4s
	uzp1	v16.4s, v0.4s, v7.4s
	uzp1	v19.4s, v18.4s, v20.4s
	uzp1	v29.4s, v6.4s, v30.4s
	uzp1	v31.4s, v4.4s, v2.4s
	uzp2	v27.4s, v0.4s, v7.4s
	uzp2	v30.4s, v6.4s, v30.4s
	uzp2	v6.4s, v21.4s, v1.4s
	uzp2	v5.4s, v18.4s, v20.4s
	uzp2	v25.4s, v25.4s, v24.4s
	fmul	v22.4s, v26.4s, v23.4s
	uzp2	v24.4s, v4.4s, v2.4s
	fmul	v21.4s, v17.4s, v23.4s
	fmul	v28.4s, v3.4s, v31.4s
	fmul	v4.4s, v6.4s, v31.4s
	fmul	v2.4s, v16.4s, v19.4s
	fmul	v0.4s, v27.4s, v19.4s
	fmla	v21.4s, v25.4s, v26.4s
	fmls	v28.4s, v24.4s, v6.4s
	fmla	v4.4s, v24.4s, v3.4s
	fmls	v2.4s, v5.4s, v27.4s
	fmla	v0.4s, v5.4s, v16.4s
	fmls	v22.4s, v25.4s, v17.4s
	fadd	v7.4s, v29.4s, v28.4s
	fadd	v26.4s, v30.4s, v4.4s
	fadd	v1.4s, v21.4s, v0.4s
	fadd	v17.4s, v22.4s, v2.4s
	fsub	v20.4s, v21.4s, v0.4s
	fsub	v29.4s, v29.4s, v28.4s
	fsub	v18.4s, v22.4s, v2.4s
	fsub	v30.4s, v30.4s, v4.4s
	fadd	v23.4s, v7.4s, v17.4s
	fadd	v31.4s, v26.4s, v1.4s
	fsub	v3.4s, v7.4s, v17.4s
	fsub	v6.4s, v26.4s, v1.4s
	fadd	v16.4s, v29.4s, v20.4s
	fsub	v27.4s, v30.4s, v18.4s
	fsub	v19.4s, v29.4s, v20.4s
	fadd	v22.4s, v30.4s, v18.4s
	zip1	v24.4s, v23.4s, v31.4s
	zip2	v21.4s, v23.4s, v31.4s
	zip1	v28.4s, v3.4s, v6.4s
	zip2	v5.4s, v3.4s, v6.4s
	zip1	v25.4s, v16.4s, v27.4s
	zip2	v4.4s, v16.4s, v27.4s
	stp	q24, q21, [x6], 32
	zip1	v2.4s, v19.4s, v22.4s
	stp	q28, q5, [x5], 32
	zip2	v0.4s, v19.4s, v22.4s
	stp	q25, q4, [x4], 32
	stp	q2, q0, [x7], 32
	bne	.L2096
	b	.L2082
	.cfi_endproc
.LFE6937:
	.size	_ZN6micron4math3fft6__impl7__stageILm4ELb0EfEEvPNS0_7complexIT1_EEPKS6_mmm, .-_ZN6micron4math3fft6__impl7__stageILm4ELb0EfEEvPNS0_7complexIT1_EEPKS6_mmm
