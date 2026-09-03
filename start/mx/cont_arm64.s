//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

	.text
	.global	_continue
	.type	_continue, %function

_continue:
	stp	x29, x30, [sp, #-16]!      // AArch64 requires sp 16-byte aligned
	mov	x29, #0                    // outermost frame marker (fp)
	bl	__micron_continuec          // (x0 = args) -> i64 in x0
	ldp	x29, x30, [sp], #16
	ret

	.size	_continue, . - _continue

	.section .note.GNU-stack,"",%progbits
