#  Copyright (c) 2026- David Lucius Severus
#
#  Distributed under the Boost Software License, Version 1.0.
#  See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt

# i386 RESET VECTOR, in multiboot form

	.set MB_MAGIC, 0x1BADB002
	.set MB_FLAGS, 0x00000003          # bit0 align modules, bit1 provide memory map
	.set MB_CKSUM, -(MB_MAGIC + MB_FLAGS)

	.section .mbhdr, "a"
	.align	4
	.long	MB_MAGIC
	.long	MB_FLAGS
	.long	MB_CKSUM

	.text
	.global	_start
	.type	_start, @function

_start:
	cld                                # the ABI wants DF clear; a bootloader need not have left it so
	movl	$__stack_top, %esp         # there is no stack until this instruction
	xorl	%ebp, %ebp                 # outermost frame marker

	# .data from LMA to VMA
	movl	$__data_load, %esi
	movl	$__data_start, %edi
	movl	$__data_end, %ecx
	subl	%edi, %ecx
	cmpl	%esi, %edi
	je	1f
	rep movsb
1:
	# zero .bss
	movl	$__bss_start, %edi
	movl	$__bss_end, %ecx
	subl	%edi, %ecx
	xorl	%eax, %eax
	rep stosb

	andl	$-16, %esp                 # SysV wants 16-byte alignment at the call
	call	__micron_metalc

	# __micron_metalc ends in port::halt and does not return
2:	hlt
	jmp	2b

	.size	_start, . - _start

	.section .note.GNU-stack,"",@progbits
