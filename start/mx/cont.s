#  Copyright (c) 2026- David Lucius Severus
#
#  Distributed under the Boost Software License, Version 1.0.
#  See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt

	.text
	.global	_continue
	.type	_continue, @function

_continue:
	push	%rbp                      # callee-saved, and this push realigns %rsp to 16
	xor	%ebp, %ebp                 # mark outermost frame for unwinders
	call	__micron_continuec        # (%rdi = args) -> i64 in %rax
	pop	%rbp
	ret

	.size	_continue, . - _continue

	.section .note.GNU-stack,"",@progbits
