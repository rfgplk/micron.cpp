
bin/fft/tuning/leaf-init/fft_microbench:     file format elf64-x86-64


Disassembly of section .init:

Disassembly of section .plt:

Disassembly of section .text:

0000000000494c60 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm>:
  494c60:	mov    %rsi,%rax
  494c63:	mov    %rdx,%rsi
  494c66:	cmp    0x80(%rdi),%rcx
  494c6d:	je     494e90 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x230>
  494c73:	push   %rbp
  494c74:	mov    %rsp,%rbp
  494c77:	push   %r15
  494c79:	push   %r14
  494c7b:	push   %r13
  494c7d:	push   %r12
  494c7f:	push   %rbx
  494c80:	and    $0xffffffffffffffe0,%rsp
  494c84:	sub    $0x40,%rsp
  494c88:	mov    0x58(%rdi),%rbx
  494c8c:	mov    0x70(%rdi),%rdx
  494c90:	mov    0x8(%rbx,%rcx,8),%r11
  494c95:	mov    (%rdx,%rcx,8),%r15
  494c99:	cmp    $0x1,%r11
  494c9d:	je     494ee0 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x280>
  494ca3:	mov    %rcx,0x28(%rsp)
  494ca8:	mov    %rdi,%rbx
  494cab:	mov    %rsi,0x30(%rsp)
  494cb0:	test   %r15,%r15
  494cb3:	je     494e63 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x203>
  494cb9:	mov    %r15,%rdi
  494cbc:	lea    0x1(%rcx),%r14
  494cc0:	mov    %r11,%r13
  494cc3:	mov    %r15,%rcx
  494cc6:	imul   %r8,%rdi
  494cca:	shl    $0x4,%r8
  494cce:	mov    0x30(%rsp),%rdx
  494cd3:	xor    %r10d,%r10d
  494cd6:	mov    %r8,%r12
  494cd9:	shl    $0x4,%r13
  494cdd:	mov    %rdi,0x38(%rsp)
  494ce2:	and    $0x3,%ecx
  494ce5:	je     494d8e <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x12e>
  494ceb:	cmp    $0x1,%rcx
  494cef:	je     494d40 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0xe0>
  494cf1:	cmp    $0x2,%rcx
  494cf5:	jne    494ea0 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x240>
  494cfb:	mov    0x38(%rsp),%r8
  494d00:	mov    %rax,%rsi
  494d03:	mov    %r14,%rcx
  494d06:	mov    %rbx,%rdi
  494d09:	mov    %r10,0x8(%rsp)
  494d0e:	mov    %r11,0x10(%rsp)
  494d13:	mov    %rdx,0x18(%rsp)
  494d18:	mov    %rax,0x20(%rsp)
  494d1d:	call   494c60 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm>
  494d22:	mov    0x8(%rsp),%r10
  494d27:	mov    0x20(%rsp),%rax
  494d2c:	mov    0x18(%rsp),%rdx
  494d31:	mov    0x10(%rsp),%r11
  494d36:	add    $0x1,%r10
  494d3a:	add    %r12,%rax
  494d3d:	add    %r13,%rdx
  494d40:	mov    0x38(%rsp),%r8
  494d45:	mov    %rax,%rsi
  494d48:	mov    %r14,%rcx
  494d4b:	mov    %rbx,%rdi
  494d4e:	mov    %r10,0x8(%rsp)
  494d53:	mov    %r11,0x10(%rsp)
  494d58:	mov    %rdx,0x18(%rsp)
  494d5d:	mov    %rax,0x20(%rsp)
  494d62:	call   494c60 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm>
  494d67:	mov    0x8(%rsp),%r10
  494d6c:	mov    0x20(%rsp),%rax
  494d71:	mov    0x18(%rsp),%rdx
  494d76:	mov    0x10(%rsp),%r11
  494d7b:	add    $0x1,%r10
  494d7f:	add    %r12,%rax
  494d82:	add    %r13,%rdx
  494d85:	cmp    %r10,%r15
  494d88:	je     494e63 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x203>
  494d8e:	mov    %r11,0x8(%rsp)
  494d93:	mov    %r15,0x10(%rsp)
  494d98:	mov    %r10,%r15
  494d9b:	mov    0x38(%rsp),%r8
  494da0:	mov    %rax,%rsi
  494da3:	mov    %r14,%rcx
  494da6:	mov    %rbx,%rdi
  494da9:	mov    %rax,0x20(%rsp)
  494dae:	add    $0x4,%r15
  494db2:	mov    %rdx,0x18(%rsp)
  494db7:	call   494c60 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm>
  494dbc:	mov    0x20(%rsp),%r9
  494dc1:	mov    %r14,%rcx
  494dc4:	mov    %rbx,%rdi
  494dc7:	mov    0x18(%rsp),%rdx
  494dcc:	mov    0x38(%rsp),%r8
  494dd1:	add    %r12,%r9
  494dd4:	mov    %r9,%rsi
  494dd7:	add    %r13,%rdx
  494dda:	mov    %r9,0x20(%rsp)
  494ddf:	mov    %rdx,0x18(%rsp)
  494de4:	call   494c60 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm>
  494de9:	mov    0x20(%rsp),%rdi
  494dee:	mov    0x18(%rsp),%rdx
  494df3:	mov    %r14,%rcx
  494df6:	mov    0x38(%rsp),%r8
  494dfb:	add    %r12,%rdi
  494dfe:	add    %r13,%rdx
  494e01:	mov    %rdi,%rsi
  494e04:	mov    %rdi,0x20(%rsp)
  494e09:	mov    %rbx,%rdi
  494e0c:	mov    %rdx,0x18(%rsp)
  494e11:	call   494c60 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm>
  494e16:	mov    0x20(%rsp),%r10
  494e1b:	mov    %r14,%rcx
  494e1e:	mov    %rbx,%rdi
  494e21:	mov    0x18(%rsp),%rdx
  494e26:	mov    0x38(%rsp),%r8
  494e2b:	add    %r12,%r10
  494e2e:	add    %r13,%rdx
  494e31:	mov    %r10,%rsi
  494e34:	mov    %r10,0x20(%rsp)
  494e39:	mov    %rdx,0x18(%rsp)
  494e3e:	call   494c60 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm>
  494e43:	mov    0x20(%rsp),%rax
  494e48:	mov    0x18(%rsp),%rdx
  494e4d:	add    %r12,%rax
  494e50:	add    %r13,%rdx
  494e53:	cmp    %r15,0x10(%rsp)
  494e58:	jne    494d9b <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x13b>
  494e5e:	mov    0x8(%rsp),%r11
  494e63:	mov    0x28(%rsp),%rdx
  494e68:	mov    0x30(%rsp),%rsi
  494e6d:	lea    -0x28(%rbp),%rsp
  494e71:	xor    %ecx,%ecx
  494e73:	mov    %rbx,%rdi
  494e76:	mov    %r11,%r8
  494e79:	pop    %rbx
  494e7a:	pop    %r12
  494e7c:	pop    %r13
  494e7e:	pop    %r14
  494e80:	pop    %r15
  494e82:	pop    %rbp
  494e83:	jmp    471fb0 <_ZNK6micron4math3fft6__impl8__engineIdE15__combine_stageILb0EEEvPNS0_7complexIdEEmmm>
  494e88:	nopl   0x0(%rax,%rax,1)
  494e90:	vmovdqu (%rax),%xmm9
  494e94:	vmovdqu %xmm9,(%rdx)
  494e98:	ret
  494e99:	nopl   0x0(%rax)
  494ea0:	mov    %rdi,%r8
  494ea3:	mov    %rax,%rsi
  494ea6:	mov    %r14,%rcx
  494ea9:	mov    %rbx,%rdi
  494eac:	mov    %r11,0x18(%rsp)
  494eb1:	mov    %rax,0x20(%rsp)
  494eb6:	call   494c60 <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm>
  494ebb:	mov    0x20(%rsp),%rax
  494ec0:	mov    0x30(%rsp),%r8
  494ec5:	mov    $0x1,%r10d
  494ecb:	mov    0x18(%rsp),%r11
  494ed0:	add    %r12,%rax
  494ed3:	lea    (%r8,%r13,1),%rdx
  494ed7:	jmp    494cfb <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x9b>
  494edc:	nopl   0x0(%rax)
  494ee0:	cmp    $0x8,%r15
  494ee4:	ja     494f8a <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x32a>
  494eea:	jmp    *0x4e3118(,%r15,8)
  494ef2:	shl    $0x4,%r8
  494ef6:	vmovsd (%rax),%xmm7
  494efa:	vmovhpd 0x8(%rax),%xmm7,%xmm0
  494eff:	vinsertf128 $0x1,%xmm0,%ymm0,%ymm0
  494f05:	add    %r8,%rax
  494f08:	vmovsd (%rax),%xmm1
  494f0c:	vmovsd 0x8(%rax),%xmm2
  494f11:	add    %r8,%rax
  494f14:	vmovsd (%rax),%xmm6
  494f18:	vmovhpd 0x8(%rax),%xmm6,%xmm8
  494f1d:	add    %r8,%rax
  494f20:	vinsertf128 $0x1,%xmm8,%ymm8,%ymm8
  494f26:	vmovsd (%rax),%xmm12
  494f2a:	vunpcklpd %xmm1,%xmm2,%xmm3
  494f2e:	vunpcklpd %xmm2,%xmm1,%xmm4
  494f32:	vaddpd %ymm0,%ymm8,%ymm9
  494f36:	vmovsd 0x8(%rax),%xmm13
  494f3b:	vinsertf128 $0x1,%xmm3,%ymm4,%ymm5
  494f41:	vsubpd %ymm8,%ymm0,%ymm10
  494f46:	vunpcklpd %xmm12,%xmm13,%xmm14
  494f4b:	vunpcklpd %xmm13,%xmm12,%xmm15
  494f50:	vinsertf128 $0x1,%xmm14,%ymm15,%ymm7
  494f56:	vblendpd $0x3,%ymm9,%ymm10,%ymm11
  494f5c:	vaddpd %ymm7,%ymm5,%ymm0
  494f60:	vsubpd %ymm7,%ymm5,%ymm1
  494f64:	vblendpd $0x3,%ymm0,%ymm1,%ymm3
  494f6a:	vsubpd %ymm3,%ymm11,%ymm2
  494f6e:	vaddpd %ymm3,%ymm11,%ymm4
  494f72:	vblendpd $0x7,%ymm4,%ymm2,%ymm5
  494f78:	vblendpd $0x7,%ymm2,%ymm4,%ymm6
  494f7e:	vmovupd %ymm5,(%rsi)
  494f82:	vmovupd %ymm6,0x20(%rsi)
  494f87:	vzeroupper
  494f8a:	lea    -0x28(%rbp),%rsp
  494f8e:	pop    %rbx
  494f8f:	pop    %r12
  494f91:	pop    %r13
  494f93:	pop    %r14
  494f95:	pop    %r15
  494f97:	pop    %rbp
  494f98:	ret
  494f99:	lea    -0x28(%rbp),%rsp
  494f9d:	mov    %r8,%rdx
  494fa0:	mov    %rax,%rdi
  494fa3:	pop    %rbx
  494fa4:	pop    %r12
  494fa6:	pop    %r13
  494fa8:	pop    %r14
  494faa:	pop    %r15
  494fac:	pop    %rbp
  494fad:	jmp    494a50 <_ZN6micron4math3fft6__impl12__leaf_largeILm8ELb0EdEEvPKNS0_7complexIT1_EEPS6_m>
  494fb2:	lea    -0x28(%rbp),%rsp
  494fb6:	mov    %r8,%rdx
  494fb9:	mov    %rax,%rdi
  494fbc:	pop    %rbx
  494fbd:	pop    %r12
  494fbf:	pop    %r13
  494fc1:	pop    %r14
  494fc3:	pop    %r15
  494fc5:	pop    %rbp
  494fc6:	jmp    494750 <_ZN6micron4math3fft6__impl12__leaf_largeILm7ELb0EdEEvPKNS0_7complexIT1_EEPS6_m>
  494fcb:	lea    -0x28(%rbp),%rsp
  494fcf:	mov    %r8,%rdx
  494fd2:	mov    %rax,%rdi
  494fd5:	pop    %rbx
  494fd6:	pop    %r12
  494fd8:	pop    %r13
  494fda:	pop    %r14
  494fdc:	pop    %r15
  494fde:	pop    %rbp
  494fdf:	jmp    494570 <_ZN6micron4math3fft6__impl12__leaf_largeILm5ELb0EdEEvPKNS0_7complexIT1_EEPS6_m>
  494fe4:	shl    $0x4,%r8
  494fe8:	vmovupd (%rax),%xmm8
  494fec:	vmovddup 0x4d39c(%rip),%xmm13        # 4e2390 <_ZN6micron6__implL11__digit_tblE+0x490>
  494ff4:	add    %r8,%rax
  494ff7:	vmovddup 0x4d3c1(%rip),%xmm7        # 4e23c0 <_ZN6micron6__implL11__digit_tblE+0x4c0>
  494fff:	vmovupd (%rax),%xmm9
  495003:	vmovupd (%rax,%r8,1),%xmm10
  495009:	vunpckhpd %xmm8,%xmm8,%xmm3
  49500e:	vsubpd %xmm10,%xmm9,%xmm11
  495013:	vaddpd %xmm9,%xmm10,%xmm15
  495018:	vpermilpd $0x1,%xmm11,%xmm12
  49501e:	vfnmadd132pd %xmm15,%xmm8,%xmm7
  495023:	vunpckhpd %xmm15,%xmm15,%xmm4
  495028:	vmulpd %xmm13,%xmm12,%xmm14
  49502d:	vaddsd %xmm15,%xmm8,%xmm2
  495032:	vaddsd %xmm4,%xmm3,%xmm8
  495036:	vunpcklpd %xmm8,%xmm2,%xmm15
  49503b:	vunpckhpd %xmm7,%xmm7,%xmm10
  49503f:	vunpckhpd %xmm14,%xmm14,%xmm11
  495044:	vaddsd %xmm14,%xmm7,%xmm9
  495049:	vaddsubpd %xmm14,%xmm7,%xmm14
  49504e:	vsubsd %xmm11,%xmm10,%xmm12
  495053:	vmovupd %xmm14,0x20(%rsi)
  495058:	vunpcklpd %xmm12,%xmm9,%xmm13
  49505d:	vinsertf128 $0x1,%xmm13,%ymm15,%ymm0
  495063:	vmovupd %ymm0,(%rsi)
  495067:	vzeroupper
  49506a:	jmp    494f8a <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x32a>
  49506f:	shl    $0x4,%r8
  495073:	vmovsd (%rax),%xmm7
  495077:	vmovsd 0x8(%rax),%xmm8
  49507c:	add    %r8,%rax
  49507f:	vmovsd (%rax),%xmm1
  495083:	vmovsd 0x8(%rax),%xmm2
  495088:	vaddsd %xmm1,%xmm7,%xmm3
  49508c:	vaddsd %xmm2,%xmm8,%xmm4
  495090:	vsubsd %xmm1,%xmm7,%xmm6
  495094:	vsubsd %xmm2,%xmm8,%xmm5
  495098:	vmovsd %xmm3,(%rsi)
  49509c:	vmovsd %xmm4,0x8(%rsi)
  4950a1:	vmovsd %xmm6,0x10(%rsi)
  4950a6:	vmovsd %xmm5,0x18(%rsi)
  4950ab:	jmp    494f8a <_ZNK6micron4math3fft6__impl8__engineIdE14__smooth_stageILb0EEEvPKNS0_7complexIdEEPS7_mm+0x32a>

Disassembly of section .fini:
