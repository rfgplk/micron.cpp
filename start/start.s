#  Copyright (c) 2026- David Lucius Severus
#
#  Distributed under the Boost Software License, Version 1.0.
#  See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt

	.text
	.global	_start
	.type	_start, @function

_start:
	xor	%rbp, %rbp                 # mark outermost frame for unwinders
	mov	(%rsp), %rdi               # argc
	lea	8(%rsp), %rsi              # argv = %rsp + 8
	lea	16(%rsp,%rdi,8), %rdx      # envp = %rsp + 16 + 8*argc

	mov	%rdx, %rcx                 # walk envp searching for the NULL
.Lscan_envp:
	cmpq	$0, (%rcx)
	je	.Lfound_auxv
	add	$8, %rcx
	jmp	.Lscan_envp
.Lfound_auxv:
	add	$8, %rcx                   # auxv = past the envp NULL terminator
	                                   # (4th arg in System V is already %rcx)

	and	$-16, %rsp                 # 16-byte align stack before the call
	call	__micron_startc

	mov	%eax, %edi                 # exit(retval)
	mov	$60, %eax                  # SYS_exit
	syscall
	ud2                                # unreachable

	.size	_start, . - _start

	.section .note.GNU-stack,"",@progbits
