#  Copyright (c) 2026- David Lucius Severus
#
#  Distributed under the Boost Software License, Version 1.0.
#  See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt

	.text
	.global	_continue
	.type	_continue, @function

_continue:
	push	%ebp                      # callee-saved
	xor	%ebp, %ebp                 # outermost frame marker
	sub	$4, %esp                   # so the callee sees a 16-aligned frame after the arg push
	pushl	12(%esp)                 # cdecl: forward the args pointer
	call	__micron_continuec        # returns i64 in edx:eax
	add	$8, %esp
	pop	%ebp
	ret

	.size	_continue, . - _continue

	.section .note.GNU-stack,"",@progbits
