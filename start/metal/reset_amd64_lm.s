#  Copyright (c) 2026- David Lucius Severus
#
#  Distributed under the Boost Software License, Version 1.0.
#  See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt

# amd64 RESET VECTOR, LONG-MODE ENTRY
#
# NOT MULTIBOOT

	.text
	.global	_start
	.type	_start, @function

_start:
	cld
	movabsq	$__stack_top, %rsp         # there is no stack until this instruction
	xorq	%rbp, %rbp                 # outermost frame marker

	# .data from LMA to VMA
	movabsq	$__data_load, %rsi
	movabsq	$__data_start, %rdi
	movabsq	$__data_end, %rcx
	subq	%rdi, %rcx
	cmpq	%rsi, %rdi
	je	1f
	rep movsb
1:
	# zero .bss
	movabsq	$__bss_start, %rdi
	movabsq	$__bss_end, %rcx
	subq	%rdi, %rcx
	xorq	%rax, %rax
	rep stosb

	andq	$-16, %rsp                 # SysV wants 16-byte alignment at the call
	call	__micron_metalc

2:	hlt                                # metalc ends in port::halt; stop here if a board's returned
	jmp	2b

	.size	_start, . - _start

	.section .note.GNU-stack,"",@progbits
