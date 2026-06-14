#  Copyright (c) 2026- David Lucius Severus
#
#  Distributed under the Boost Software License, Version 1.0.
#  See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt

	.text
	.global	_start
	.type	_start, @function

_start:
	xor	%ebp, %ebp                 # outermost frame marker
	mov	(%esp), %ecx               # argc
	lea	4(%esp), %edx              # argv = esp + 4
	lea	8(%esp,%ecx,4), %ebx       # envp = esp + 8 + argc*4

	mov	%ebx, %eax                 # walk envp searching for the trailing NULL
.Lscan:
	cmpl	$0, (%eax)
	je	.Lfound
	add	$4, %eax
	jmp	.Lscan
.Lfound:
	add	$4, %eax                   # eax = auxv

	and	$-16, %esp                 # 16-byte align so that, after 4 pushes + call, the
	                               # callee sees a properly aligned frame
	push	%eax                       # cdecl args, right-to-left: auxv, envp, argv, argc
	push	%ebx
	push	%edx
	push	%ecx
	call	__micron_startc            # returns int in eax

	mov	%eax, %ebx                 # status
	mov	$1, %eax                   # SYS_exit (i386)
	int	$0x80
	ud2                                # unreachable

	.size	_start, . - _start

	.section .note.GNU-stack,"",@progbits
