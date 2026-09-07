#  Copyright (c) 2026- David Lucius Severus
#
#  Distributed under the Boost Software License, Version 1.0.
#  See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt

# amd64 RESET VECTOR, in multiboot form
#
#   .mbhdr      multiboot 1. GRUB
#   .note.Xen   the PVH boot note. QEMU'S MULTIBOOT LOADER TAKES ELF32 ONLY
#
# `duck --metal --metal-entry lm` selects reset_amd64_lm.s instead: same body, no trampoline, for a
# board whose first stage already did all of this.
	.section .note.Xen, "a", @note
	.align	4
	.long	4                          # namesz -- "Xen\0"
	.long	4                          # descsz -- one 32-bit address
	.long	18                         # XEN_ELFNOTE_PHYS32_ENTRY
	.asciz	"Xen"
	.align	4
	.long	_start
	.align	4

	.section .text.mb32, "ax", @progbits
	.code32
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

	# long mode present? CPUID leaf 0x80000001, EDX bit 29. A 486-class virtual CPU does not even
	# have the extended leaf, hence the two-step check
	movl	$0x80000000, %eax
	cpuid
	cmpl	$0x80000001, %eax
	jb	9f
	movl	$0x80000001, %eax
	cpuid
	testl	$(1 << 29), %edx
	jz	9f

	# pml4[0] -> pdpt
	movl	$__mb_pdpt, %eax
	orl	$0x03, %eax                # present | writable
	movl	%eax, __mb_pml4

	# pdpt[0..3] -> the four page directories
	movl	$__mb_pd, %eax
	orl	$0x03, %eax
	movl	$__mb_pdpt, %edi
	movl	$4, %ecx
2:	movl	%eax, (%edi)
	addl	$0x1000, %eax
	addl	$8, %edi
	loop	2b

	# 2048 directory entries x 2 MiB = 4 GiB. 0x83 is present | writable | PS (2 MiB page).
	# The high dword of every entry stays 0 from the .bss fill above.
	movl	$__mb_pd, %edi
	movl	$0x00000083, %eax
	movl	$2048, %ecx
3:	movl	%eax, (%edi)
	addl	$0x200000, %eax
	addl	$8, %edi
	loop	3b

	# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	# into long mode

	movl	$__mb_pml4, %eax
	movl	%eax, %cr3

	movl	%cr4, %eax
	orl	$(1 << 5), %eax            # CR4.PAE
	movl	%eax, %cr4

	movl	$0xC0000080, %ecx          # IA32_EFER
	rdmsr
	orl	$(1 << 8), %eax            # EFER.LME
	wrmsr

	movl	%cr0, %eax
	orl	$0x80000001, %eax          # CR0.PG | CR0.PE
	movl	%eax, %cr0

	lgdt	__mb_gdtr
	ljmp	$0x08, $_start64

	# no long mode, and no console yet either -- there is nothing to say it with
9:	cli
	hlt
	jmp	9b

	.size	_start, . - _start

# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
# 64-bit: the reset body

	.text
	.code64
	.global	_start64
	.type	_start64, @function

_start64:
	movw	$0x10, %ax                 # the flat data descriptor
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movw	%ax, %fs
	movw	%ax, %gs

	movabsq	$__stack_top, %rsp         # reload: %esp was set with a 32-bit immediate
	xorq	%rbp, %rbp                 # outermost frame marker

	# .data and .bss were handled by the trampoline, in 32-bit mode

	andq	$-16, %rsp                 # SysV wants 16-byte alignment at the call
	call	__micron_metalc

2:	hlt                                # metalc ends in port::halt; stop here if a board's returned
	jmp	2b

	.size	_start64, . - _start64

# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
# the descriptor table
#
# 0x08 is 64-bit code (L=1, D=0 -- both must hold; L=1 with D=1 is reserved and faults) and 0x10 is
# flat data. Long mode ignores the base and limit of both; they are written out in full anyway so
# the same table is legal to load while still in 32-bit mode.

	.section .rodata
	.align	8
__mb_gdt:
	.quad	0x0000000000000000         # null
	.quad	0x00AF9A000000FFFF         # 0x08  code64: G=1 L=1, present, DPL0, exec/read
	.quad	0x00CF92000000FFFF         # 0x10  data:   G=1 D=1, present, DPL0, read/write
__mb_gdt_end:

__mb_gdtr:
	.word	__mb_gdt_end - __mb_gdt - 1
	.long	__mb_gdt

# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
# the page tables
#
# In .bss on purpose: they are 24 KiB that must not be in the image on disk, and the trampoline
# zeroes .bss before it touches them. 4 KiB alignment is architectural, not a preference.

	.section .bss, "aw", @nobits
	.balign	4096
__mb_pml4:
	.space	4096
__mb_pdpt:
	.space	4096
__mb_pd:
	.space	4096 * 4

	.section .note.GNU-stack,"",@progbits
