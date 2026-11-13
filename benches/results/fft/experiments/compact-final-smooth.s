
bin/fft/tuning/compact-final/fft_microbench:     file format elf64-x86-64


Disassembly of section .init:

Disassembly of section .plt:

Disassembly of section .text:

0000000000494aa0 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm>:
  494aa0:	mov    %rsi,%rax
  494aa3:	mov    %rdx,%rsi
  494aa6:	cmp    0x80(%rdi),%rcx
  494aad:	je     494cd0 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x230>
  494ab3:	push   %rbp
  494ab4:	mov    %rsp,%rbp
  494ab7:	push   %r15
  494ab9:	push   %r14
  494abb:	push   %r13
  494abd:	push   %r12
  494abf:	push   %rbx
  494ac0:	and    $0xffffffffffffffe0,%rsp
  494ac4:	add    $0xffffffffffffff80,%rsp
  494ac8:	mov    0x58(%rdi),%rbx
  494acc:	mov    0x70(%rdi),%rdx
  494ad0:	mov    0x8(%rbx,%rcx,8),%r11
  494ad5:	mov    (%rdx,%rcx,8),%r15
  494ad9:	cmp    $0x1,%r11
  494add:	je     494d20 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x280>
  494ae3:	mov    %rcx,0x28(%rsp)
  494ae8:	mov    %rdi,%rbx
  494aeb:	mov    %rsi,0x30(%rsp)
  494af0:	test   %r15,%r15
  494af3:	je     494ca3 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x203>
  494af9:	mov    %r15,%rdi
  494afc:	lea    0x1(%rcx),%r14
  494b00:	mov    %r11,%r13
  494b03:	mov    %r15,%rcx
  494b06:	imul   %r8,%rdi
  494b0a:	shl    $0x4,%r8
  494b0e:	mov    0x30(%rsp),%rdx
  494b13:	xor    %r10d,%r10d
  494b16:	mov    %r8,%r12
  494b19:	shl    $0x4,%r13
  494b1d:	mov    %rdi,0x38(%rsp)
  494b22:	and    $0x3,%ecx
  494b25:	je     494bce <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x12e>
  494b2b:	cmp    $0x1,%rcx
  494b2f:	je     494b80 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0xe0>
  494b31:	cmp    $0x2,%rcx
  494b35:	jne    494ce0 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x240>
  494b3b:	mov    0x38(%rsp),%r8
  494b40:	mov    %rax,%rsi
  494b43:	mov    %r14,%rcx
  494b46:	mov    %rbx,%rdi
  494b49:	mov    %r10,0x8(%rsp)
  494b4e:	mov    %r11,0x10(%rsp)
  494b53:	mov    %rdx,0x18(%rsp)
  494b58:	mov    %rax,0x20(%rsp)
  494b5d:	call   494aa0 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm>
  494b62:	mov    0x8(%rsp),%r10
  494b67:	mov    0x20(%rsp),%rax
  494b6c:	mov    0x18(%rsp),%rdx
  494b71:	mov    0x10(%rsp),%r11
  494b76:	add    $0x1,%r10
  494b7a:	add    %r12,%rax
  494b7d:	add    %r13,%rdx
  494b80:	mov    0x38(%rsp),%r8
  494b85:	mov    %rax,%rsi
  494b88:	mov    %r14,%rcx
  494b8b:	mov    %rbx,%rdi
  494b8e:	mov    %r10,0x8(%rsp)
  494b93:	mov    %r11,0x10(%rsp)
  494b98:	mov    %rdx,0x18(%rsp)
  494b9d:	mov    %rax,0x20(%rsp)
  494ba2:	call   494aa0 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm>
  494ba7:	mov    0x8(%rsp),%r10
  494bac:	mov    0x20(%rsp),%rax
  494bb1:	mov    0x18(%rsp),%rdx
  494bb6:	mov    0x10(%rsp),%r11
  494bbb:	add    $0x1,%r10
  494bbf:	add    %r12,%rax
  494bc2:	add    %r13,%rdx
  494bc5:	cmp    %r10,%r15
  494bc8:	je     494ca3 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x203>
  494bce:	mov    %r11,0x8(%rsp)
  494bd3:	mov    %r15,0x10(%rsp)
  494bd8:	mov    %r10,%r15
  494bdb:	mov    0x38(%rsp),%r8
  494be0:	mov    %rax,%rsi
  494be3:	mov    %r14,%rcx
  494be6:	mov    %rbx,%rdi
  494be9:	mov    %rax,0x20(%rsp)
  494bee:	add    $0x4,%r15
  494bf2:	mov    %rdx,0x18(%rsp)
  494bf7:	call   494aa0 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm>
  494bfc:	mov    0x20(%rsp),%r9
  494c01:	mov    %r14,%rcx
  494c04:	mov    %rbx,%rdi
  494c07:	mov    0x18(%rsp),%rdx
  494c0c:	mov    0x38(%rsp),%r8
  494c11:	add    %r12,%r9
  494c14:	mov    %r9,%rsi
  494c17:	add    %r13,%rdx
  494c1a:	mov    %r9,0x20(%rsp)
  494c1f:	mov    %rdx,0x18(%rsp)
  494c24:	call   494aa0 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm>
  494c29:	mov    0x20(%rsp),%rdi
  494c2e:	mov    0x18(%rsp),%rdx
  494c33:	mov    %r14,%rcx
  494c36:	mov    0x38(%rsp),%r8
  494c3b:	add    %r12,%rdi
  494c3e:	add    %r13,%rdx
  494c41:	mov    %rdi,%rsi
  494c44:	mov    %rdi,0x20(%rsp)
  494c49:	mov    %rbx,%rdi
  494c4c:	mov    %rdx,0x18(%rsp)
  494c51:	call   494aa0 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm>
  494c56:	mov    0x20(%rsp),%r10
  494c5b:	mov    %r14,%rcx
  494c5e:	mov    %rbx,%rdi
  494c61:	mov    0x18(%rsp),%rdx
  494c66:	mov    0x38(%rsp),%r8
  494c6b:	add    %r12,%r10
  494c6e:	add    %r13,%rdx
  494c71:	mov    %r10,%rsi
  494c74:	mov    %r10,0x20(%rsp)
  494c79:	mov    %rdx,0x18(%rsp)
  494c7e:	call   494aa0 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm>
  494c83:	mov    0x20(%rsp),%rax
  494c88:	mov    0x18(%rsp),%rdx
  494c8d:	add    %r12,%rax
  494c90:	add    %r13,%rdx
  494c93:	cmp    %r15,0x10(%rsp)
  494c98:	jne    494bdb <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x13b>
  494c9e:	mov    0x8(%rsp),%r11
  494ca3:	mov    0x28(%rsp),%rdx
  494ca8:	mov    0x30(%rsp),%rsi
  494cad:	lea    -0x28(%rbp),%rsp
  494cb1:	xor    %ecx,%ecx
  494cb3:	mov    %rbx,%rdi
  494cb6:	mov    %r11,%r8
  494cb9:	pop    %rbx
  494cba:	pop    %r12
  494cbc:	pop    %r13
  494cbe:	pop    %r14
  494cc0:	pop    %r15
  494cc2:	pop    %rbp
  494cc3:	jmp    471ff0 <_ZNK6micron4math3fft6__impl8__engineIdE15__combine_stageILb0EEEvPNS0_7complexIdEEmmm>
  494cc8:	nopl   0x0(%rax,%rax,1)
  494cd0:	vmovdqu (%rax),%xmm9
  494cd4:	vmovdqu %xmm9,(%rdx)
  494cd8:	ret
  494cd9:	nopl   0x0(%rax)
  494ce0:	mov    %rdi,%r8
  494ce3:	mov    %rax,%rsi
  494ce6:	mov    %r14,%rcx
  494ce9:	mov    %rbx,%rdi
  494cec:	mov    %r11,0x18(%rsp)
  494cf1:	mov    %rax,0x20(%rsp)
  494cf6:	call   494aa0 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm>
  494cfb:	mov    0x20(%rsp),%rax
  494d00:	mov    0x30(%rsp),%r8
  494d05:	mov    $0x1,%r10d
  494d0b:	mov    0x18(%rsp),%r11
  494d10:	add    %r12,%rax
  494d13:	lea    (%r8,%r13,1),%rdx
  494d17:	jmp    494b3b <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x9b>
  494d1c:	nopl   0x0(%rax)
  494d20:	cmp    $0x8,%r15
  494d24:	ja     494db1 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x311>
  494d2a:	jmp    *0x4e40c8(,%r15,8)
  494d32:	cmp    $0x1,%r8
  494d36:	jne    494f0f <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x46f>
  494d3c:	vmovdqu (%rax),%ymm4
  494d40:	vmovdqu 0x20(%rax),%ymm5
  494d45:	vmovdqa %ymm4,0x40(%rsp)
  494d4b:	vmovdqa %ymm5,0x60(%rsp)
  494d51:	vpermpd $0x44,0x40(%rsp),%ymm6
  494d59:	vpermpd $0x44,0x60(%rsp),%ymm7
  494d61:	vpermpd $0xbe,0x40(%rsp),%ymm11
  494d69:	vpermpd $0xbe,0x60(%rsp),%ymm12
  494d71:	vaddpd %ymm7,%ymm6,%ymm8
  494d75:	vsubpd %ymm7,%ymm6,%ymm9
  494d79:	vaddpd %ymm11,%ymm12,%ymm13
  494d7e:	vsubpd %ymm12,%ymm11,%ymm14
  494d83:	vblendpd $0x3,%ymm8,%ymm9,%ymm10
  494d89:	vblendpd $0x3,%ymm13,%ymm14,%ymm15
  494d8f:	vsubpd %ymm15,%ymm10,%ymm2
  494d94:	vaddpd %ymm10,%ymm15,%ymm0
  494d99:	vblendpd $0x7,%ymm0,%ymm2,%ymm1
  494d9f:	vblendpd $0x7,%ymm2,%ymm0,%ymm3
  494da5:	vmovupd %ymm1,(%rsi)
  494da9:	vmovupd %ymm3,0x20(%rsi)
  494dae:	vzeroupper
  494db1:	lea    -0x28(%rbp),%rsp
  494db5:	pop    %rbx
  494db6:	pop    %r12
  494db8:	pop    %r13
  494dba:	pop    %r14
  494dbc:	pop    %r15
  494dbe:	pop    %rbp
  494dbf:	ret
  494dc0:	lea    -0x28(%rbp),%rsp
  494dc4:	mov    %r8,%rdx
  494dc7:	mov    %rax,%rdi
  494dca:	pop    %rbx
  494dcb:	pop    %r12
  494dcd:	pop    %r13
  494dcf:	pop    %r14
  494dd1:	pop    %r15
  494dd3:	pop    %rbp
  494dd4:	jmp    494890 <_ZN6micron4math3fft6__impl12__leaf_largeILm8ELb0EdEEvPKNS0_7complexIT1_EEPS6_m>
  494dd9:	lea    -0x28(%rbp),%rsp
  494ddd:	mov    %r8,%rdx
  494de0:	mov    %rax,%rdi
  494de3:	pop    %rbx
  494de4:	pop    %r12
  494de6:	pop    %r13
  494de8:	pop    %r14
  494dea:	pop    %r15
  494dec:	pop    %rbp
  494ded:	jmp    494590 <_ZN6micron4math3fft6__impl12__leaf_largeILm7ELb0EdEEvPKNS0_7complexIT1_EEPS6_m>
  494df2:	lea    -0x28(%rbp),%rsp
  494df6:	mov    %r8,%rdx
  494df9:	mov    %rax,%rdi
  494dfc:	pop    %rbx
  494dfd:	pop    %r12
  494dff:	pop    %r13
  494e01:	pop    %r14
  494e03:	pop    %r15
  494e05:	pop    %rbp
  494e06:	jmp    4943b0 <_ZN6micron4math3fft6__impl12__leaf_largeILm5ELb0EdEEvPKNS0_7complexIT1_EEPS6_m>
  494e0b:	cmp    $0x1,%r8
  494e0f:	jne    494f48 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x4a8>
  494e15:	vmovdqu (%rax),%ymm7
  494e19:	vmovdqu 0x20(%rax),%xmm8
  494e1e:	vmovdqa %ymm7,0x40(%rsp)
  494e24:	vmovdqa %xmm8,0x60(%rsp)
  494e2a:	vzeroupper
  494e2d:	vmovsd 0x50(%rsp),%xmm10
  494e33:	vmovsd 0x60(%rsp),%xmm12
  494e39:	vmovsd 0x58(%rsp),%xmm11
  494e3f:	vmovsd 0x68(%rsp),%xmm13
  494e45:	vaddsd %xmm12,%xmm10,%xmm14
  494e4a:	vmovsd 0x40(%rsp),%xmm3
  494e50:	vsubsd %xmm12,%xmm10,%xmm0
  494e55:	vmovsd 0x48(%rsp),%xmm5
  494e5b:	vaddsd %xmm13,%xmm11,%xmm15
  494e60:	vmovsd 0x4e558(%rip),%xmm2        # 4e33c0 <_ZN6micron6__implL11__digit_tblE+0x4c0>
  494e68:	vsubsd %xmm13,%xmm11,%xmm1
  494e6d:	vmovsd 0x4e51b(%rip),%xmm7        # 4e3390 <_ZN6micron6__implL11__digit_tblE+0x490>
  494e75:	vmovapd %xmm14,%xmm9
  494e7a:	vmovapd %xmm0,%xmm10
  494e7e:	vaddsd %xmm3,%xmm14,%xmm4
  494e82:	vfnmadd132sd %xmm2,%xmm3,%xmm9
  494e87:	vfnmadd132sd %xmm15,%xmm5,%xmm2
  494e8c:	vmovapd %xmm1,%xmm8
  494e90:	vaddsd %xmm5,%xmm15,%xmm6
  494e94:	vmovsd %xmm4,(%rsi)
  494e98:	vmovsd %xmm6,0x8(%rsi)
  494e9d:	vfmadd132sd %xmm7,%xmm9,%xmm8
  494ea2:	vfnmadd132sd %xmm7,%xmm2,%xmm10
  494ea7:	vfnmadd132sd %xmm7,%xmm9,%xmm1
  494eac:	vfmadd132sd %xmm7,%xmm2,%xmm0
  494eb1:	vmovsd %xmm8,0x10(%rsi)
  494eb6:	vmovsd %xmm10,0x18(%rsi)
  494ebb:	vmovsd %xmm1,0x20(%rsi)
  494ec0:	vmovsd %xmm0,0x28(%rsi)
  494ec5:	jmp    494db1 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x311>
  494eca:	shl    $0x4,%r8
  494ece:	vmovsd (%rax),%xmm11
  494ed2:	vmovsd 0x8(%rax),%xmm12
  494ed7:	add    %r8,%rax
  494eda:	vmovsd (%rax),%xmm13
  494ede:	vmovsd 0x8(%rax),%xmm14
  494ee3:	vaddsd %xmm13,%xmm11,%xmm15
  494ee8:	vaddsd %xmm14,%xmm12,%xmm3
  494eed:	vsubsd %xmm13,%xmm11,%xmm5
  494ef2:	vsubsd %xmm14,%xmm12,%xmm2
  494ef7:	vmovsd %xmm15,(%rsi)
  494efb:	vmovsd %xmm3,0x8(%rsi)
  494f00:	vmovsd %xmm5,0x10(%rsi)
  494f05:	vmovsd %xmm2,0x18(%rsi)
  494f0a:	jmp    494db1 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x311>
  494f0f:	shl    $0x4,%r8
  494f13:	vmovdqu (%rax),%xmm0
  494f17:	add    %r8,%rax
  494f1a:	vmovdqu (%rax),%xmm1
  494f1e:	add    %r8,%rax
  494f21:	vmovdqa %xmm0,0x40(%rsp)
  494f27:	vmovdqu (%rax),%xmm2
  494f2b:	vmovdqu (%rax,%r8,1),%xmm3
  494f31:	vmovdqa %xmm1,0x50(%rsp)
  494f37:	vmovdqa %xmm2,0x60(%rsp)
  494f3d:	vmovdqa %xmm3,0x70(%rsp)
  494f43:	jmp    494d51 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x2b1>
  494f48:	shl    $0x4,%r8
  494f4c:	vmovdqu (%rax),%xmm4
  494f50:	add    %r8,%rax
  494f53:	vmovdqu (%rax),%xmm5
  494f57:	vmovdqu (%rax,%r8,1),%xmm6
  494f5d:	vmovdqa %xmm4,0x40(%rsp)
  494f63:	vmovdqa %xmm5,0x50(%rsp)
  494f69:	vmovdqa %xmm6,0x60(%rsp)
  494f6f:	jmp    494e2d <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x38d>

Disassembly of section .fini:
