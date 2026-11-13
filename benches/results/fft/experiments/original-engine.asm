000000000047b780 <void micron::math::fft::__impl::__engine<float>::__butterflies<8ul, false>(micron::math::complex<float>*, unsigned long, unsigned long, unsigned long, unsigned long) const>:
  47b780:	4d 39 c8             	cmp    %r9,%r8
  47b783:	0f 83 e7 05 00 00    	jae    47bd70 <void micron::math::fft::__impl::__engine<float>::__butterflies<8ul, false>(micron::math::complex<float>*, unsigned long, unsigned long, unsigned long, unsigned long) const+0x5f0>
  47b789:	55                   	push   %rbp
  47b78a:	48 89 e5             	mov    %rsp,%rbp
  47b78d:	41 57                	push   %r15
  47b78f:	41 56                	push   %r14
  47b791:	41 55                	push   %r13
  47b793:	41 54                	push   %r12
  47b795:	4d 89 c4             	mov    %r8,%r12
  47b798:	49 89 c8             	mov    %rcx,%r8
  47b79b:	48 89 f9             	mov    %rdi,%rcx
  47b79e:	53                   	push   %rbx
  47b79f:	48 89 d7             	mov    %rdx,%rdi
  47b7a2:	4c 89 c2             	mov    %r8,%rdx
  47b7a5:	4f 8d 2c 40          	lea    (%r8,%r8,2),%r13
  47b7a9:	48 c1 e2 04          	shl    $0x4,%rdx
  47b7ad:	4f 8d 1c 80          	lea    (%r8,%r8,4),%r11
  47b7b1:	4d 89 c7             	mov    %r8,%r15
  47b7b4:	4e 8d 34 dd 00 00 00 	lea    0x0(,%r11,8),%r14
  47b7bb:	00 
  47b7bc:	49 c1 e7 05          	shl    $0x5,%r15
  47b7c0:	48 83 e4 e0          	and    $0xffffffffffffffe0,%rsp
  47b7c4:	48 81 ec 40 01 00 00 	sub    $0x140,%rsp
  47b7cb:	48 8b 41 08          	mov    0x8(%rcx),%rax
  47b7cf:	48 8b 49 10          	mov    0x10(%rcx),%rcx
  47b7d3:	48 89 94 24 80 00 00 	mov    %rdx,0x80(%rsp)
  47b7da:	00 
  47b7db:	49 0f af d4          	imul   %r12,%rdx
  47b7df:	48 83 e0 f8          	and    $0xfffffffffffffff8,%rax
  47b7e3:	4c 89 4c 24 38       	mov    %r9,0x38(%rsp)
  47b7e8:	4e 8d 0c c5 00 00 00 	lea    0x0(,%r8,8),%r9
  47b7ef:	00 
  47b7f0:	48 8d 1c 01          	lea    (%rcx,%rax,1),%rbx
  47b7f4:	4c 89 7c 24 60       	mov    %r15,0x60(%rsp)
  47b7f9:	48 01 ca             	add    %rcx,%rdx
  47b7fc:	48 89 5c 24 70       	mov    %rbx,0x70(%rsp)
  47b801:	4d 0f af fc          	imul   %r12,%r15
  47b805:	48 01 c3             	add    %rax,%rbx
  47b808:	48 89 94 24 90 00 00 	mov    %rdx,0x90(%rsp)
  47b80f:	00 
  47b810:	49 6b d0 38          	imul   $0x38,%r8,%rdx
  47b814:	48 01 d8             	add    %rbx,%rax
  47b817:	4c 8d 04 fe          	lea    (%rsi,%rdi,8),%r8
  47b81b:	48 89 5c 24 78       	mov    %rbx,0x78(%rsp)
  47b820:	48 89 fb             	mov    %rdi,%rbx
  47b823:	48 89 44 24 68       	mov    %rax,0x68(%rsp)
  47b828:	48 c1 e3 04          	shl    $0x4,%rbx
  47b82c:	4a 8d 04 ed 00 00 00 	lea    0x0(,%r13,8),%rax
  47b833:	00 
  47b834:	49 c1 e5 04          	shl    $0x4,%r13
  47b838:	48 89 94 24 88 00 00 	mov    %rdx,0x88(%rsp)
  47b83f:	00 
  47b840:	49 0f af d4          	imul   %r12,%rdx
  47b844:	4c 8d 1c 1e          	lea    (%rsi,%rbx,1),%r11
  47b848:	4c 89 74 24 48       	mov    %r14,0x48(%rsp)
  47b84d:	4d 0f af f4          	imul   %r12,%r14
  47b851:	48 8d 1c 7f          	lea    (%rdi,%rdi,2),%rbx
  47b855:	4c 89 6c 24 40       	mov    %r13,0x40(%rsp)
  47b85a:	4d 0f af ec          	imul   %r12,%r13
  47b85e:	4c 8d 14 0a          	lea    (%rdx,%rcx,1),%r10
  47b862:	48 89 fa             	mov    %rdi,%rdx
  47b865:	4c 89 4c 24 58       	mov    %r9,0x58(%rsp)
  47b86a:	48 c1 e2 05          	shl    $0x5,%rdx
  47b86e:	4c 89 94 24 98 00 00 	mov    %r10,0x98(%rsp)
  47b875:	00 
  47b876:	4d 0f af cc          	imul   %r12,%r9
  47b87a:	4c 8d 14 16          	lea    (%rsi,%rdx,1),%r10
  47b87e:	48 8d 14 bf          	lea    (%rdi,%rdi,4),%rdx
  47b882:	48 89 44 24 50       	mov    %rax,0x50(%rsp)
  47b887:	48 6b ff 38          	imul   $0x38,%rdi,%rdi
  47b88b:	4c 89 9c 24 a8 00 00 	mov    %r11,0xa8(%rsp)
  47b892:	00 
  47b893:	4c 8d 1c de          	lea    (%rsi,%rbx,8),%r11
  47b897:	48 c1 e3 04          	shl    $0x4,%rbx
  47b89b:	49 0f af c4          	imul   %r12,%rax
  47b89f:	48 01 f3             	add    %rsi,%rbx
  47b8a2:	48 89 b4 24 a0 00 00 	mov    %rsi,0xa0(%rsp)
  47b8a9:	00 
  47b8aa:	49 01 c9             	add    %rcx,%r9
  47b8ad:	4c 89 94 24 b0 00 00 	mov    %r10,0xb0(%rsp)
  47b8b4:	00 
  47b8b5:	4c 8d 14 d6          	lea    (%rsi,%rdx,8),%r10
  47b8b9:	49 8d 14 0e          	lea    (%r14,%rcx,1),%rdx
  47b8bd:	48 89 9c 24 b8 00 00 	mov    %rbx,0xb8(%rsp)
  47b8c4:	00 
  47b8c5:	4c 89 c3             	mov    %r8,%rbx
  47b8c8:	4d 8d 44 0d 00       	lea    0x0(%r13,%rcx,1),%r8
  47b8cd:	4c 8d 2c 3e          	lea    (%rsi,%rdi,1),%r13
  47b8d1:	49 8d 34 0f          	lea    (%r15,%rcx,1),%rsi
  47b8d5:	4d 89 d6             	mov    %r10,%r14
  47b8d8:	4d 89 df             	mov    %r11,%r15
  47b8db:	48 01 c8             	add    %rcx,%rax
  47b8de:	49 89 f3             	mov    %rsi,%r11
  47b8e1:	49 89 d2             	mov    %rdx,%r10
  47b8e4:	90                   	nop
  47b8e5:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  47b8ec:	00 00 00 00 
  47b8f0:	48 8b bc 24 a0 00 00 	mov    0xa0(%rsp),%rdi
  47b8f7:	00 
  47b8f8:	48 8b 94 24 a8 00 00 	mov    0xa8(%rsp),%rdx
  47b8ff:	00 
  47b900:	48 8b b4 24 b0 00 00 	mov    0xb0(%rsp),%rsi
  47b907:	00 
  47b908:	c4 a1 7a 10 0c e3    	vmovss (%rbx,%r12,8),%xmm1
  47b90e:	c4 21 7a 7e 14 e7    	vmovq  (%rdi,%r12,8),%xmm10
  47b914:	48 8b bc 24 b8 00 00 	mov    0xb8(%rsp),%rdi
  47b91b:	00 
  47b91c:	c4 a1 7a 10 5c e3 04 	vmovss 0x4(%rbx,%r12,8),%xmm3
  47b923:	c4 21 7a 7e 3c e2    	vmovq  (%rdx,%r12,8),%xmm15
  47b929:	c4 01 7a 10 24 e7    	vmovss (%r15,%r12,8),%xmm12
  47b92f:	c4 81 7a 10 74 e7 04 	vmovss 0x4(%r15,%r12,8),%xmm6
  47b936:	c4 a1 7a 7e 04 e6    	vmovq  (%rsi,%r12,8),%xmm0
  47b93c:	c4 01 7a 10 2c e6    	vmovss (%r14,%r12,8),%xmm13
  47b942:	c4 01 7a 10 74 e6 04 	vmovss 0x4(%r14,%r12,8),%xmm14
  47b949:	c4 21 7a 7e 04 e7    	vmovq  (%rdi,%r12,8),%xmm8
  47b94f:	c4 01 7a 10 4c e5 00 	vmovss 0x0(%r13,%r12,8),%xmm9
  47b956:	c4 01 7a 10 5c e5 04 	vmovss 0x4(%r13,%r12,8),%xmm11
  47b95d:	4d 85 e4             	test   %r12,%r12
  47b960:	0f 84 a1 01 00 00    	je     47bb07 <void micron::math::fft::__impl::__engine<float>::__butterflies<8ul, false>(micron::math::complex<float>*, unsigned long, unsigned long, unsigned long, unsigned long) const+0x387>
  47b966:	48 8b 94 24 98 00 00 	mov    0x98(%rsp),%rdx
  47b96d:	00 
  47b96e:	c4 41 30 14 cb       	vunpcklps %xmm11,%xmm9,%xmm9
  47b973:	c4 41 10 14 ee       	vunpcklps %xmm14,%xmm13,%xmm13
  47b978:	c5 98 14 f6          	vunpcklps %xmm6,%xmm12,%xmm6
  47b97c:	c4 c1 7a 10 20       	vmovss (%r8),%xmm4
  47b981:	c4 c3 59 21 78 04 10 	vinsertps $0x10,0x4(%r8),%xmm4,%xmm7
  47b988:	48 89 44 24 20       	mov    %rax,0x20(%rsp)
  47b98d:	c5 f0 14 cb          	vunpcklps %xmm3,%xmm1,%xmm1
  47b991:	c5 fa 10 12          	vmovss (%rdx),%xmm2
  47b995:	c4 e3 69 21 6a 04 10 	vinsertps $0x10,0x4(%rdx),%xmm2,%xmm5
  47b99c:	4c 89 44 24 08       	mov    %r8,0x8(%rsp)
  47b9a1:	c5 00 16 e6          	vmovlhps %xmm6,%xmm15,%xmm12
  47b9a5:	c4 c1 7a 10 12       	vmovss (%r10),%xmm2
  47b9aa:	4c 89 54 24 10       	mov    %r10,0x10(%rsp)
  47b9af:	c5 a8 16 d9          	vmovlhps %xmm1,%xmm10,%xmm3
  47b9b3:	c4 41 38 16 c1       	vmovlhps %xmm9,%xmm8,%xmm8
  47b9b8:	c5 c0 16 e5          	vmovlhps %xmm5,%xmm7,%xmm4
  47b9bc:	c4 c3 69 21 6a 04 10 	vinsertps $0x10,0x4(%r10),%xmm2,%xmm5
  47b9c3:	c4 c1 7a 10 3b       	vmovss (%r11),%xmm7
  47b9c8:	4c 89 5c 24 18       	mov    %r11,0x18(%rsp)
  47b9cd:	c4 c3 41 21 53 04 10 	vinsertps $0x10,0x4(%r11),%xmm7,%xmm2
  47b9d4:	4c 89 4c 24 28       	mov    %r9,0x28(%rsp)
  47b9d9:	c4 43 65 18 d4 01    	vinsertf128 $0x1,%xmm12,%ymm3,%ymm10
  47b9df:	c4 c1 78 16 c5       	vmovlhps %xmm13,%xmm0,%xmm0
  47b9e4:	48 89 4c 24 30       	mov    %rcx,0x30(%rsp)
  47b9e9:	c4 43 7d 18 d8 01    	vinsertf128 $0x1,%xmm8,%ymm0,%ymm11
  47b9ef:	48 8d bc 24 c0 00 00 	lea    0xc0(%rsp),%rdi
  47b9f6:	00 
  47b9f7:	ba 08 00 00 00       	mov    $0x8,%edx
  47b9fc:	c5 e8 16 ed          	vmovlhps %xmm5,%xmm2,%xmm5
  47ba00:	c4 e3 55 18 d4 01    	vinsertf128 $0x1,%xmm4,%ymm5,%ymm2
  47ba06:	c5 fa 10 20          	vmovss (%rax),%xmm4
  47ba0a:	c4 e3 59 21 78 04 10 	vinsertps $0x10,0x4(%rax),%xmm4,%xmm7
  47ba11:	48 8b 84 24 90 00 00 	mov    0x90(%rsp),%rax
  47ba18:	00 
  47ba19:	48 8d b4 24 00 01 00 	lea    0x100(%rsp),%rsi
  47ba20:	00 
  47ba21:	c5 fa 10 28          	vmovss (%rax),%xmm5
  47ba25:	c4 e3 51 21 60 04 10 	vinsertps $0x10,0x4(%rax),%xmm5,%xmm4
  47ba2c:	c5 fa 10 29          	vmovss (%rcx),%xmm5
  47ba30:	c4 e3 51 21 69 04 10 	vinsertps $0x10,0x4(%rcx),%xmm5,%xmm5
  47ba37:	c5 7c 29 94 24 c0 00 	vmovaps %ymm10,0xc0(%rsp)
  47ba3e:	00 00 
  47ba40:	c5 7c 29 9c 24 e0 00 	vmovaps %ymm11,0xe0(%rsp)
  47ba47:	00 00 
  47ba49:	c5 d8 16 e7          	vmovlhps %xmm7,%xmm4,%xmm4
  47ba4d:	c4 c1 7a 10 39       	vmovss (%r9),%xmm7
  47ba52:	c4 c3 41 21 79 04 10 	vinsertps $0x10,0x4(%r9),%xmm7,%xmm7
  47ba59:	c5 fc 29 94 24 20 01 	vmovaps %ymm2,0x120(%rsp)
  47ba60:	00 00 
  47ba62:	c5 d0 16 ff          	vmovlhps %xmm7,%xmm5,%xmm7
  47ba66:	c4 e3 45 18 e4 01    	vinsertf128 $0x1,%xmm4,%ymm7,%ymm4
  47ba6c:	c5 fc 29 a4 24 00 01 	vmovaps %ymm4,0x100(%rsp)
  47ba73:	00 00 
  47ba75:	c5 f8 77             	vzeroupper
  47ba78:	e8 53 2d ff ff       	call   46e7d0 <void micron::math::fft::__impl::__multiply<float>(micron::math::complex<float>*, micron::math::complex<float> const*, unsigned long)>
  47ba7d:	4c 8b 44 24 08       	mov    0x8(%rsp),%r8
  47ba82:	c5 7a 7e 94 24 c0 00 	vmovq  0xc0(%rsp),%xmm10
  47ba89:	00 00 
  47ba8b:	c5 fa 10 8c 24 c8 00 	vmovss 0xc8(%rsp),%xmm1
  47ba92:	00 00 
  47ba94:	4c 8b 54 24 10       	mov    0x10(%rsp),%r10
  47ba99:	c5 fa 10 9c 24 cc 00 	vmovss 0xcc(%rsp),%xmm3
  47baa0:	00 00 
  47baa2:	4c 8b 5c 24 18       	mov    0x18(%rsp),%r11
  47baa7:	c5 7a 7e bc 24 d0 00 	vmovq  0xd0(%rsp),%xmm15
  47baae:	00 00 
  47bab0:	48 8b 44 24 20       	mov    0x20(%rsp),%rax
  47bab5:	c5 7a 10 a4 24 d8 00 	vmovss 0xd8(%rsp),%xmm12
  47babc:	00 00 
  47babe:	4c 8b 4c 24 28       	mov    0x28(%rsp),%r9
  47bac3:	c5 fa 10 b4 24 dc 00 	vmovss 0xdc(%rsp),%xmm6
  47baca:	00 00 
  47bacc:	48 8b 4c 24 30       	mov    0x30(%rsp),%rcx
  47bad1:	c5 fa 7e 84 24 e0 00 	vmovq  0xe0(%rsp),%xmm0
  47bad8:	00 00 
  47bada:	c5 7a 10 ac 24 e8 00 	vmovss 0xe8(%rsp),%xmm13
  47bae1:	00 00 
  47bae3:	c5 7a 10 b4 24 ec 00 	vmovss 0xec(%rsp),%xmm14
  47baea:	00 00 
  47baec:	c5 7a 7e 84 24 f0 00 	vmovq  0xf0(%rsp),%xmm8
  47baf3:	00 00 
  47baf5:	c5 7a 10 8c 24 f8 00 	vmovss 0xf8(%rsp),%xmm9
  47bafc:	00 00 
  47bafe:	c5 7a 10 9c 24 fc 00 	vmovss 0xfc(%rsp),%xmm11
  47bb05:	00 00 
  47bb07:	c4 41 7a 7e ff       	vmovq  %xmm15,%xmm15
  47bb0c:	c4 41 7a 7e c0       	vmovq  %xmm8,%xmm8
  47bb11:	c5 fa 7e c0          	vmovq  %xmm0,%xmm0
  47bb15:	48 8b 74 24 70       	mov    0x70(%rsp),%rsi
  47bb1a:	c4 c1 00 5c d0       	vsubps %xmm8,%xmm15,%xmm2
  47bb1f:	c4 41 7a 7e d2       	vmovq  %xmm10,%xmm10
  47bb24:	c4 41 38 58 c7       	vaddps %xmm15,%xmm8,%xmm8
  47bb29:	48 8b 7c 24 78       	mov    0x78(%rsp),%rdi
  47bb2e:	c5 a8 5c f8          	vsubps %xmm0,%xmm10,%xmm7
  47bb32:	c4 41 78 58 fa       	vaddps %xmm10,%xmm0,%xmm15
  47bb37:	48 8b 54 24 68       	mov    0x68(%rsp),%rdx
  47bb3c:	4c 03 4c 24 58       	add    0x58(%rsp),%r9
  47bb41:	48 03 44 24 50       	add    0x50(%rsp),%rax
  47bb46:	4c 03 5c 24 60       	add    0x60(%rsp),%r11
  47bb4b:	c5 e8 c6 ea e1       	vshufps $0xe1,%xmm2,%xmm2,%xmm5
  47bb50:	c4 41 7a 7e d0       	vmovq  %xmm8,%xmm10
  47bb55:	c4 41 72 58 c5       	vaddss %xmm13,%xmm1,%xmm8
  47bb5a:	4c 03 54 24 48       	add    0x48(%rsp),%r10
  47bb5f:	c5 fa 7e e5          	vmovq  %xmm5,%xmm4
  47bb63:	c5 fa 7e d7          	vmovq  %xmm7,%xmm2
  47bb67:	c5 79 d6 f8          	vmovq  %xmm15,%xmm0
  47bb6b:	4c 03 44 24 40       	add    0x40(%rsp),%r8
  47bb70:	c5 d8 58 ea          	vaddps %xmm2,%xmm4,%xmm5
  47bb74:	c5 e8 5c fc          	vsubps %xmm4,%xmm2,%xmm7
  47bb78:	c4 c1 72 5c cd       	vsubss %xmm13,%xmm1,%xmm1
  47bb7d:	c4 41 1a 58 f9       	vaddss %xmm9,%xmm12,%xmm15
  47bb82:	c5 eb d0 d4          	vaddsubps %xmm4,%xmm2,%xmm2
  47bb86:	c4 c1 1a 5c e1       	vsubss %xmm9,%xmm12,%xmm4
  47bb8b:	c4 e3 41 0c fd 01    	vblendps $0x1,%xmm5,%xmm7,%xmm7
  47bb91:	c5 a8 58 e8          	vaddps %xmm0,%xmm10,%xmm5
  47bb95:	c4 c1 78 5c c2       	vsubps %xmm10,%xmm0,%xmm0
  47bb9a:	c4 41 62 58 d6       	vaddss %xmm14,%xmm3,%xmm10
  47bb9f:	c4 c1 62 5c de       	vsubss %xmm14,%xmm3,%xmm3
  47bba4:	c5 fa 7e ff          	vmovq  %xmm7,%xmm7
  47bba8:	c4 41 4a 58 f3       	vaddss %xmm11,%xmm6,%xmm14
  47bbad:	c4 c1 4a 5c f3       	vsubss %xmm11,%xmm6,%xmm6
  47bbb2:	c5 fa 7e d2          	vmovq  %xmm2,%xmm2
  47bbb6:	c4 41 3a 58 cf       	vaddss %xmm15,%xmm8,%xmm9
  47bbbb:	c4 41 3a 5c c7       	vsubss %xmm15,%xmm8,%xmm8
  47bbc0:	c5 fa 7e ed          	vmovq  %xmm5,%xmm5
  47bbc4:	c5 fa 7e c0          	vmovq  %xmm0,%xmm0
  47bbc8:	c4 41 2a 58 ee       	vaddss %xmm14,%xmm10,%xmm13
  47bbcd:	c5 72 58 e6          	vaddss %xmm6,%xmm1,%xmm12
  47bbd1:	c4 41 2a 5c d6       	vsubss %xmm14,%xmm10,%xmm10
  47bbd6:	c5 f2 5c ce          	vsubss %xmm6,%xmm1,%xmm1
  47bbda:	c4 41 7a 12 f8       	vmovsldup %xmm8,%xmm15
  47bbdf:	c4 41 7a 12 c9       	vmovsldup %xmm9,%xmm9
  47bbe4:	c4 41 7a 12 ed       	vmovsldup %xmm13,%xmm13
  47bbe9:	c4 41 7a 12 dc       	vmovsldup %xmm12,%xmm11
  47bbee:	c5 62 5c e4          	vsubss %xmm4,%xmm3,%xmm12
  47bbf2:	c4 41 7a 12 f2       	vmovsldup %xmm10,%xmm14
  47bbf7:	c5 fa 12 c9          	vmovsldup %xmm1,%xmm1
  47bbfb:	c5 e2 58 dc          	vaddss %xmm4,%xmm3,%xmm3
  47bbff:	c5 fa 7e 21          	vmovq  (%rcx),%xmm4
  47bc03:	c4 41 7a 7e ff       	vmovq  %xmm15,%xmm15
  47bc08:	c5 d8 c6 f4 e1       	vshufps $0xe1,%xmm4,%xmm4,%xmm6
  47bc0d:	c4 41 7a 7e c9       	vmovq  %xmm9,%xmm9
  47bc12:	c5 fa 7e e4          	vmovq  %xmm4,%xmm4
  47bc16:	c4 41 7a 7e c5       	vmovq  %xmm13,%xmm8
  47bc1b:	c5 7a 7e d6          	vmovq  %xmm6,%xmm10
  47bc1f:	c4 41 7a 7e db       	vmovq  %xmm11,%xmm11
  47bc24:	c4 c1 28 59 f0       	vmulps %xmm8,%xmm10,%xmm6
  47bc29:	c4 41 7a 12 e4       	vmovsldup %xmm12,%xmm12
  47bc2e:	c5 7a 7e 16          	vmovq  (%rsi),%xmm10
  47bc32:	c4 41 7a 7e f6       	vmovq  %xmm14,%xmm14
  47bc37:	c5 fa 12 db          	vmovsldup %xmm3,%xmm3
  47bc3b:	c5 fa 7e c9          	vmovq  %xmm1,%xmm1
  47bc3f:	48 8b b4 24 a0 00 00 	mov    0xa0(%rsp),%rsi
  47bc46:	00 
  47bc47:	c4 41 7a 7e e4       	vmovq  %xmm12,%xmm12
  47bc4c:	c5 7a 7e ee          	vmovq  %xmm6,%xmm13
  47bc50:	c4 c2 11 96 e1       	vfmaddsub132ps %xmm9,%xmm13,%xmm4
  47bc55:	c4 41 28 c6 ca e1    	vshufps $0xe1,%xmm10,%xmm10,%xmm9
  47bc5b:	c4 41 7a 7e d2       	vmovq  %xmm10,%xmm10
  47bc60:	c5 7a 7e c4          	vmovq  %xmm4,%xmm8
  47bc64:	c5 79 d6 cc          	vmovq  %xmm9,%xmm4
  47bc68:	c5 b8 58 f5          	vaddps %xmm5,%xmm8,%xmm6
  47bc6c:	c4 41 50 5c e8       	vsubps %xmm8,%xmm5,%xmm13
  47bc71:	c4 c1 58 59 ec       	vmulps %xmm12,%xmm4,%xmm5
  47bc76:	c5 fa 7e 27          	vmovq  (%rdi),%xmm4
  47bc7a:	48 8b bc 24 a8 00 00 	mov    0xa8(%rsp),%rdi
  47bc81:	00 
  47bc82:	c5 7a 7e c5          	vmovq  %xmm5,%xmm8
  47bc86:	c4 42 39 96 d3       	vfmaddsub132ps %xmm11,%xmm8,%xmm10
  47bc8b:	c5 58 c6 c4 e1       	vshufps $0xe1,%xmm4,%xmm4,%xmm8
  47bc90:	c4 41 7a 7e d8       	vmovq  %xmm8,%xmm11
  47bc95:	c4 41 7a 7e e2       	vmovq  %xmm10,%xmm12
  47bc9a:	c4 41 20 59 d6       	vmulps %xmm14,%xmm11,%xmm10
  47bc9f:	c5 7a 7e 1a          	vmovq  (%rdx),%xmm11
  47bca3:	c5 18 58 cf          	vaddps %xmm7,%xmm12,%xmm9
  47bca7:	c4 c1 40 5c ec       	vsubps %xmm12,%xmm7,%xmm5
  47bcac:	c5 7a 7e e4          	vmovq  %xmm4,%xmm12
  47bcb0:	c4 a1 78 13 34 e6    	vmovlps %xmm6,(%rsi,%r12,8)
  47bcb6:	48 8b 94 24 b0 00 00 	mov    0xb0(%rsp),%rdx
  47bcbd:	00 
  47bcbe:	48 8b b4 24 b8 00 00 	mov    0xb8(%rsp),%rsi
  47bcc5:	00 
  47bcc6:	c4 21 78 13 0c e3    	vmovlps %xmm9,(%rbx,%r12,8)
  47bccc:	c5 79 d6 d7          	vmovq  %xmm10,%xmm7
  47bcd0:	c4 41 20 c6 d3 e1    	vshufps $0xe1,%xmm11,%xmm11,%xmm10
  47bcd6:	c4 42 41 96 e7       	vfmaddsub132ps %xmm15,%xmm7,%xmm12
  47bcdb:	c5 fa 7e fb          	vmovq  %xmm3,%xmm7
  47bcdf:	c5 79 d6 d3          	vmovq  %xmm10,%xmm3
  47bce3:	c5 60 59 ff          	vmulps %xmm7,%xmm3,%xmm15
  47bce7:	c5 79 d6 e4          	vmovq  %xmm12,%xmm4
  47bceb:	c5 78 5c c4          	vsubps %xmm4,%xmm0,%xmm8
  47bcef:	c5 78 58 f4          	vaddps %xmm4,%xmm0,%xmm14
  47bcf3:	c4 41 7a 7e e7       	vmovq  %xmm15,%xmm12
  47bcf8:	c5 79 d6 dc          	vmovq  %xmm11,%xmm4
  47bcfc:	c4 e2 19 96 e1       	vfmaddsub132ps %xmm1,%xmm12,%xmm4
  47bd01:	c4 21 78 13 34 e7    	vmovlps %xmm14,(%rdi,%r12,8)
  47bd07:	48 8b bc 24 80 00 00 	mov    0x80(%rsp),%rdi
  47bd0e:	00 
  47bd0f:	48 01 bc 24 90 00 00 	add    %rdi,0x90(%rsp)
  47bd16:	00 
  47bd17:	c5 fa 7e f4          	vmovq  %xmm4,%xmm6
  47bd1b:	c5 68 58 ce          	vaddps %xmm6,%xmm2,%xmm9
  47bd1f:	c4 01 78 13 0c e7    	vmovlps %xmm9,(%r15,%r12,8)
  47bd25:	c4 21 78 13 2c e2    	vmovlps %xmm13,(%rdx,%r12,8)
  47bd2b:	c5 68 5c ee          	vsubps %xmm6,%xmm2,%xmm13
  47bd2f:	48 8b 94 24 88 00 00 	mov    0x88(%rsp),%rdx
  47bd36:	00 
  47bd37:	c4 81 78 13 2c e6    	vmovlps %xmm5,(%r14,%r12,8)
  47bd3d:	48 01 94 24 98 00 00 	add    %rdx,0x98(%rsp)
  47bd44:	00 
  47bd45:	c4 21 78 13 04 e6    	vmovlps %xmm8,(%rsi,%r12,8)
  47bd4b:	c4 01 78 13 6c e5 00 	vmovlps %xmm13,0x0(%r13,%r12,8)
  47bd52:	49 83 c4 01          	add    $0x1,%r12
  47bd56:	4c 39 64 24 38       	cmp    %r12,0x38(%rsp)
  47bd5b:	0f 85 8f fb ff ff    	jne    47b8f0 <void micron::math::fft::__impl::__engine<float>::__butterflies<8ul, false>(micron::math::complex<float>*, unsigned long, unsigned long, unsigned long, unsigned long) const+0x170>
  47bd61:	48 8d 65 d8          	lea    -0x28(%rbp),%rsp
  47bd65:	5b                   	pop    %rbx
  47bd66:	41 5c                	pop    %r12
  47bd68:	41 5d                	pop    %r13
  47bd6a:	41 5e                	pop    %r14
  47bd6c:	41 5f                	pop    %r15
  47bd6e:	5d                   	pop    %rbp
  47bd6f:	c3                   	ret
  47bd70:	c3                   	ret
  47bd71:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  47bd78:	00 00 00 
  47bd7b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)


000000000047c940 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const>:
  47c940:	48 3b 8f 80 00 00 00 	cmp    0x80(%rdi),%rcx
  47c947:	0f 84 23 03 00 00    	je     47cc70 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x330>
  47c94d:	55                   	push   %rbp
  47c94e:	48 89 c8             	mov    %rcx,%rax
  47c951:	48 89 e5             	mov    %rsp,%rbp
  47c954:	48 83 ec 70          	sub    $0x70,%rsp
  47c958:	4c 89 65 e0          	mov    %r12,-0x20(%rbp)
  47c95c:	49 89 d4             	mov    %rdx,%r12
  47c95f:	48 8b 57 70          	mov    0x70(%rdi),%rdx
  47c963:	4c 89 7d f8          	mov    %r15,-0x8(%rbp)
  47c967:	49 89 ff             	mov    %rdi,%r15
  47c96a:	4c 8b 0c ca          	mov    (%rdx,%rcx,8),%r9
  47c96e:	48 8b 4f 58          	mov    0x58(%rdi),%rcx
  47c972:	4c 8b 5c c1 08       	mov    0x8(%rcx,%rax,8),%r11
  47c977:	49 83 fb 01          	cmp    $0x1,%r11
  47c97b:	0f 84 df 01 00 00    	je     47cb60 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x220>
  47c981:	4d 85 c9             	test   %r9,%r9
  47c984:	0f 84 b5 01 00 00    	je     47cb3f <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x1ff>
  47c98a:	48 89 5d d8          	mov    %rbx,-0x28(%rbp)
  47c98e:	4c 89 cb             	mov    %r9,%rbx
  47c991:	4c 89 cf             	mov    %r9,%rdi
  47c994:	4c 89 e2             	mov    %r12,%rdx
  47c997:	49 0f af d8          	imul   %r8,%rbx
  47c99b:	4c 89 6d e8          	mov    %r13,-0x18(%rbp)
  47c99f:	45 31 d2             	xor    %r10d,%r10d
  47c9a2:	4e 8d 2c dd 00 00 00 	lea    0x0(,%r11,8),%r13
  47c9a9:	00 
  47c9aa:	4c 89 75 f0          	mov    %r14,-0x10(%rbp)
  47c9ae:	4c 8d 70 01          	lea    0x1(%rax),%r14
  47c9b2:	48 89 5d c8          	mov    %rbx,-0x38(%rbp)
  47c9b6:	4a 8d 1c c5 00 00 00 	lea    0x0(,%r8,8),%rbx
  47c9bd:	00 
  47c9be:	83 e7 03             	and    $0x3,%edi
  47c9c1:	0f 84 ab 00 00 00    	je     47ca72 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x132>
  47c9c7:	48 83 ff 01          	cmp    $0x1,%rdi
  47c9cb:	74 53                	je     47ca20 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0xe0>
  47c9cd:	48 83 ff 02          	cmp    $0x2,%rdi
  47c9d1:	0f 85 a9 02 00 00    	jne    47cc80 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x340>
  47c9d7:	4c 8b 45 c8          	mov    -0x38(%rbp),%r8
  47c9db:	4c 89 f1             	mov    %r14,%rcx
  47c9de:	4c 89 ff             	mov    %r15,%rdi
  47c9e1:	48 89 45 98          	mov    %rax,-0x68(%rbp)
  47c9e5:	4c 89 55 a0          	mov    %r10,-0x60(%rbp)
  47c9e9:	4c 89 5d a8          	mov    %r11,-0x58(%rbp)
  47c9ed:	4c 89 4d b0          	mov    %r9,-0x50(%rbp)
  47c9f1:	48 89 55 b8          	mov    %rdx,-0x48(%rbp)
  47c9f5:	48 89 75 c0          	mov    %rsi,-0x40(%rbp)
  47c9f9:	e8 42 ff ff ff       	call   47c940 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const>
  47c9fe:	4c 8b 55 a0          	mov    -0x60(%rbp),%r10
  47ca02:	48 8b 75 c0          	mov    -0x40(%rbp),%rsi
  47ca06:	48 8b 55 b8          	mov    -0x48(%rbp),%rdx
  47ca0a:	48 8b 45 98          	mov    -0x68(%rbp),%rax
  47ca0e:	4c 8b 5d a8          	mov    -0x58(%rbp),%r11
  47ca12:	4c 8b 4d b0          	mov    -0x50(%rbp),%r9
  47ca16:	49 83 c2 01          	add    $0x1,%r10
  47ca1a:	48 01 de             	add    %rbx,%rsi
  47ca1d:	4c 01 ea             	add    %r13,%rdx
  47ca20:	4c 8b 45 c8          	mov    -0x38(%rbp),%r8
  47ca24:	4c 89 f1             	mov    %r14,%rcx
  47ca27:	4c 89 ff             	mov    %r15,%rdi
  47ca2a:	48 89 45 98          	mov    %rax,-0x68(%rbp)
  47ca2e:	4c 89 55 a0          	mov    %r10,-0x60(%rbp)
  47ca32:	4c 89 5d a8          	mov    %r11,-0x58(%rbp)
  47ca36:	4c 89 4d b0          	mov    %r9,-0x50(%rbp)
  47ca3a:	48 89 55 b8          	mov    %rdx,-0x48(%rbp)
  47ca3e:	48 89 75 c0          	mov    %rsi,-0x40(%rbp)
  47ca42:	e8 f9 fe ff ff       	call   47c940 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const>
  47ca47:	4c 8b 55 a0          	mov    -0x60(%rbp),%r10
  47ca4b:	48 8b 75 c0          	mov    -0x40(%rbp),%rsi
  47ca4f:	48 8b 55 b8          	mov    -0x48(%rbp),%rdx
  47ca53:	4c 8b 4d b0          	mov    -0x50(%rbp),%r9
  47ca57:	49 83 c2 01          	add    $0x1,%r10
  47ca5b:	48 01 de             	add    %rbx,%rsi
  47ca5e:	4c 8b 5d a8          	mov    -0x58(%rbp),%r11
  47ca62:	48 8b 45 98          	mov    -0x68(%rbp),%rax
  47ca66:	4c 01 ea             	add    %r13,%rdx
  47ca69:	4d 39 d1             	cmp    %r10,%r9
  47ca6c:	0f 84 c1 00 00 00    	je     47cb33 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x1f3>
  47ca72:	4c 89 4d b0          	mov    %r9,-0x50(%rbp)
  47ca76:	4c 89 5d a8          	mov    %r11,-0x58(%rbp)
  47ca7a:	48 89 45 98          	mov    %rax,-0x68(%rbp)
  47ca7e:	4c 89 65 a0          	mov    %r12,-0x60(%rbp)
  47ca82:	4d 89 d4             	mov    %r10,%r12
  47ca85:	4c 8b 45 c8          	mov    -0x38(%rbp),%r8
  47ca89:	4c 89 f1             	mov    %r14,%rcx
  47ca8c:	4c 89 ff             	mov    %r15,%rdi
  47ca8f:	48 89 55 b8          	mov    %rdx,-0x48(%rbp)
  47ca93:	48 89 75 c0          	mov    %rsi,-0x40(%rbp)
  47ca97:	49 83 c4 04          	add    $0x4,%r12
  47ca9b:	e8 a0 fe ff ff       	call   47c940 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const>
  47caa0:	48 8b 75 c0          	mov    -0x40(%rbp),%rsi
  47caa4:	48 8b 55 b8          	mov    -0x48(%rbp),%rdx
  47caa8:	4c 89 f1             	mov    %r14,%rcx
  47caab:	4c 8b 45 c8          	mov    -0x38(%rbp),%r8
  47caaf:	4c 89 ff             	mov    %r15,%rdi
  47cab2:	48 01 de             	add    %rbx,%rsi
  47cab5:	4c 01 ea             	add    %r13,%rdx
  47cab8:	48 89 55 b8          	mov    %rdx,-0x48(%rbp)
  47cabc:	48 89 75 c0          	mov    %rsi,-0x40(%rbp)
  47cac0:	e8 7b fe ff ff       	call   47c940 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const>
  47cac5:	48 8b 75 c0          	mov    -0x40(%rbp),%rsi
  47cac9:	48 8b 55 b8          	mov    -0x48(%rbp),%rdx
  47cacd:	4c 89 f1             	mov    %r14,%rcx
  47cad0:	4c 8b 45 c8          	mov    -0x38(%rbp),%r8
  47cad4:	4c 89 ff             	mov    %r15,%rdi
  47cad7:	48 01 de             	add    %rbx,%rsi
  47cada:	4c 01 ea             	add    %r13,%rdx
  47cadd:	48 89 55 b8          	mov    %rdx,-0x48(%rbp)
  47cae1:	48 89 75 c0          	mov    %rsi,-0x40(%rbp)
  47cae5:	e8 56 fe ff ff       	call   47c940 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const>
  47caea:	48 8b 75 c0          	mov    -0x40(%rbp),%rsi
  47caee:	48 8b 55 b8          	mov    -0x48(%rbp),%rdx
  47caf2:	4c 89 f1             	mov    %r14,%rcx
  47caf5:	4c 8b 45 c8          	mov    -0x38(%rbp),%r8
  47caf9:	4c 89 ff             	mov    %r15,%rdi
  47cafc:	48 01 de             	add    %rbx,%rsi
  47caff:	4c 01 ea             	add    %r13,%rdx
  47cb02:	48 89 55 b8          	mov    %rdx,-0x48(%rbp)
  47cb06:	48 89 75 c0          	mov    %rsi,-0x40(%rbp)
  47cb0a:	e8 31 fe ff ff       	call   47c940 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const>
  47cb0f:	48 8b 75 c0          	mov    -0x40(%rbp),%rsi
  47cb13:	48 8b 55 b8          	mov    -0x48(%rbp),%rdx
  47cb17:	48 01 de             	add    %rbx,%rsi
  47cb1a:	4c 01 ea             	add    %r13,%rdx
  47cb1d:	4c 39 65 b0          	cmp    %r12,-0x50(%rbp)
  47cb21:	0f 85 5e ff ff ff    	jne    47ca85 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x145>
  47cb27:	4c 8b 5d a8          	mov    -0x58(%rbp),%r11
  47cb2b:	4c 8b 65 a0          	mov    -0x60(%rbp),%r12
  47cb2f:	48 8b 45 98          	mov    -0x68(%rbp),%rax
  47cb33:	48 8b 5d d8          	mov    -0x28(%rbp),%rbx
  47cb37:	4c 8b 6d e8          	mov    -0x18(%rbp),%r13
  47cb3b:	4c 8b 75 f0          	mov    -0x10(%rbp),%r14
  47cb3f:	4c 89 e6             	mov    %r12,%rsi
  47cb42:	4c 89 ff             	mov    %r15,%rdi
  47cb45:	4c 8b 65 e0          	mov    -0x20(%rbp),%r12
  47cb49:	4c 8b 7d f8          	mov    -0x8(%rbp),%r15
  47cb4d:	c9                   	leave
  47cb4e:	4d 89 d8             	mov    %r11,%r8
  47cb51:	31 c9                	xor    %ecx,%ecx
  47cb53:	48 89 c2             	mov    %rax,%rdx
  47cb56:	e9 25 f2 ff ff       	jmp    47bd80 <void micron::math::fft::__impl::__engine<float>::__combine_stage<false>(micron::math::complex<float>*, unsigned long, unsigned long, unsigned long) const>
  47cb5b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  47cb60:	4d 85 c9             	test   %r9,%r9
  47cb63:	74 da                	je     47cb3f <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x1ff>
  47cb65:	31 d2                	xor    %edx,%edx
  47cb67:	49 83 f8 01          	cmp    $0x1,%r8
  47cb6b:	0f 85 57 01 00 00    	jne    47ccc8 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x388>
  47cb71:	4d 89 c8             	mov    %r9,%r8
  47cb74:	41 83 e0 07          	and    $0x7,%r8d
  47cb78:	0f 84 81 00 00 00    	je     47cbff <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x2bf>
  47cb7e:	49 83 f8 01          	cmp    $0x1,%r8
  47cb82:	74 66                	je     47cbea <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x2aa>
  47cb84:	49 83 f8 02          	cmp    $0x2,%r8
  47cb88:	74 54                	je     47cbde <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x29e>
  47cb8a:	49 83 f8 03          	cmp    $0x3,%r8
  47cb8e:	74 42                	je     47cbd2 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x292>
  47cb90:	49 83 f8 04          	cmp    $0x4,%r8
  47cb94:	74 30                	je     47cbc6 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x286>
  47cb96:	49 83 f8 05          	cmp    $0x5,%r8
  47cb9a:	74 1e                	je     47cbba <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x27a>
  47cb9c:	49 83 f8 06          	cmp    $0x6,%r8
  47cba0:	74 0c                	je     47cbae <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x26e>
  47cba2:	4c 8b 16             	mov    (%rsi),%r10
  47cba5:	ba 01 00 00 00       	mov    $0x1,%edx
  47cbaa:	4d 89 14 24          	mov    %r10,(%r12)
  47cbae:	48 8b 0c d6          	mov    (%rsi,%rdx,8),%rcx
  47cbb2:	49 89 0c d4          	mov    %rcx,(%r12,%rdx,8)
  47cbb6:	48 83 c2 01          	add    $0x1,%rdx
  47cbba:	48 8b 3c d6          	mov    (%rsi,%rdx,8),%rdi
  47cbbe:	49 89 3c d4          	mov    %rdi,(%r12,%rdx,8)
  47cbc2:	48 83 c2 01          	add    $0x1,%rdx
  47cbc6:	4c 8b 04 d6          	mov    (%rsi,%rdx,8),%r8
  47cbca:	4d 89 04 d4          	mov    %r8,(%r12,%rdx,8)
  47cbce:	48 83 c2 01          	add    $0x1,%rdx
  47cbd2:	4c 8b 14 d6          	mov    (%rsi,%rdx,8),%r10
  47cbd6:	4d 89 14 d4          	mov    %r10,(%r12,%rdx,8)
  47cbda:	48 83 c2 01          	add    $0x1,%rdx
  47cbde:	48 8b 0c d6          	mov    (%rsi,%rdx,8),%rcx
  47cbe2:	49 89 0c d4          	mov    %rcx,(%r12,%rdx,8)
  47cbe6:	48 83 c2 01          	add    $0x1,%rdx
  47cbea:	48 8b 3c d6          	mov    (%rsi,%rdx,8),%rdi
  47cbee:	49 89 3c d4          	mov    %rdi,(%r12,%rdx,8)
  47cbf2:	48 83 c2 01          	add    $0x1,%rdx
  47cbf6:	49 39 d1             	cmp    %rdx,%r9
  47cbf9:	0f 84 40 ff ff ff    	je     47cb3f <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x1ff>
  47cbff:	4c 8b 04 d6          	mov    (%rsi,%rdx,8),%r8
  47cc03:	4c 8d 52 01          	lea    0x1(%rdx),%r10
  47cc07:	4d 89 04 d4          	mov    %r8,(%r12,%rdx,8)
  47cc0b:	4a 8b 0c d6          	mov    (%rsi,%r10,8),%rcx
  47cc0f:	4c 8d 42 02          	lea    0x2(%rdx),%r8
  47cc13:	4b 89 0c d4          	mov    %rcx,(%r12,%r10,8)
  47cc17:	4a 8b 3c c6          	mov    (%rsi,%r8,8),%rdi
  47cc1b:	4c 8d 52 03          	lea    0x3(%rdx),%r10
  47cc1f:	4b 89 3c c4          	mov    %rdi,(%r12,%r8,8)
  47cc23:	4a 8b 0c d6          	mov    (%rsi,%r10,8),%rcx
  47cc27:	4c 8d 42 04          	lea    0x4(%rdx),%r8
  47cc2b:	4b 89 0c d4          	mov    %rcx,(%r12,%r10,8)
  47cc2f:	4a 8b 3c c6          	mov    (%rsi,%r8,8),%rdi
  47cc33:	4c 8d 52 05          	lea    0x5(%rdx),%r10
  47cc37:	4b 89 3c c4          	mov    %rdi,(%r12,%r8,8)
  47cc3b:	4a 8b 0c d6          	mov    (%rsi,%r10,8),%rcx
  47cc3f:	4c 8d 42 06          	lea    0x6(%rdx),%r8
  47cc43:	4b 89 0c d4          	mov    %rcx,(%r12,%r10,8)
  47cc47:	4a 8b 3c c6          	mov    (%rsi,%r8,8),%rdi
  47cc4b:	4c 8d 52 07          	lea    0x7(%rdx),%r10
  47cc4f:	48 83 c2 08          	add    $0x8,%rdx
  47cc53:	4b 89 3c c4          	mov    %rdi,(%r12,%r8,8)
  47cc57:	4a 8b 0c d6          	mov    (%rsi,%r10,8),%rcx
  47cc5b:	4b 89 0c d4          	mov    %rcx,(%r12,%r10,8)
  47cc5f:	49 39 d1             	cmp    %rdx,%r9
  47cc62:	75 9b                	jne    47cbff <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x2bf>
  47cc64:	e9 d6 fe ff ff       	jmp    47cb3f <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x1ff>
  47cc69:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  47cc70:	4c 8b 1e             	mov    (%rsi),%r11
  47cc73:	4c 89 1a             	mov    %r11,(%rdx)
  47cc76:	c3                   	ret
  47cc77:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
  47cc7e:	00 00 
  47cc80:	4c 8b 45 c8          	mov    -0x38(%rbp),%r8
  47cc84:	4c 89 f1             	mov    %r14,%rcx
  47cc87:	4c 89 ff             	mov    %r15,%rdi
  47cc8a:	48 89 45 a8          	mov    %rax,-0x58(%rbp)
  47cc8e:	4c 89 5d b0          	mov    %r11,-0x50(%rbp)
  47cc92:	4c 89 4d b8          	mov    %r9,-0x48(%rbp)
  47cc96:	48 89 75 c0          	mov    %rsi,-0x40(%rbp)
  47cc9a:	e8 a1 fc ff ff       	call   47c940 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const>
  47cc9f:	48 8b 75 c0          	mov    -0x40(%rbp),%rsi
  47cca3:	48 8b 45 a8          	mov    -0x58(%rbp),%rax
  47cca7:	41 ba 01 00 00 00    	mov    $0x1,%r10d
  47ccad:	4c 8b 5d b0          	mov    -0x50(%rbp),%r11
  47ccb1:	4c 8b 4d b8          	mov    -0x48(%rbp),%r9
  47ccb5:	4b 8d 14 2c          	lea    (%r12,%r13,1),%rdx
  47ccb9:	48 01 de             	add    %rbx,%rsi
  47ccbc:	e9 16 fd ff ff       	jmp    47c9d7 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x97>
  47ccc1:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  47ccc8:	4a 8d 3c cd f8 ff ff 	lea    -0x8(,%r9,8),%rdi
  47cccf:	ff 
  47ccd0:	48 89 5d d8          	mov    %rbx,-0x28(%rbp)
  47ccd4:	49 89 f2             	mov    %rsi,%r10
  47ccd7:	4c 89 e1             	mov    %r12,%rcx
  47ccda:	48 c1 ef 03          	shr    $0x3,%rdi
  47ccde:	4a 8d 1c c5 00 00 00 	lea    0x0(,%r8,8),%rbx
  47cce5:	00 
  47cce6:	4f 8d 04 cc          	lea    (%r12,%r9,8),%r8
  47ccea:	48 83 c7 01          	add    $0x1,%rdi
  47ccee:	83 e7 07             	and    $0x7,%edi
  47ccf1:	0f 84 8d 00 00 00    	je     47cd84 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x444>
  47ccf7:	48 83 ff 01          	cmp    $0x1,%rdi
  47ccfb:	74 74                	je     47cd71 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x431>
  47ccfd:	48 83 ff 02          	cmp    $0x2,%rdi
  47cd01:	74 60                	je     47cd63 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x423>
  47cd03:	48 83 ff 03          	cmp    $0x3,%rdi
  47cd07:	74 4c                	je     47cd55 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x415>
  47cd09:	48 83 ff 04          	cmp    $0x4,%rdi
  47cd0d:	74 38                	je     47cd47 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x407>
  47cd0f:	48 83 ff 05          	cmp    $0x5,%rdi
  47cd13:	74 24                	je     47cd39 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x3f9>
  47cd15:	48 83 ff 06          	cmp    $0x6,%rdi
  47cd19:	74 10                	je     47cd2b <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x3eb>
  47cd1b:	48 8b 16             	mov    (%rsi),%rdx
  47cd1e:	4c 8d 14 1e          	lea    (%rsi,%rbx,1),%r10
  47cd22:	49 8d 4c 24 08       	lea    0x8(%r12),%rcx
  47cd27:	49 89 14 24          	mov    %rdx,(%r12)
  47cd2b:	49 8b 32             	mov    (%r10),%rsi
  47cd2e:	48 83 c1 08          	add    $0x8,%rcx
  47cd32:	49 01 da             	add    %rbx,%r10
  47cd35:	48 89 71 f8          	mov    %rsi,-0x8(%rcx)
  47cd39:	4d 8b 0a             	mov    (%r10),%r9
  47cd3c:	48 83 c1 08          	add    $0x8,%rcx
  47cd40:	49 01 da             	add    %rbx,%r10
  47cd43:	4c 89 49 f8          	mov    %r9,-0x8(%rcx)
  47cd47:	49 8b 3a             	mov    (%r10),%rdi
  47cd4a:	48 83 c1 08          	add    $0x8,%rcx
  47cd4e:	49 01 da             	add    %rbx,%r10
  47cd51:	48 89 79 f8          	mov    %rdi,-0x8(%rcx)
  47cd55:	49 8b 12             	mov    (%r10),%rdx
  47cd58:	48 83 c1 08          	add    $0x8,%rcx
  47cd5c:	49 01 da             	add    %rbx,%r10
  47cd5f:	48 89 51 f8          	mov    %rdx,-0x8(%rcx)
  47cd63:	49 8b 32             	mov    (%r10),%rsi
  47cd66:	48 83 c1 08          	add    $0x8,%rcx
  47cd6a:	49 01 da             	add    %rbx,%r10
  47cd6d:	48 89 71 f8          	mov    %rsi,-0x8(%rcx)
  47cd71:	4d 8b 0a             	mov    (%r10),%r9
  47cd74:	48 83 c1 08          	add    $0x8,%rcx
  47cd78:	49 01 da             	add    %rbx,%r10
  47cd7b:	4c 89 49 f8          	mov    %r9,-0x8(%rcx)
  47cd7f:	4c 39 c1             	cmp    %r8,%rcx
  47cd82:	74 59                	je     47cddd <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x49d>
  47cd84:	49 8b 3a             	mov    (%r10),%rdi
  47cd87:	49 01 da             	add    %rbx,%r10
  47cd8a:	48 83 c1 40          	add    $0x40,%rcx
  47cd8e:	48 89 79 c0          	mov    %rdi,-0x40(%rcx)
  47cd92:	49 8b 12             	mov    (%r10),%rdx
  47cd95:	49 01 da             	add    %rbx,%r10
  47cd98:	48 89 51 c8          	mov    %rdx,-0x38(%rcx)
  47cd9c:	49 8b 32             	mov    (%r10),%rsi
  47cd9f:	49 01 da             	add    %rbx,%r10
  47cda2:	48 89 71 d0          	mov    %rsi,-0x30(%rcx)
  47cda6:	4d 8b 0a             	mov    (%r10),%r9
  47cda9:	49 01 da             	add    %rbx,%r10
  47cdac:	4c 89 49 d8          	mov    %r9,-0x28(%rcx)
  47cdb0:	49 8b 3a             	mov    (%r10),%rdi
  47cdb3:	49 01 da             	add    %rbx,%r10
  47cdb6:	48 89 79 e0          	mov    %rdi,-0x20(%rcx)
  47cdba:	49 8b 12             	mov    (%r10),%rdx
  47cdbd:	49 01 da             	add    %rbx,%r10
  47cdc0:	48 89 51 e8          	mov    %rdx,-0x18(%rcx)
  47cdc4:	49 8b 32             	mov    (%r10),%rsi
  47cdc7:	49 01 da             	add    %rbx,%r10
  47cdca:	48 89 71 f0          	mov    %rsi,-0x10(%rcx)
  47cdce:	4d 8b 0a             	mov    (%r10),%r9
  47cdd1:	49 01 da             	add    %rbx,%r10
  47cdd4:	4c 89 49 f8          	mov    %r9,-0x8(%rcx)
  47cdd8:	4c 39 c1             	cmp    %r8,%rcx
  47cddb:	75 a7                	jne    47cd84 <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x444>
  47cddd:	48 8b 5d d8          	mov    -0x28(%rbp),%rbx
  47cde1:	e9 59 fd ff ff       	jmp    47cb3f <void micron::math::fft::__impl::__engine<float>::__smooth_stage<false>(micron::math::complex<float> const*, micron::math::complex<float>*, unsigned long, unsigned long) const+0x1ff>
  47cde6:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  47cded:	00 00 00 


0000000000499800 <void micron::math::fft::__impl::__engine<double>::__butterflies<8ul, false>(micron::math::complex<double>*, unsigned long, unsigned long, unsigned long, unsigned long) const>:
  499800:	4d 39 c8             	cmp    %r9,%r8
  499803:	0f 83 6f 05 00 00    	jae    499d78 <void micron::math::fft::__impl::__engine<double>::__butterflies<8ul, false>(micron::math::complex<double>*, unsigned long, unsigned long, unsigned long, unsigned long) const+0x578>
  499809:	55                   	push   %rbp
  49980a:	49 89 fa             	mov    %rdi,%r10
  49980d:	48 89 c8             	mov    %rcx,%rax
  499810:	b9 03 00 00 00       	mov    $0x3,%ecx
  499815:	48 89 d7             	mov    %rdx,%rdi
  499818:	49 89 c3             	mov    %rax,%r11
  49981b:	49 c1 e3 04          	shl    $0x4,%r11
  49981f:	48 89 e5             	mov    %rsp,%rbp
  499822:	41 57                	push   %r15
  499824:	4c 6b f8 70          	imul   $0x70,%rax,%r15
  499828:	41 56                	push   %r14
  49982a:	41 55                	push   %r13
  49982c:	49 89 f5             	mov    %rsi,%r13
  49982f:	48 8d 34 40          	lea    (%rax,%rax,2),%rsi
  499833:	41 54                	push   %r12
  499835:	4d 89 c4             	mov    %r8,%r12
  499838:	53                   	push   %rbx
  499839:	48 83 e4 e0          	and    $0xffffffffffffffe0,%rsp
  49983d:	48 81 ec e0 01 00 00 	sub    $0x1e0,%rsp
  499844:	4d 8b 42 10          	mov    0x10(%r10),%r8
  499848:	c4 c2 f3 f7 5a 08    	shrx   %rcx,0x8(%r10),%rbx
  49984e:	48 c1 e3 04          	shl    $0x4,%rbx
  499852:	4c 89 4c 24 50       	mov    %r9,0x50(%rsp)
  499857:	49 89 f1             	mov    %rsi,%r9
  49985a:	48 c1 e6 05          	shl    $0x5,%rsi
  49985e:	49 89 c2             	mov    %rax,%r10
  499861:	49 8d 14 18          	lea    (%r8,%rbx,1),%rdx
  499865:	48 8d 0c 80          	lea    (%rax,%rax,4),%rcx
  499869:	49 c1 e2 05          	shl    $0x5,%r10
  49986d:	49 c1 e1 04          	shl    $0x4,%r9
  499871:	48 89 94 24 88 00 00 	mov    %rdx,0x88(%rsp)
  499878:	00 
  499879:	48 01 da             	add    %rbx,%rdx
  49987c:	48 c1 e1 04          	shl    $0x4,%rcx
  499880:	48 01 d3             	add    %rdx,%rbx
  499883:	48 89 94 24 90 00 00 	mov    %rdx,0x90(%rsp)
  49988a:	00 
  49988b:	48 89 c2             	mov    %rax,%rdx
  49988e:	4c 89 e0             	mov    %r12,%rax
  499891:	48 89 74 24 58       	mov    %rsi,0x58(%rsp)
  499896:	49 0f af f4          	imul   %r12,%rsi
  49989a:	48 c1 e2 06          	shl    $0x6,%rdx
  49989e:	48 c1 e0 04          	shl    $0x4,%rax
  4998a2:	4c 89 bc 24 a0 00 00 	mov    %r15,0xa0(%rsp)
  4998a9:	00 
  4998aa:	4d 0f af fc          	imul   %r12,%r15
  4998ae:	4c 01 e8             	add    %r13,%rax
  4998b1:	48 89 54 24 78       	mov    %rdx,0x78(%rsp)
  4998b6:	49 0f af d4          	imul   %r12,%rdx
  4998ba:	4c 01 c6             	add    %r8,%rsi
  4998bd:	4c 89 5c 24 70       	mov    %r11,0x70(%rsp)
  4998c2:	4d 0f af dc          	imul   %r12,%r11
  4998c6:	4d 01 c7             	add    %r8,%r15
  4998c9:	4c 89 94 24 98 00 00 	mov    %r10,0x98(%rsp)
  4998d0:	00 
  4998d1:	4d 0f af d4          	imul   %r12,%r10
  4998d5:	4c 89 4c 24 68       	mov    %r9,0x68(%rsp)
  4998da:	4d 0f af cc          	imul   %r12,%r9
  4998de:	4e 8d 34 02          	lea    (%rdx,%r8,1),%r14
  4998e2:	48 89 4c 24 60       	mov    %rcx,0x60(%rsp)
  4998e7:	49 0f af cc          	imul   %r12,%rcx
  4998eb:	4d 01 c3             	add    %r8,%r11
  4998ee:	48 89 b4 24 b8 00 00 	mov    %rsi,0xb8(%rsp)
  4998f5:	00 
  4998f6:	49 8d 34 7c          	lea    (%r12,%rdi,2),%rsi
  4998fa:	4d 01 c2             	add    %r8,%r10
  4998fd:	48 89 9c 24 80 00 00 	mov    %rbx,0x80(%rsp)
  499904:	00 
  499905:	48 89 f3             	mov    %rsi,%rbx
  499908:	4d 01 c1             	add    %r8,%r9
  49990b:	4c 89 bc 24 b0 00 00 	mov    %r15,0xb0(%rsp)
  499912:	00 
  499913:	4c 8d 3c 3e          	lea    (%rsi,%rdi,1),%r15
  499917:	48 8d 34 7e          	lea    (%rsi,%rdi,2),%rsi
  49991b:	48 c1 e3 04          	shl    $0x4,%rbx
  49991f:	48 89 84 24 c0 00 00 	mov    %rax,0xc0(%rsp)
  499926:	00 
  499927:	49 8d 04 3c          	lea    (%r12,%rdi,1),%rax
  49992b:	49 c1 e7 04          	shl    $0x4,%r15
  49992f:	4c 01 c1             	add    %r8,%rcx
  499932:	4c 89 b4 24 a8 00 00 	mov    %r14,0xa8(%rsp)
  499939:	00 
  49993a:	48 c1 e0 04          	shl    $0x4,%rax
  49993e:	49 89 f6             	mov    %rsi,%r14
  499941:	4d 01 ef             	add    %r13,%r15
  499944:	4c 01 e8             	add    %r13,%rax
  499947:	49 c1 e6 04          	shl    $0x4,%r14
  49994b:	49 8d 54 1d 00       	lea    0x0(%r13,%rbx,1),%rdx
  499950:	4b 8d 5c 35 00       	lea    0x0(%r13,%r14,1),%rbx
  499955:	4c 8d 34 3e          	lea    (%rsi,%rdi,1),%r14
  499959:	48 89 94 24 c8 00 00 	mov    %rdx,0xc8(%rsp)
  499960:	00 
  499961:	48 8d 34 7e          	lea    (%rsi,%rdi,2),%rsi
  499965:	48 89 9c 24 d0 00 00 	mov    %rbx,0xd0(%rsp)
  49996c:	00 
  49996d:	49 c1 e6 04          	shl    $0x4,%r14
  499971:	48 89 f2             	mov    %rsi,%rdx
  499974:	48 01 f7             	add    %rsi,%rdi
  499977:	4d 01 ee             	add    %r13,%r14
  49997a:	48 c1 e2 04          	shl    $0x4,%rdx
  49997e:	48 c1 e7 04          	shl    $0x4,%rdi
  499982:	49 8d 5c 15 00       	lea    0x0(%r13,%rdx,1),%rbx
  499987:	49 01 fd             	add    %rdi,%r13
  49998a:	48 89 9c 24 d8 00 00 	mov    %rbx,0xd8(%rsp)
  499991:	00 
  499992:	31 db                	xor    %ebx,%ebx
  499994:	90                   	nop
  499995:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
  49999c:	00 00 00 00 
  4999a0:	48 8b bc 24 c0 00 00 	mov    0xc0(%rsp),%rdi
  4999a7:	00 
  4999a8:	48 8b b4 24 c8 00 00 	mov    0xc8(%rsp),%rsi
  4999af:	00 
  4999b0:	48 8b 94 24 d0 00 00 	mov    0xd0(%rsp),%rdx
  4999b7:	00 
  4999b8:	c5 7b 10 0c 18       	vmovsd (%rax,%rbx,1),%xmm9
  4999bd:	c5 79 10 14 1f       	vmovupd (%rdi,%rbx,1),%xmm10
  4999c2:	48 8b bc 24 d8 00 00 	mov    0xd8(%rsp),%rdi
  4999c9:	00 
  4999ca:	c5 fb 10 7c 18 08    	vmovsd 0x8(%rax,%rbx,1),%xmm7
  4999d0:	c5 79 10 1c 1e       	vmovupd (%rsi,%rbx,1),%xmm11
  4999d5:	c4 41 7b 10 04 1f    	vmovsd (%r15,%rbx,1),%xmm8
  4999db:	c4 c1 7b 10 74 1f 08 	vmovsd 0x8(%r15,%rbx,1),%xmm6
  4999e2:	c5 f9 10 0c 1a       	vmovupd (%rdx,%rbx,1),%xmm1
  4999e7:	c4 c1 7b 10 14 1e    	vmovsd (%r14,%rbx,1),%xmm2
  4999ed:	c4 c1 7b 10 44 1e 08 	vmovsd 0x8(%r14,%rbx,1),%xmm0
  4999f4:	c5 f9 10 2c 1f       	vmovupd (%rdi,%rbx,1),%xmm5
  4999f9:	c4 c1 7b 10 64 1d 00 	vmovsd 0x0(%r13,%rbx,1),%xmm4
  499a00:	c4 c1 7b 10 5c 1d 08 	vmovsd 0x8(%r13,%rbx,1),%xmm3
  499a07:	4d 85 e4             	test   %r12,%r12
  499a0a:	0f 84 93 01 00 00    	je     499ba3 <void micron::math::fft::__impl::__engine<double>::__butterflies<8ul, false>(micron::math::complex<double>*, unsigned long, unsigned long, unsigned long, unsigned long) const+0x3a3>
  499a10:	c5 79 10 39          	vmovupd (%rcx),%xmm15
  499a14:	48 89 4c 24 28       	mov    %rcx,0x28(%rsp)
  499a19:	c5 e9 14 c0          	vunpcklpd %xmm0,%xmm2,%xmm0
  499a1d:	c5 d9 14 db          	vunpcklpd %xmm3,%xmm4,%xmm3
  499a21:	c4 41 79 10 21       	vmovupd (%r9),%xmm12
  499a26:	c4 41 79 10 32       	vmovupd (%r10),%xmm14
  499a2b:	4c 89 4c 24 18       	mov    %r9,0x18(%rsp)
  499a30:	c5 b1 14 ff          	vunpcklpd %xmm7,%xmm9,%xmm7
  499a34:	4c 89 54 24 20       	mov    %r10,0x20(%rsp)
  499a39:	48 8b 8c 24 a8 00 00 	mov    0xa8(%rsp),%rcx
  499a40:	00 
  499a41:	c5 b9 14 f6          	vunpcklpd %xmm6,%xmm8,%xmm6
  499a45:	c4 63 2d 18 cf 01    	vinsertf128 $0x1,%xmm7,%ymm10,%ymm9
  499a4b:	c4 43 0d 18 f4 01    	vinsertf128 $0x1,%xmm12,%ymm14,%ymm14
  499a51:	48 89 44 24 10       	mov    %rax,0x10(%rsp)
  499a56:	c4 63 25 18 c6 01    	vinsertf128 $0x1,%xmm6,%ymm11,%ymm8
  499a5c:	4c 8b 94 24 b0 00 00 	mov    0xb0(%rsp),%r10
  499a63:	00 
  499a64:	c5 79 10 29          	vmovupd (%rcx),%xmm13
  499a68:	4c 89 5c 24 30       	mov    %r11,0x30(%rsp)
  499a6d:	ba 08 00 00 00       	mov    $0x8,%edx
  499a72:	c4 e3 55 18 d3 01    	vinsertf128 $0x1,%xmm3,%ymm5,%ymm2
  499a78:	4c 8b 8c 24 b8 00 00 	mov    0xb8(%rsp),%r9
  499a7f:	00 
  499a80:	4c 89 44 24 38       	mov    %r8,0x38(%rsp)
  499a85:	c4 e3 75 18 c8 01    	vinsertf128 $0x1,%xmm0,%ymm1,%ymm1
  499a8b:	48 8d bc 24 e0 00 00 	lea    0xe0(%rsp),%rdi
  499a92:	00 
  499a93:	c4 43 15 18 ef 01    	vinsertf128 $0x1,%xmm15,%ymm13,%ymm13
  499a99:	c4 41 79 10 3a       	vmovupd (%r10),%xmm15
  499a9e:	48 8d b4 24 60 01 00 	lea    0x160(%rsp),%rsi
  499aa5:	00 
  499aa6:	c4 41 79 10 21       	vmovupd (%r9),%xmm12
  499aab:	c4 43 1d 18 e7 01    	vinsertf128 $0x1,%xmm15,%ymm12,%ymm12
  499ab1:	c4 41 79 10 3b       	vmovupd (%r11),%xmm15
  499ab6:	c5 79 29 7c 24 40    	vmovapd %xmm15,0x40(%rsp)
  499abc:	c4 41 79 10 38       	vmovupd (%r8),%xmm15
  499ac1:	c4 63 05 18 7c 24 40 	vinsertf128 $0x1,0x40(%rsp),%ymm15,%ymm15
  499ac8:	01 
  499ac9:	c5 7d 29 8c 24 e0 00 	vmovapd %ymm9,0xe0(%rsp)
  499ad0:	00 00 
  499ad2:	c5 7d 29 84 24 00 01 	vmovapd %ymm8,0x100(%rsp)
  499ad9:	00 00 
  499adb:	c5 fd 29 8c 24 20 01 	vmovapd %ymm1,0x120(%rsp)
  499ae2:	00 00 
  499ae4:	c5 fd 29 94 24 40 01 	vmovapd %ymm2,0x140(%rsp)
  499aeb:	00 00 
  499aed:	c5 7d 29 bc 24 60 01 	vmovapd %ymm15,0x160(%rsp)
  499af4:	00 00 
  499af6:	c5 7d 29 b4 24 80 01 	vmovapd %ymm14,0x180(%rsp)
  499afd:	00 00 
  499aff:	c5 7d 29 ac 24 a0 01 	vmovapd %ymm13,0x1a0(%rsp)
  499b06:	00 00 
  499b08:	c5 7d 29 a4 24 c0 01 	vmovapd %ymm12,0x1c0(%rsp)
  499b0f:	00 00 
  499b11:	c5 f8 77             	vzeroupper
  499b14:	e8 d7 60 fd ff       	call   46fbf0 <void micron::math::fft::__impl::__multiply<double>(micron::math::complex<double>*, micron::math::complex<double> const*, unsigned long)>
  499b19:	48 8b 44 24 10       	mov    0x10(%rsp),%rax
  499b1e:	c5 79 28 94 24 e0 00 	vmovapd 0xe0(%rsp),%xmm10
  499b25:	00 00 
  499b27:	c5 7b 10 8c 24 f0 00 	vmovsd 0xf0(%rsp),%xmm9
  499b2e:	00 00 
  499b30:	4c 8b 4c 24 18       	mov    0x18(%rsp),%r9
  499b35:	c5 fb 10 bc 24 f8 00 	vmovsd 0xf8(%rsp),%xmm7
  499b3c:	00 00 
  499b3e:	4c 8b 54 24 20       	mov    0x20(%rsp),%r10
  499b43:	c5 79 28 9c 24 00 01 	vmovapd 0x100(%rsp),%xmm11
  499b4a:	00 00 
  499b4c:	48 8b 4c 24 28       	mov    0x28(%rsp),%rcx
  499b51:	c5 7b 10 84 24 10 01 	vmovsd 0x110(%rsp),%xmm8
  499b58:	00 00 
  499b5a:	4c 8b 5c 24 30       	mov    0x30(%rsp),%r11
  499b5f:	c5 fb 10 b4 24 18 01 	vmovsd 0x118(%rsp),%xmm6
  499b66:	00 00 
  499b68:	4c 8b 44 24 38       	mov    0x38(%rsp),%r8
  499b6d:	c5 f9 28 8c 24 20 01 	vmovapd 0x120(%rsp),%xmm1
  499b74:	00 00 
  499b76:	c5 fb 10 94 24 30 01 	vmovsd 0x130(%rsp),%xmm2
  499b7d:	00 00 
  499b7f:	c5 fb 10 84 24 38 01 	vmovsd 0x138(%rsp),%xmm0
  499b86:	00 00 
  499b88:	c5 f9 28 ac 24 40 01 	vmovapd 0x140(%rsp),%xmm5
  499b8f:	00 00 
  499b91:	c5 fb 10 a4 24 50 01 	vmovsd 0x150(%rsp),%xmm4
  499b98:	00 00 
  499b9a:	c5 fb 10 9c 24 58 01 	vmovsd 0x158(%rsp),%xmm3
  499ba1:	00 00 
  499ba3:	c5 21 5c f5          	vsubpd %xmm5,%xmm11,%xmm14
  499ba7:	c5 29 5c e1          	vsubpd %xmm1,%xmm10,%xmm12
  499bab:	48 8b b4 24 88 00 00 	mov    0x88(%rsp),%rsi
  499bb2:	00 
  499bb3:	48 8b 94 24 90 00 00 	mov    0x90(%rsp),%rdx
  499bba:	00 
  499bbb:	c4 41 51 58 db       	vaddpd %xmm11,%xmm5,%xmm11
  499bc0:	c4 c1 71 58 ca       	vaddpd %xmm10,%xmm1,%xmm1
  499bc5:	48 8b bc 24 80 00 00 	mov    0x80(%rsp),%rdi
  499bcc:	00 
  499bcd:	49 83 c4 01          	add    $0x1,%r12
  499bd1:	c5 b3 58 ea          	vaddsd %xmm2,%xmm9,%xmm5
  499bd5:	c5 b3 5c d2          	vsubsd %xmm2,%xmm9,%xmm2
  499bd9:	4c 03 5c 24 70       	add    0x70(%rsp),%r11
  499bde:	4c 03 94 24 98 00 00 	add    0x98(%rsp),%r10
  499be5:	00 
  499be6:	c4 43 79 05 f6 01    	vpermilpd $0x1,%xmm14,%xmm14
  499bec:	c5 4b 58 cb          	vaddsd %xmm3,%xmm6,%xmm9
  499bf0:	c5 cb 5c db          	vsubsd %xmm3,%xmm6,%xmm3
  499bf4:	4c 03 4c 24 68       	add    0x68(%rsp),%r9
  499bf9:	c4 41 09 58 fc       	vaddpd %xmm12,%xmm14,%xmm15
  499bfe:	c5 21 58 d1          	vaddpd %xmm1,%xmm11,%xmm10
  499c02:	48 03 4c 24 60       	add    0x60(%rsp),%rcx
  499c07:	c4 41 19 5c ee       	vsubpd %xmm14,%xmm12,%xmm13
  499c0c:	c4 c1 71 5c cb       	vsubpd %xmm11,%xmm1,%xmm1
  499c11:	c4 41 19 d0 e6       	vaddsubpd %xmm14,%xmm12,%xmm12
  499c16:	c5 43 58 d8          	vaddsd %xmm0,%xmm7,%xmm11
  499c1a:	c5 3b 58 f4          	vaddsd %xmm4,%xmm8,%xmm14
  499c1e:	c5 bb 5c e4          	vsubsd %xmm4,%xmm8,%xmm4
  499c22:	c4 41 13 10 ef       	vmovsd %xmm15,%xmm13,%xmm13
  499c27:	c5 c3 5c c0          	vsubsd %xmm0,%xmm7,%xmm0
  499c2b:	c5 eb 58 fb          	vaddsd %xmm3,%xmm2,%xmm7
  499c2f:	c4 41 23 58 f9       	vaddsd %xmm9,%xmm11,%xmm15
  499c34:	c5 eb 5c d3          	vsubsd %xmm3,%xmm2,%xmm2
  499c38:	c4 41 53 58 c6       	vaddsd %xmm14,%xmm5,%xmm8
  499c3d:	c4 41 23 5c d9       	vsubsd %xmm9,%xmm11,%xmm11
  499c42:	c4 41 79 10 08       	vmovupd (%r8),%xmm9
  499c47:	c4 c1 53 5c ee       	vsubsd %xmm14,%xmm5,%xmm5
  499c4c:	c5 fb 12 f7          	vmovddup %xmm7,%xmm6
  499c50:	c5 fb 5c fc          	vsubsd %xmm4,%xmm0,%xmm7
  499c54:	c4 41 7b 12 ff       	vmovddup %xmm15,%xmm15
  499c59:	c5 fb 58 c4          	vaddsd %xmm4,%xmm0,%xmm0
  499c5d:	c4 c1 7b 12 d8       	vmovddup %xmm8,%xmm3
  499c62:	c4 43 79 05 c1 01    	vpermilpd $0x1,%xmm9,%xmm8
  499c68:	c5 7b 12 f5          	vmovddup %xmm5,%xmm14
  499c6c:	c4 c1 7b 12 eb       	vmovddup %xmm11,%xmm5
  499c71:	c5 7b 12 da          	vmovddup %xmm2,%xmm11
  499c75:	c4 c1 39 59 d7       	vmulpd %xmm15,%xmm8,%xmm2
  499c7a:	c5 fb 12 e0          	vmovddup %xmm0,%xmm4
  499c7e:	c5 fb 12 ff          	vmovddup %xmm7,%xmm7
  499c82:	c4 62 e9 96 cb       	vfmaddsub132pd %xmm3,%xmm2,%xmm9
  499c87:	c5 f9 10 17          	vmovupd (%rdi),%xmm2
  499c8b:	48 8b bc 24 d0 00 00 	mov    0xd0(%rsp),%rdi
  499c92:	00 
  499c93:	c4 41 31 58 c2       	vaddpd %xmm10,%xmm9,%xmm8
  499c98:	c4 41 29 5c d1       	vsubpd %xmm9,%xmm10,%xmm10
  499c9d:	c5 79 10 0e          	vmovupd (%rsi),%xmm9
  499ca1:	48 8b b4 24 c0 00 00 	mov    0xc0(%rsp),%rsi
  499ca8:	00 
  499ca9:	c4 c3 79 05 c1 01    	vpermilpd $0x1,%xmm9,%xmm0
  499caf:	c5 f9 59 ff          	vmulpd %xmm7,%xmm0,%xmm7
  499cb3:	c4 62 c1 96 ce       	vfmaddsub132pd %xmm6,%xmm7,%xmm9
  499cb8:	c5 f9 10 32          	vmovupd (%rdx),%xmm6
  499cbc:	c5 79 11 04 1e       	vmovupd %xmm8,(%rsi,%rbx,1)
  499cc1:	48 8b 94 24 c8 00 00 	mov    0xc8(%rsp),%rdx
  499cc8:	00 
  499cc9:	48 8b b4 24 d8 00 00 	mov    0xd8(%rsp),%rsi
  499cd0:	00 
  499cd1:	c4 e3 79 05 de 01    	vpermilpd $0x1,%xmm6,%xmm3
  499cd7:	c5 e1 59 ed          	vmulpd %xmm5,%xmm3,%xmm5
  499cdb:	c4 41 31 58 fd       	vaddpd %xmm13,%xmm9,%xmm15
  499ce0:	c4 41 11 5c e9       	vsubpd %xmm9,%xmm13,%xmm13
  499ce5:	c4 63 79 05 ca 01    	vpermilpd $0x1,%xmm2,%xmm9
  499ceb:	c5 b1 59 e4          	vmulpd %xmm4,%xmm9,%xmm4
  499cef:	c4 c2 d1 96 f6       	vfmaddsub132pd %xmm14,%xmm5,%xmm6
  499cf4:	c5 79 11 3c 18       	vmovupd %xmm15,(%rax,%rbx,1)
  499cf9:	c4 c2 d9 96 d3       	vfmaddsub132pd %xmm11,%xmm4,%xmm2
  499cfe:	c5 71 5c f6          	vsubpd %xmm6,%xmm1,%xmm14
  499d02:	c5 f1 58 ce          	vaddpd %xmm6,%xmm1,%xmm1
  499d06:	c5 19 58 da          	vaddpd %xmm2,%xmm12,%xmm11
  499d0a:	c5 19 5c e2          	vsubpd %xmm2,%xmm12,%xmm12
  499d0e:	c5 f9 11 0c 1a       	vmovupd %xmm1,(%rdx,%rbx,1)
  499d13:	48 8b 54 24 78       	mov    0x78(%rsp),%rdx
  499d18:	48 01 94 24 a8 00 00 	add    %rdx,0xa8(%rsp)
  499d1f:	00 
  499d20:	c4 41 79 11 1c 1f    	vmovupd %xmm11,(%r15,%rbx,1)
  499d26:	c5 79 11 14 1f       	vmovupd %xmm10,(%rdi,%rbx,1)
  499d2b:	48 8b 7c 24 58       	mov    0x58(%rsp),%rdi
  499d30:	c4 41 79 11 2c 1e    	vmovupd %xmm13,(%r14,%rbx,1)
  499d36:	48 01 bc 24 b8 00 00 	add    %rdi,0xb8(%rsp)
  499d3d:	00 
  499d3e:	c5 79 11 34 1e       	vmovupd %xmm14,(%rsi,%rbx,1)
  499d43:	48 8b b4 24 a0 00 00 	mov    0xa0(%rsp),%rsi
  499d4a:	00 
  499d4b:	c4 41 79 11 64 1d 00 	vmovupd %xmm12,0x0(%r13,%rbx,1)
  499d52:	48 83 c3 10          	add    $0x10,%rbx
  499d56:	48 01 b4 24 b0 00 00 	add    %rsi,0xb0(%rsp)
  499d5d:	00 
  499d5e:	4c 39 64 24 50       	cmp    %r12,0x50(%rsp)
  499d63:	0f 85 37 fc ff ff    	jne    4999a0 <void micron::math::fft::__impl::__engine<double>::__butterflies<8ul, false>(micron::math::complex<double>*, unsigned long, unsigned long, unsigned long, unsigned long) const+0x1a0>
  499d69:	48 8d 65 d8          	lea    -0x28(%rbp),%rsp
  499d6d:	5b                   	pop    %rbx
  499d6e:	41 5c                	pop    %r12
  499d70:	41 5d                	pop    %r13
  499d72:	41 5e                	pop    %r14
  499d74:	41 5f                	pop    %r15
  499d76:	5d                   	pop    %rbp
  499d77:	c3                   	ret
  499d78:	c3                   	ret
  499d79:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)


000000000049a530 <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const>:
  49a530:	48 3b 8f 80 00 00 00 	cmp    0x80(%rdi),%rcx
  49a537:	0f 84 5b 03 00 00    	je     49a898 <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x368>
  49a53d:	55                   	push   %rbp
  49a53e:	48 89 c8             	mov    %rcx,%rax
  49a541:	48 89 e5             	mov    %rsp,%rbp
  49a544:	48 83 ec 70          	sub    $0x70,%rsp
  49a548:	4c 89 65 e0          	mov    %r12,-0x20(%rbp)
  49a54c:	49 89 d4             	mov    %rdx,%r12
  49a54f:	48 8b 57 70          	mov    0x70(%rdi),%rdx
  49a553:	48 89 5d d8          	mov    %rbx,-0x28(%rbp)
  49a557:	4c 89 c3             	mov    %r8,%rbx
  49a55a:	4c 8b 0c ca          	mov    (%rdx,%rcx,8),%r9
  49a55e:	48 8b 4f 58          	mov    0x58(%rdi),%rcx
  49a562:	4c 89 7d f8          	mov    %r15,-0x8(%rbp)
  49a566:	49 89 ff             	mov    %rdi,%r15
  49a569:	4c 8b 5c c1 08       	mov    0x8(%rcx,%rax,8),%r11
  49a56e:	49 83 fb 01          	cmp    $0x1,%r11
  49a572:	0f 84 d8 01 00 00    	je     49a750 <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x220>
  49a578:	4d 85 c9             	test   %r9,%r9
  49a57b:	0f 84 a9 01 00 00    	je     49a72a <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x1fa>
  49a581:	4c 89 cf             	mov    %r9,%rdi
  49a584:	4c 89 6d e8          	mov    %r13,-0x18(%rbp)
  49a588:	4d 89 dd             	mov    %r11,%r13
  49a58b:	48 c1 e3 04          	shl    $0x4,%rbx
  49a58f:	49 0f af f8          	imul   %r8,%rdi
  49a593:	4d 89 c8             	mov    %r9,%r8
  49a596:	4c 89 75 f0          	mov    %r14,-0x10(%rbp)
  49a59a:	49 c1 e5 04          	shl    $0x4,%r13
  49a59e:	4c 8d 70 01          	lea    0x1(%rax),%r14
  49a5a2:	4c 89 e2             	mov    %r12,%rdx
  49a5a5:	45 31 d2             	xor    %r10d,%r10d
  49a5a8:	48 89 7d c8          	mov    %rdi,-0x38(%rbp)
  49a5ac:	41 83 e0 03          	and    $0x3,%r8d
  49a5b0:	0f 84 ab 00 00 00    	je     49a661 <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x131>
  49a5b6:	49 83 f8 01          	cmp    $0x1,%r8
  49a5ba:	74 53                	je     49a60f <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0xdf>
  49a5bc:	49 83 f8 02          	cmp    $0x2,%r8
  49a5c0:	0f 85 e2 02 00 00    	jne    49a8a8 <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x378>
  49a5c6:	4c 8b 45 c8          	mov    -0x38(%rbp),%r8
  49a5ca:	4c 89 f1             	mov    %r14,%rcx
  49a5cd:	4c 89 ff             	mov    %r15,%rdi
  49a5d0:	48 89 45 98          	mov    %rax,-0x68(%rbp)
  49a5d4:	4c 89 55 a0          	mov    %r10,-0x60(%rbp)
  49a5d8:	4c 89 5d a8          	mov    %r11,-0x58(%rbp)
  49a5dc:	4c 89 4d b0          	mov    %r9,-0x50(%rbp)
  49a5e0:	48 89 55 b8          	mov    %rdx,-0x48(%rbp)
  49a5e4:	48 89 75 c0          	mov    %rsi,-0x40(%rbp)
  49a5e8:	e8 43 ff ff ff       	call   49a530 <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const>
  49a5ed:	4c 8b 55 a0          	mov    -0x60(%rbp),%r10
  49a5f1:	48 8b 75 c0          	mov    -0x40(%rbp),%rsi
  49a5f5:	48 8b 55 b8          	mov    -0x48(%rbp),%rdx
  49a5f9:	48 8b 45 98          	mov    -0x68(%rbp),%rax
  49a5fd:	4c 8b 5d a8          	mov    -0x58(%rbp),%r11
  49a601:	4c 8b 4d b0          	mov    -0x50(%rbp),%r9
  49a605:	49 83 c2 01          	add    $0x1,%r10
  49a609:	48 01 de             	add    %rbx,%rsi
  49a60c:	4c 01 ea             	add    %r13,%rdx
  49a60f:	4c 8b 45 c8          	mov    -0x38(%rbp),%r8
  49a613:	4c 89 f1             	mov    %r14,%rcx
  49a616:	4c 89 ff             	mov    %r15,%rdi
  49a619:	48 89 45 98          	mov    %rax,-0x68(%rbp)
  49a61d:	4c 89 55 a0          	mov    %r10,-0x60(%rbp)
  49a621:	4c 89 5d a8          	mov    %r11,-0x58(%rbp)
  49a625:	4c 89 4d b0          	mov    %r9,-0x50(%rbp)
  49a629:	48 89 55 b8          	mov    %rdx,-0x48(%rbp)
  49a62d:	48 89 75 c0          	mov    %rsi,-0x40(%rbp)
  49a631:	e8 fa fe ff ff       	call   49a530 <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const>
  49a636:	4c 8b 55 a0          	mov    -0x60(%rbp),%r10
  49a63a:	48 8b 75 c0          	mov    -0x40(%rbp),%rsi
  49a63e:	48 8b 55 b8          	mov    -0x48(%rbp),%rdx
  49a642:	4c 8b 4d b0          	mov    -0x50(%rbp),%r9
  49a646:	49 83 c2 01          	add    $0x1,%r10
  49a64a:	48 01 de             	add    %rbx,%rsi
  49a64d:	4c 8b 5d a8          	mov    -0x58(%rbp),%r11
  49a651:	48 8b 45 98          	mov    -0x68(%rbp),%rax
  49a655:	4c 01 ea             	add    %r13,%rdx
  49a658:	4d 39 d1             	cmp    %r10,%r9
  49a65b:	0f 84 c1 00 00 00    	je     49a722 <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x1f2>
  49a661:	4c 89 4d b0          	mov    %r9,-0x50(%rbp)
  49a665:	4c 89 5d a8          	mov    %r11,-0x58(%rbp)
  49a669:	48 89 45 98          	mov    %rax,-0x68(%rbp)
  49a66d:	4c 89 65 a0          	mov    %r12,-0x60(%rbp)
  49a671:	4d 89 d4             	mov    %r10,%r12
  49a674:	4c 8b 45 c8          	mov    -0x38(%rbp),%r8
  49a678:	4c 89 f1             	mov    %r14,%rcx
  49a67b:	4c 89 ff             	mov    %r15,%rdi
  49a67e:	48 89 55 b8          	mov    %rdx,-0x48(%rbp)
  49a682:	48 89 75 c0          	mov    %rsi,-0x40(%rbp)
  49a686:	49 83 c4 04          	add    $0x4,%r12
  49a68a:	e8 a1 fe ff ff       	call   49a530 <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const>
  49a68f:	48 8b 75 c0          	mov    -0x40(%rbp),%rsi
  49a693:	48 8b 55 b8          	mov    -0x48(%rbp),%rdx
  49a697:	4c 89 f1             	mov    %r14,%rcx
  49a69a:	4c 8b 45 c8          	mov    -0x38(%rbp),%r8
  49a69e:	4c 89 ff             	mov    %r15,%rdi
  49a6a1:	48 01 de             	add    %rbx,%rsi
  49a6a4:	4c 01 ea             	add    %r13,%rdx
  49a6a7:	48 89 55 b8          	mov    %rdx,-0x48(%rbp)
  49a6ab:	48 89 75 c0          	mov    %rsi,-0x40(%rbp)
  49a6af:	e8 7c fe ff ff       	call   49a530 <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const>
  49a6b4:	48 8b 75 c0          	mov    -0x40(%rbp),%rsi
  49a6b8:	48 8b 55 b8          	mov    -0x48(%rbp),%rdx
  49a6bc:	4c 89 f1             	mov    %r14,%rcx
  49a6bf:	4c 8b 45 c8          	mov    -0x38(%rbp),%r8
  49a6c3:	4c 89 ff             	mov    %r15,%rdi
  49a6c6:	48 01 de             	add    %rbx,%rsi
  49a6c9:	4c 01 ea             	add    %r13,%rdx
  49a6cc:	48 89 55 b8          	mov    %rdx,-0x48(%rbp)
  49a6d0:	48 89 75 c0          	mov    %rsi,-0x40(%rbp)
  49a6d4:	e8 57 fe ff ff       	call   49a530 <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const>
  49a6d9:	48 8b 75 c0          	mov    -0x40(%rbp),%rsi
  49a6dd:	48 8b 55 b8          	mov    -0x48(%rbp),%rdx
  49a6e1:	4c 89 f1             	mov    %r14,%rcx
  49a6e4:	4c 8b 45 c8          	mov    -0x38(%rbp),%r8
  49a6e8:	4c 89 ff             	mov    %r15,%rdi
  49a6eb:	48 01 de             	add    %rbx,%rsi
  49a6ee:	4c 01 ea             	add    %r13,%rdx
  49a6f1:	48 89 55 b8          	mov    %rdx,-0x48(%rbp)
  49a6f5:	48 89 75 c0          	mov    %rsi,-0x40(%rbp)
  49a6f9:	e8 32 fe ff ff       	call   49a530 <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const>
  49a6fe:	48 8b 75 c0          	mov    -0x40(%rbp),%rsi
  49a702:	48 8b 55 b8          	mov    -0x48(%rbp),%rdx
  49a706:	48 01 de             	add    %rbx,%rsi
  49a709:	4c 01 ea             	add    %r13,%rdx
  49a70c:	4c 39 65 b0          	cmp    %r12,-0x50(%rbp)
  49a710:	0f 85 5e ff ff ff    	jne    49a674 <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x144>
  49a716:	4c 8b 5d a8          	mov    -0x58(%rbp),%r11
  49a71a:	4c 8b 65 a0          	mov    -0x60(%rbp),%r12
  49a71e:	48 8b 45 98          	mov    -0x68(%rbp),%rax
  49a722:	4c 8b 6d e8          	mov    -0x18(%rbp),%r13
  49a726:	4c 8b 75 f0          	mov    -0x10(%rbp),%r14
  49a72a:	48 8b 5d d8          	mov    -0x28(%rbp),%rbx
  49a72e:	4c 89 e6             	mov    %r12,%rsi
  49a731:	4c 89 ff             	mov    %r15,%rdi
  49a734:	4c 8b 65 e0          	mov    -0x20(%rbp),%r12
  49a738:	4c 8b 7d f8          	mov    -0x8(%rbp),%r15
  49a73c:	4d 89 d8             	mov    %r11,%r8
  49a73f:	31 c9                	xor    %ecx,%ecx
  49a741:	48 89 c2             	mov    %rax,%rdx
  49a744:	c9                   	leave
  49a745:	e9 36 f6 ff ff       	jmp    499d80 <void micron::math::fft::__impl::__engine<double>::__combine_stage<false>(micron::math::complex<double>*, unsigned long, unsigned long, unsigned long) const>
  49a74a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  49a750:	4d 85 c9             	test   %r9,%r9
  49a753:	74 d5                	je     49a72a <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x1fa>
  49a755:	49 c1 e1 04          	shl    $0x4,%r9
  49a759:	45 31 c0             	xor    %r8d,%r8d
  49a75c:	48 83 fb 01          	cmp    $0x1,%rbx
  49a760:	0f 85 8a 01 00 00    	jne    49a8f0 <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x3c0>
  49a766:	49 8d 59 f0          	lea    -0x10(%r9),%rbx
  49a76a:	48 c1 eb 04          	shr    $0x4,%rbx
  49a76e:	48 83 c3 01          	add    $0x1,%rbx
  49a772:	83 e3 07             	and    $0x7,%ebx
  49a775:	0f 84 9d 00 00 00    	je     49a818 <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x2e8>
  49a77b:	48 83 fb 01          	cmp    $0x1,%rbx
  49a77f:	74 7e                	je     49a7ff <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x2cf>
  49a781:	48 83 fb 02          	cmp    $0x2,%rbx
  49a785:	74 68                	je     49a7ef <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x2bf>
  49a787:	48 83 fb 03          	cmp    $0x3,%rbx
  49a78b:	74 52                	je     49a7df <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x2af>
  49a78d:	48 83 fb 04          	cmp    $0x4,%rbx
  49a791:	74 3c                	je     49a7cf <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x29f>
  49a793:	48 83 fb 05          	cmp    $0x5,%rbx
  49a797:	74 26                	je     49a7bf <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x28f>
  49a799:	48 83 fb 06          	cmp    $0x6,%rbx
  49a79d:	74 10                	je     49a7af <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x27f>
  49a79f:	c5 7a 6f 3e          	vmovdqu (%rsi),%xmm15
  49a7a3:	41 b8 10 00 00 00    	mov    $0x10,%r8d
  49a7a9:	c4 41 7a 7f 3c 24    	vmovdqu %xmm15,(%r12)
  49a7af:	c4 a1 7a 6f 04 06    	vmovdqu (%rsi,%r8,1),%xmm0
  49a7b5:	c4 81 7a 7f 04 04    	vmovdqu %xmm0,(%r12,%r8,1)
  49a7bb:	49 83 c0 10          	add    $0x10,%r8
  49a7bf:	c4 a1 7a 6f 0c 06    	vmovdqu (%rsi,%r8,1),%xmm1
  49a7c5:	c4 81 7a 7f 0c 04    	vmovdqu %xmm1,(%r12,%r8,1)
  49a7cb:	49 83 c0 10          	add    $0x10,%r8
  49a7cf:	c4 a1 7a 6f 14 06    	vmovdqu (%rsi,%r8,1),%xmm2
  49a7d5:	c4 81 7a 7f 14 04    	vmovdqu %xmm2,(%r12,%r8,1)
  49a7db:	49 83 c0 10          	add    $0x10,%r8
  49a7df:	c4 a1 7a 6f 1c 06    	vmovdqu (%rsi,%r8,1),%xmm3
  49a7e5:	c4 81 7a 7f 1c 04    	vmovdqu %xmm3,(%r12,%r8,1)
  49a7eb:	49 83 c0 10          	add    $0x10,%r8
  49a7ef:	c4 a1 7a 6f 24 06    	vmovdqu (%rsi,%r8,1),%xmm4
  49a7f5:	c4 81 7a 7f 24 04    	vmovdqu %xmm4,(%r12,%r8,1)
  49a7fb:	49 83 c0 10          	add    $0x10,%r8
  49a7ff:	c4 a1 7a 6f 2c 06    	vmovdqu (%rsi,%r8,1),%xmm5
  49a805:	c4 81 7a 7f 2c 04    	vmovdqu %xmm5,(%r12,%r8,1)
  49a80b:	49 83 c0 10          	add    $0x10,%r8
  49a80f:	4d 39 c8             	cmp    %r9,%r8
  49a812:	0f 84 12 ff ff ff    	je     49a72a <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x1fa>
  49a818:	c4 a1 7a 6f 34 06    	vmovdqu (%rsi,%r8,1),%xmm6
  49a81e:	c4 81 7a 7f 34 04    	vmovdqu %xmm6,(%r12,%r8,1)
  49a824:	c4 a1 7a 6f 7c 06 10 	vmovdqu 0x10(%rsi,%r8,1),%xmm7
  49a82b:	c4 81 7a 7f 7c 04 10 	vmovdqu %xmm7,0x10(%r12,%r8,1)
  49a832:	c4 21 7a 6f 44 06 20 	vmovdqu 0x20(%rsi,%r8,1),%xmm8
  49a839:	c4 01 7a 7f 44 04 20 	vmovdqu %xmm8,0x20(%r12,%r8,1)
  49a840:	c4 21 7a 6f 4c 06 30 	vmovdqu 0x30(%rsi,%r8,1),%xmm9
  49a847:	c4 01 7a 7f 4c 04 30 	vmovdqu %xmm9,0x30(%r12,%r8,1)
  49a84e:	c4 21 7a 6f 54 06 40 	vmovdqu 0x40(%rsi,%r8,1),%xmm10
  49a855:	c4 01 7a 7f 54 04 40 	vmovdqu %xmm10,0x40(%r12,%r8,1)
  49a85c:	c4 21 7a 6f 5c 06 50 	vmovdqu 0x50(%rsi,%r8,1),%xmm11
  49a863:	c4 01 7a 7f 5c 04 50 	vmovdqu %xmm11,0x50(%r12,%r8,1)
  49a86a:	c4 21 7a 6f 64 06 60 	vmovdqu 0x60(%rsi,%r8,1),%xmm12
  49a871:	c4 01 7a 7f 64 04 60 	vmovdqu %xmm12,0x60(%r12,%r8,1)
  49a878:	c4 21 7a 6f 6c 06 70 	vmovdqu 0x70(%rsi,%r8,1),%xmm13
  49a87f:	c4 01 7a 7f 6c 04 70 	vmovdqu %xmm13,0x70(%r12,%r8,1)
  49a886:	49 83 e8 80          	sub    $0xffffffffffffff80,%r8
  49a88a:	4d 39 c8             	cmp    %r9,%r8
  49a88d:	75 89                	jne    49a818 <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x2e8>
  49a88f:	e9 96 fe ff ff       	jmp    49a72a <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x1fa>
  49a894:	0f 1f 40 00          	nopl   0x0(%rax)
  49a898:	c5 7a 6f 36          	vmovdqu (%rsi),%xmm14
  49a89c:	c5 7a 7f 32          	vmovdqu %xmm14,(%rdx)
  49a8a0:	c3                   	ret
  49a8a1:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  49a8a8:	49 89 f8             	mov    %rdi,%r8
  49a8ab:	4c 89 f1             	mov    %r14,%rcx
  49a8ae:	4c 89 ff             	mov    %r15,%rdi
  49a8b1:	48 89 45 a8          	mov    %rax,-0x58(%rbp)
  49a8b5:	4c 89 5d b0          	mov    %r11,-0x50(%rbp)
  49a8b9:	4c 89 4d b8          	mov    %r9,-0x48(%rbp)
  49a8bd:	48 89 75 c0          	mov    %rsi,-0x40(%rbp)
  49a8c1:	e8 6a fc ff ff       	call   49a530 <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const>
  49a8c6:	48 8b 75 c0          	mov    -0x40(%rbp),%rsi
  49a8ca:	48 8b 45 a8          	mov    -0x58(%rbp),%rax
  49a8ce:	41 ba 01 00 00 00    	mov    $0x1,%r10d
  49a8d4:	4c 8b 5d b0          	mov    -0x50(%rbp),%r11
  49a8d8:	4c 8b 4d b8          	mov    -0x48(%rbp),%r9
  49a8dc:	4b 8d 14 2c          	lea    (%r12,%r13,1),%rdx
  49a8e0:	48 01 de             	add    %rbx,%rsi
  49a8e3:	e9 de fc ff ff       	jmp    49a5c6 <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x96>
  49a8e8:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
  49a8ef:	00 
  49a8f0:	4b 8d 3c 0c          	lea    (%r12,%r9,1),%rdi
  49a8f4:	49 83 e9 10          	sub    $0x10,%r9
  49a8f8:	48 c1 e3 04          	shl    $0x4,%rbx
  49a8fc:	48 89 f2             	mov    %rsi,%rdx
  49a8ff:	49 c1 e9 04          	shr    $0x4,%r9
  49a903:	4c 89 e1             	mov    %r12,%rcx
  49a906:	49 83 c1 01          	add    $0x1,%r9
  49a90a:	41 83 e1 07          	and    $0x7,%r9d
  49a90e:	0f 84 a4 00 00 00    	je     49a9b8 <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x488>
  49a914:	49 83 f9 01          	cmp    $0x1,%r9
  49a918:	0f 84 81 00 00 00    	je     49a99f <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x46f>
  49a91e:	49 83 f9 02          	cmp    $0x2,%r9
  49a922:	74 6b                	je     49a98f <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x45f>
  49a924:	49 83 f9 03          	cmp    $0x3,%r9
  49a928:	74 55                	je     49a97f <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x44f>
  49a92a:	49 83 f9 04          	cmp    $0x4,%r9
  49a92e:	74 3f                	je     49a96f <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x43f>
  49a930:	49 83 f9 05          	cmp    $0x5,%r9
  49a934:	74 29                	je     49a95f <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x42f>
  49a936:	49 83 f9 06          	cmp    $0x6,%r9
  49a93a:	74 13                	je     49a94f <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x41f>
  49a93c:	c5 fa 6f 06          	vmovdqu (%rsi),%xmm0
  49a940:	48 8d 14 1e          	lea    (%rsi,%rbx,1),%rdx
  49a944:	49 8d 4c 24 10       	lea    0x10(%r12),%rcx
  49a949:	c4 c1 7a 7f 04 24    	vmovdqu %xmm0,(%r12)
  49a94f:	c5 fa 6f 0a          	vmovdqu (%rdx),%xmm1
  49a953:	48 83 c1 10          	add    $0x10,%rcx
  49a957:	48 01 da             	add    %rbx,%rdx
  49a95a:	c5 fa 7f 49 f0       	vmovdqu %xmm1,-0x10(%rcx)
  49a95f:	c5 fa 6f 12          	vmovdqu (%rdx),%xmm2
  49a963:	48 83 c1 10          	add    $0x10,%rcx
  49a967:	48 01 da             	add    %rbx,%rdx
  49a96a:	c5 fa 7f 51 f0       	vmovdqu %xmm2,-0x10(%rcx)
  49a96f:	c5 fa 6f 1a          	vmovdqu (%rdx),%xmm3
  49a973:	48 83 c1 10          	add    $0x10,%rcx
  49a977:	48 01 da             	add    %rbx,%rdx
  49a97a:	c5 fa 7f 59 f0       	vmovdqu %xmm3,-0x10(%rcx)
  49a97f:	c5 fa 6f 22          	vmovdqu (%rdx),%xmm4
  49a983:	48 83 c1 10          	add    $0x10,%rcx
  49a987:	48 01 da             	add    %rbx,%rdx
  49a98a:	c5 fa 7f 61 f0       	vmovdqu %xmm4,-0x10(%rcx)
  49a98f:	c5 fa 6f 2a          	vmovdqu (%rdx),%xmm5
  49a993:	48 83 c1 10          	add    $0x10,%rcx
  49a997:	48 01 da             	add    %rbx,%rdx
  49a99a:	c5 fa 7f 69 f0       	vmovdqu %xmm5,-0x10(%rcx)
  49a99f:	c5 fa 6f 32          	vmovdqu (%rdx),%xmm6
  49a9a3:	48 83 c1 10          	add    $0x10,%rcx
  49a9a7:	48 01 da             	add    %rbx,%rdx
  49a9aa:	c5 fa 7f 71 f0       	vmovdqu %xmm6,-0x10(%rcx)
  49a9af:	48 39 f9             	cmp    %rdi,%rcx
  49a9b2:	0f 84 72 fd ff ff    	je     49a72a <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x1fa>
  49a9b8:	c5 fa 6f 3a          	vmovdqu (%rdx),%xmm7
  49a9bc:	48 01 da             	add    %rbx,%rdx
  49a9bf:	48 83 e9 80          	sub    $0xffffffffffffff80,%rcx
  49a9c3:	c5 fa 7f 79 80       	vmovdqu %xmm7,-0x80(%rcx)
  49a9c8:	c5 7a 6f 02          	vmovdqu (%rdx),%xmm8
  49a9cc:	48 01 da             	add    %rbx,%rdx
  49a9cf:	c5 7a 7f 41 90       	vmovdqu %xmm8,-0x70(%rcx)
  49a9d4:	c5 7a 6f 0a          	vmovdqu (%rdx),%xmm9
  49a9d8:	48 01 da             	add    %rbx,%rdx
  49a9db:	c5 7a 7f 49 a0       	vmovdqu %xmm9,-0x60(%rcx)
  49a9e0:	c5 7a 6f 12          	vmovdqu (%rdx),%xmm10
  49a9e4:	48 01 da             	add    %rbx,%rdx
  49a9e7:	c5 7a 7f 51 b0       	vmovdqu %xmm10,-0x50(%rcx)
  49a9ec:	c5 7a 6f 1a          	vmovdqu (%rdx),%xmm11
  49a9f0:	48 01 da             	add    %rbx,%rdx
  49a9f3:	c5 7a 7f 59 c0       	vmovdqu %xmm11,-0x40(%rcx)
  49a9f8:	c5 7a 6f 22          	vmovdqu (%rdx),%xmm12
  49a9fc:	48 01 da             	add    %rbx,%rdx
  49a9ff:	c5 7a 7f 61 d0       	vmovdqu %xmm12,-0x30(%rcx)
  49aa04:	c5 7a 6f 2a          	vmovdqu (%rdx),%xmm13
  49aa08:	48 01 da             	add    %rbx,%rdx
  49aa0b:	c5 7a 7f 69 e0       	vmovdqu %xmm13,-0x20(%rcx)
  49aa10:	c5 7a 6f 32          	vmovdqu (%rdx),%xmm14
  49aa14:	48 01 da             	add    %rbx,%rdx
  49aa17:	c5 7a 7f 71 f0       	vmovdqu %xmm14,-0x10(%rcx)
  49aa1c:	48 39 f9             	cmp    %rdi,%rcx
  49aa1f:	75 97                	jne    49a9b8 <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x488>
  49aa21:	e9 04 fd ff ff       	jmp    49a72a <void micron::math::fft::__impl::__engine<double>::__smooth_stage<false>(micron::math::complex<double> const*, micron::math::complex<double>*, unsigned long, unsigned long) const+0x1fa>
  49aa26:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  49aa2d:	00 00 00 


00000000004b4240 <void micron::math::fft::__impl::__engine<long double>::__butterflies<8ul, false>(micron::math::complex<long double>*, unsigned long, unsigned long, unsigned long, unsigned long) const>:
  4b4240:	55                   	push   %rbp
  4b4241:	48 89 e5             	mov    %rsp,%rbp
  4b4244:	48 81 ec d8 01 00 00 	sub    $0x1d8,%rsp
  4b424b:	48 89 4d c0          	mov    %rcx,-0x40(%rbp)
  4b424f:	4c 89 8d 68 fe ff ff 	mov    %r9,-0x198(%rbp)
  4b4256:	4d 39 c8             	cmp    %r9,%r8
  4b4259:	0f 83 b4 07 00 00    	jae    4b4a13 <void micron::math::fft::__impl::__engine<long double>::__butterflies<8ul, false>(micron::math::complex<long double>*, unsigned long, unsigned long, unsigned long, unsigned long) const+0x7d3>
  4b425f:	48 8b 4f 08          	mov    0x8(%rdi),%rcx
  4b4263:	48 89 f0             	mov    %rsi,%rax
  4b4266:	48 8b 77 10          	mov    0x10(%rdi),%rsi
  4b426a:	48 89 5d d8          	mov    %rbx,-0x28(%rbp)
  4b426e:	4c 89 75 f0          	mov    %r14,-0x10(%rbp)
  4b4272:	4c 89 c3             	mov    %r8,%rbx
  4b4275:	49 c1 e0 05          	shl    $0x5,%r8
  4b4279:	48 c1 e9 03          	shr    $0x3,%rcx
  4b427d:	4c 89 6d e8          	mov    %r13,-0x18(%rbp)
  4b4281:	4c 8d 1c 13          	lea    (%rbx,%rdx,1),%r11
  4b4285:	49 01 c0             	add    %rax,%r8
  4b4288:	48 c1 e1 05          	shl    $0x5,%rcx
  4b428c:	4c 89 7d f8          	mov    %r15,-0x8(%rbp)
  4b4290:	4c 8d 3c 53          	lea    (%rbx,%rdx,2),%r15
  4b4294:	49 c1 e3 05          	shl    $0x5,%r11
  4b4298:	4c 8d 34 0e          	lea    (%rsi,%rcx,1),%r14
  4b429c:	4d 8d 0c 17          	lea    (%r15,%rdx,1),%r9
  4b42a0:	4d 89 fa             	mov    %r15,%r10
  4b42a3:	4c 89 65 e0          	mov    %r12,-0x20(%rbp)
  4b42a7:	4d 8d 2c 0e          	lea    (%r14,%rcx,1),%r13
  4b42ab:	49 89 f4             	mov    %rsi,%r12
  4b42ae:	49 c1 e2 05          	shl    $0x5,%r10
  4b42b2:	49 01 c3             	add    %rax,%r11
  4b42b5:	4a 8d 3c 29          	lea    (%rcx,%r13,1),%rdi
  4b42b9:	49 8d 0c 57          	lea    (%r15,%rdx,2),%rcx
  4b42bd:	49 c1 e1 05          	shl    $0x5,%r9
  4b42c1:	49 01 c2             	add    %rax,%r10
  4b42c4:	4c 8d 3c 51          	lea    (%rcx,%rdx,2),%r15
  4b42c8:	48 8d 34 11          	lea    (%rcx,%rdx,1),%rsi
  4b42cc:	49 01 c1             	add    %rax,%r9
  4b42cf:	48 89 bd 60 fe ff ff 	mov    %rdi,-0x1a0(%rbp)
  4b42d6:	4c 01 fa             	add    %r15,%rdx
  4b42d9:	48 89 cf             	mov    %rcx,%rdi
  4b42dc:	4c 89 f9             	mov    %r15,%rcx
  4b42df:	4c 8b 7d c0          	mov    -0x40(%rbp),%r15
  4b42e3:	48 c1 e2 05          	shl    $0x5,%rdx
  4b42e7:	48 c1 e7 05          	shl    $0x5,%rdi
  4b42eb:	4c 89 a5 70 fe ff ff 	mov    %r12,-0x190(%rbp)
  4b42f2:	48 c1 e6 05          	shl    $0x5,%rsi
  4b42f6:	48 c1 e1 05          	shl    $0x5,%rcx
  4b42fa:	48 01 c7             	add    %rax,%rdi
  4b42fd:	4c 89 ad 20 fe ff ff 	mov    %r13,-0x1e0(%rbp)
  4b4304:	48 01 c6             	add    %rax,%rsi
  4b4307:	48 01 c1             	add    %rax,%rcx
  4b430a:	48 01 d0             	add    %rdx,%rax
  4b430d:	4c 89 fa             	mov    %r15,%rdx
  4b4310:	48 c1 e2 05          	shl    $0x5,%rdx
  4b4314:	48 89 95 40 fe ff ff 	mov    %rdx,-0x1c0(%rbp)
  4b431b:	48 0f af d3          	imul   %rbx,%rdx
  4b431f:	4c 01 e2             	add    %r12,%rdx
  4b4322:	48 89 95 78 fe ff ff 	mov    %rdx,-0x188(%rbp)
  4b4329:	4c 89 fa             	mov    %r15,%rdx
  4b432c:	48 c1 e2 06          	shl    $0x6,%rdx
  4b4330:	48 89 95 48 fe ff ff 	mov    %rdx,-0x1b8(%rbp)
  4b4337:	48 0f af d3          	imul   %rbx,%rdx
  4b433b:	4c 01 e2             	add    %r12,%rdx
  4b433e:	4c 8b a5 70 fe ff ff 	mov    -0x190(%rbp),%r12
  4b4345:	48 89 95 80 fe ff ff 	mov    %rdx,-0x180(%rbp)
  4b434c:	4b 8d 14 7f          	lea    (%r15,%r15,2),%rdx
  4b4350:	49 89 d7             	mov    %rdx,%r15
  4b4353:	49 c1 e7 05          	shl    $0x5,%r15
  4b4357:	4c 89 bd 38 fe ff ff 	mov    %r15,-0x1c8(%rbp)
  4b435e:	4c 0f af fb          	imul   %rbx,%r15
  4b4362:	4d 01 fc             	add    %r15,%r12
  4b4365:	4c 8b 7d c0          	mov    -0x40(%rbp),%r15
  4b4369:	4c 89 a5 88 fe ff ff 	mov    %r12,-0x178(%rbp)
  4b4370:	4c 8b a5 70 fe ff ff 	mov    -0x190(%rbp),%r12
  4b4377:	49 c1 e7 07          	shl    $0x7,%r15
  4b437b:	4c 89 bd 50 fe ff ff 	mov    %r15,-0x1b0(%rbp)
  4b4382:	4c 0f af fb          	imul   %rbx,%r15
  4b4386:	4d 01 e7             	add    %r12,%r15
  4b4389:	4c 89 bd 90 fe ff ff 	mov    %r15,-0x170(%rbp)
  4b4390:	4c 8b 7d c0          	mov    -0x40(%rbp),%r15
  4b4394:	4f 8d 3c bf          	lea    (%r15,%r15,4),%r15
  4b4398:	49 c1 e7 05          	shl    $0x5,%r15
  4b439c:	4c 89 bd 30 fe ff ff 	mov    %r15,-0x1d0(%rbp)
  4b43a3:	4c 0f af fb          	imul   %rbx,%r15
  4b43a7:	48 c1 e2 06          	shl    $0x6,%rdx
  4b43ab:	48 89 95 28 fe ff ff 	mov    %rdx,-0x1d8(%rbp)
  4b43b2:	48 0f af d3          	imul   %rbx,%rdx
  4b43b6:	4d 01 e7             	add    %r12,%r15
  4b43b9:	4c 01 e2             	add    %r12,%rdx
  4b43bc:	48 89 95 98 fe ff ff 	mov    %rdx,-0x168(%rbp)
  4b43c3:	48 69 55 c0 e0 00 00 	imul   $0xe0,-0x40(%rbp),%rdx
  4b43ca:	00 
  4b43cb:	48 89 95 58 fe ff ff 	mov    %rdx,-0x1a8(%rbp)
  4b43d2:	48 0f af d3          	imul   %rbx,%rdx
  4b43d6:	4c 01 e2             	add    %r12,%rdx
  4b43d9:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  4b43e0:	41 db 28             	fldt   (%r8)
  4b43e3:	4c 8b ad 70 fe ff ff 	mov    -0x190(%rbp),%r13
  4b43ea:	41 db 68 10          	fldt   0x10(%r8)
  4b43ee:	41 db 2b             	fldt   (%r11)
  4b43f1:	db 7d a0             	fstpt  -0x60(%rbp)
  4b43f4:	41 db 6b 10          	fldt   0x10(%r11)
  4b43f8:	db bd 10 ff ff ff    	fstpt  -0xf0(%rbp)
  4b43fe:	41 db 2a             	fldt   (%r10)
  4b4401:	41 db 6a 10          	fldt   0x10(%r10)
  4b4405:	db 7d 80             	fstpt  -0x80(%rbp)
  4b4408:	41 db 29             	fldt   (%r9)
  4b440b:	db bd 30 ff ff ff    	fstpt  -0xd0(%rbp)
  4b4411:	41 db 69 10          	fldt   0x10(%r9)
  4b4415:	db bd 20 ff ff ff    	fstpt  -0xe0(%rbp)
  4b441b:	db 2f                	fldt   (%rdi)
  4b441d:	db 6f 10             	fldt   0x10(%rdi)
  4b4420:	db bd 50 ff ff ff    	fstpt  -0xb0(%rbp)
  4b4426:	db 2e                	fldt   (%rsi)
  4b4428:	db bd e0 fe ff ff    	fstpt  -0x120(%rbp)
  4b442e:	db 6e 10             	fldt   0x10(%rsi)
  4b4431:	db bd f0 fe ff ff    	fstpt  -0x110(%rbp)
  4b4437:	db 29                	fldt   (%rcx)
  4b4439:	db bd 60 ff ff ff    	fstpt  -0xa0(%rbp)
  4b443f:	db 69 10             	fldt   0x10(%rcx)
  4b4442:	db bd 70 ff ff ff    	fstpt  -0x90(%rbp)
  4b4448:	db 28                	fldt   (%rax)
  4b444a:	db 7d c0             	fstpt  -0x40(%rbp)
  4b444d:	db 68 10             	fldt   0x10(%rax)
  4b4450:	db bd 00 ff ff ff    	fstpt  -0x100(%rbp)
  4b4456:	41 db 6d 00          	fldt   0x0(%r13)
  4b445a:	db 7d b0             	fstpt  -0x50(%rbp)
  4b445d:	41 db 6d 10          	fldt   0x10(%r13)
  4b4461:	db bd d0 fe ff ff    	fstpt  -0x130(%rbp)
  4b4467:	48 85 db             	test   %rbx,%rbx
  4b446a:	0f 84 7c 02 00 00    	je     4b46ec <void micron::math::fft::__impl::__engine<long double>::__butterflies<8ul, false>(micron::math::complex<long double>*, unsigned long, unsigned long, unsigned long, unsigned long) const+0x4ac>
  4b4470:	4c 8b a5 78 fe ff ff 	mov    -0x188(%rbp),%r12
  4b4477:	4c 8b ad 80 fe ff ff 	mov    -0x180(%rbp),%r13
  4b447e:	41 db 2c 24          	fldt   (%r12)
  4b4482:	41 db 6c 24 10       	fldt   0x10(%r12)
  4b4487:	4c 8b a5 88 fe ff ff 	mov    -0x178(%rbp),%r12
  4b448e:	db 7d 90             	fstpt  -0x70(%rbp)
  4b4491:	41 db 6d 00          	fldt   0x0(%r13)
  4b4495:	db bd 40 ff ff ff    	fstpt  -0xc0(%rbp)
  4b449b:	41 db 6d 10          	fldt   0x10(%r13)
  4b449f:	4c 8b ad 90 fe ff ff 	mov    -0x170(%rbp),%r13
  4b44a6:	db bd c0 fe ff ff    	fstpt  -0x140(%rbp)
  4b44ac:	41 db 2c 24          	fldt   (%r12)
  4b44b0:	db bd b0 fe ff ff    	fstpt  -0x150(%rbp)
  4b44b6:	41 db 6c 24 10       	fldt   0x10(%r12)
  4b44bb:	4c 8b a5 98 fe ff ff 	mov    -0x168(%rbp),%r12
  4b44c2:	db bd a0 fe ff ff    	fstpt  -0x160(%rbp)
  4b44c8:	41 db 6d 00          	fldt   0x0(%r13)
  4b44cc:	db bd 10 fe ff ff    	fstpt  -0x1f0(%rbp)
  4b44d2:	41 db 6d 10          	fldt   0x10(%r13)
  4b44d6:	db bd 00 fe ff ff    	fstpt  -0x200(%rbp)
  4b44dc:	41 db 2c 24          	fldt   (%r12)
  4b44e0:	db bd f0 fd ff ff    	fstpt  -0x210(%rbp)
  4b44e6:	41 db 6c 24 10       	fldt   0x10(%r12)
  4b44eb:	db bd e0 fd ff ff    	fstpt  -0x220(%rbp)
  4b44f1:	db 6d b0             	fldt   -0x50(%rbp)
  4b44f4:	d9 c0                	fld    %st(0)
  4b44f6:	d8 ce                	fmul   %st(6),%st
  4b44f8:	db bd d0 fd ff ff    	fstpt  -0x230(%rbp)
  4b44fe:	db ad d0 fe ff ff    	fldt   -0x130(%rbp)
  4b4504:	d9 c0                	fld    %st(0)
  4b4506:	d8 ce                	fmul   %st(6),%st
  4b4508:	db bd c0 fd ff ff    	fstpt  -0x240(%rbp)
  4b450e:	de ce                	fmulp  %st,%st(6)
  4b4510:	de cc                	fmulp  %st,%st(4)
  4b4512:	d9 cb                	fxch   %st(3)
  4b4514:	de c4                	faddp  %st,%st(4)
  4b4516:	d9 c2                	fld    %st(2)
  4b4518:	db 6d a0             	fldt   -0x60(%rbp)
  4b451b:	de cc                	fmulp  %st,%st(4)
  4b451d:	d9 cb                	fxch   %st(3)
  4b451f:	db bd b0 fd ff ff    	fstpt  -0x250(%rbp)
  4b4525:	db ad 10 ff ff ff    	fldt   -0xf0(%rbp)
  4b452b:	d9 c0                	fld    %st(0)
  4b452d:	db 6d 90             	fldt   -0x70(%rbp)
  4b4530:	dc c9                	fmul   %st,%st(1)
  4b4532:	d9 c9                	fxch   %st(1)
  4b4534:	db 7d 90             	fstpt  -0x70(%rbp)
  4b4537:	db 6d a0             	fldt   -0x60(%rbp)
  4b453a:	de c9                	fmulp  %st,%st(1)
  4b453c:	d9 c9                	fxch   %st(1)
  4b453e:	de cc                	fmulp  %st,%st(4)
  4b4540:	de c3                	faddp  %st,%st(3)
  4b4542:	d9 ca                	fxch   %st(2)
  4b4544:	db bd 10 ff ff ff    	fstpt  -0xf0(%rbp)
  4b454a:	db ad 40 ff ff ff    	fldt   -0xc0(%rbp)
  4b4550:	d9 c0                	fld    %st(0)
  4b4552:	d8 ca                	fmul   %st(2),%st
  4b4554:	db 7d a0             	fstpt  -0x60(%rbp)
  4b4557:	db 6d 80             	fldt   -0x80(%rbp)
  4b455a:	d9 c0                	fld    %st(0)
  4b455c:	db ad c0 fe ff ff    	fldt   -0x140(%rbp)
  4b4562:	dc c9                	fmul   %st,%st(1)
  4b4564:	de cc                	fmulp  %st,%st(4)
  4b4566:	d9 c9                	fxch   %st(1)
  4b4568:	de ca                	fmulp  %st,%st(2)
  4b456a:	d9 ca                	fxch   %st(2)
  4b456c:	de c1                	faddp  %st,%st(1)
  4b456e:	db 7d 80             	fstpt  -0x80(%rbp)
  4b4571:	db ad 30 ff ff ff    	fldt   -0xd0(%rbp)
  4b4577:	d9 c0                	fld    %st(0)
  4b4579:	db ad b0 fe ff ff    	fldt   -0x150(%rbp)
  4b457f:	dc c9                	fmul   %st,%st(1)
  4b4581:	d9 c9                	fxch   %st(1)
  4b4583:	db bd 40 ff ff ff    	fstpt  -0xc0(%rbp)
  4b4589:	db ad 20 ff ff ff    	fldt   -0xe0(%rbp)
  4b458f:	d9 c0                	fld    %st(0)
  4b4591:	db ad a0 fe ff ff    	fldt   -0x160(%rbp)
  4b4597:	dc c9                	fmul   %st,%st(1)
  4b4599:	d9 c9                	fxch   %st(1)
  4b459b:	db bd 30 ff ff ff    	fstpt  -0xd0(%rbp)
  4b45a1:	de cb                	fmulp  %st,%st(3)
  4b45a3:	de c9                	fmulp  %st,%st(1)
  4b45a5:	de c1                	faddp  %st,%st(1)
  4b45a7:	db bd 20 ff ff ff    	fstpt  -0xe0(%rbp)
  4b45ad:	db ad 10 fe ff ff    	fldt   -0x1f0(%rbp)
  4b45b3:	d9 c0                	fld    %st(0)
  4b45b5:	d8 cb                	fmul   %st(3),%st
  4b45b7:	db bd c0 fe ff ff    	fstpt  -0x140(%rbp)
  4b45bd:	db ad 00 fe ff ff    	fldt   -0x200(%rbp)
  4b45c3:	d9 c0                	fld    %st(0)
  4b45c5:	db ad 50 ff ff ff    	fldt   -0xb0(%rbp)
  4b45cb:	dc c9                	fmul   %st,%st(1)
  4b45cd:	d9 c9                	fxch   %st(1)
  4b45cf:	db bd b0 fe ff ff    	fstpt  -0x150(%rbp)
  4b45d5:	d9 cc                	fxch   %st(4)
  4b45d7:	de c9                	fmulp  %st,%st(1)
  4b45d9:	d9 c9                	fxch   %st(1)
  4b45db:	de cb                	fmulp  %st,%st(3)
  4b45dd:	de c2                	faddp  %st,%st(2)
  4b45df:	d9 c9                	fxch   %st(1)
  4b45e1:	db bd 50 ff ff ff    	fstpt  -0xb0(%rbp)
  4b45e7:	db ad e0 fe ff ff    	fldt   -0x120(%rbp)
  4b45ed:	d9 c0                	fld    %st(0)
  4b45ef:	41 db 2f             	fldt   (%r15)
  4b45f2:	de c9                	fmulp  %st,%st(1)
  4b45f4:	db bd e0 fe ff ff    	fstpt  -0x120(%rbp)
  4b45fa:	41 db 6f 10          	fldt   0x10(%r15)
  4b45fe:	db ad f0 fe ff ff    	fldt   -0x110(%rbp)
  4b4604:	dc c9                	fmul   %st,%st(1)
  4b4606:	d9 c9                	fxch   %st(1)
  4b4608:	db bd a0 fe ff ff    	fstpt  -0x160(%rbp)
  4b460e:	41 db 6f 10          	fldt   0x10(%r15)
  4b4612:	de ca                	fmulp  %st,%st(2)
  4b4614:	41 db 2f             	fldt   (%r15)
  4b4617:	de c9                	fmulp  %st,%st(1)
  4b4619:	de c1                	faddp  %st,%st(1)
  4b461b:	db bd f0 fe ff ff    	fstpt  -0x110(%rbp)
  4b4621:	db ad f0 fd ff ff    	fldt   -0x210(%rbp)
  4b4627:	d9 c0                	fld    %st(0)
  4b4629:	db ad 60 ff ff ff    	fldt   -0xa0(%rbp)
  4b462f:	dc c9                	fmul   %st,%st(1)
  4b4631:	db ad 70 ff ff ff    	fldt   -0x90(%rbp)
  4b4637:	d9 c0                	fld    %st(0)
  4b4639:	db ad e0 fd ff ff    	fldt   -0x220(%rbp)
  4b463f:	dc c9                	fmul   %st,%st(1)
  4b4641:	de cb                	fmulp  %st,%st(3)
  4b4643:	d9 c9                	fxch   %st(1)
  4b4645:	de cc                	fmulp  %st,%st(4)
  4b4647:	d9 c9                	fxch   %st(1)
  4b4649:	de c3                	faddp  %st,%st(3)
  4b464b:	d9 ca                	fxch   %st(2)
  4b464d:	db bd 70 ff ff ff    	fstpt  -0x90(%rbp)
  4b4653:	db 6d c0             	fldt   -0x40(%rbp)
  4b4656:	d9 c0                	fld    %st(0)
  4b4658:	db 2a                	fldt   (%rdx)
  4b465a:	de c9                	fmulp  %st,%st(1)
  4b465c:	db 7d c0             	fstpt  -0x40(%rbp)
  4b465f:	db ad 00 ff ff ff    	fldt   -0x100(%rbp)
  4b4665:	d9 c0                	fld    %st(0)
  4b4667:	db 6a 10             	fldt   0x10(%rdx)
  4b466a:	de c9                	fmulp  %st,%st(1)
  4b466c:	db 6a 10             	fldt   0x10(%rdx)
  4b466f:	de cb                	fmulp  %st,%st(3)
  4b4671:	db 2a                	fldt   (%rdx)
  4b4673:	de ca                	fmulp  %st,%st(2)
  4b4675:	d9 ca                	fxch   %st(2)
  4b4677:	de c1                	faddp  %st,%st(1)
  4b4679:	db bd 00 ff ff ff    	fstpt  -0x100(%rbp)
  4b467f:	db 6d c0             	fldt   -0x40(%rbp)
  4b4682:	de e1                	fsubp  %st,%st(1)
  4b4684:	db 7d c0             	fstpt  -0x40(%rbp)
  4b4687:	de e1                	fsubp  %st,%st(1)
  4b4689:	db bd 60 ff ff ff    	fstpt  -0xa0(%rbp)
  4b468f:	db ad e0 fe ff ff    	fldt   -0x120(%rbp)
  4b4695:	db ad a0 fe ff ff    	fldt   -0x160(%rbp)
  4b469b:	de e9                	fsubrp %st,%st(1)
  4b469d:	db bd e0 fe ff ff    	fstpt  -0x120(%rbp)
  4b46a3:	db ad c0 fe ff ff    	fldt   -0x140(%rbp)
  4b46a9:	db ad b0 fe ff ff    	fldt   -0x150(%rbp)
  4b46af:	de e9                	fsubrp %st,%st(1)
  4b46b1:	db ad 40 ff ff ff    	fldt   -0xc0(%rbp)
  4b46b7:	db ad 30 ff ff ff    	fldt   -0xd0(%rbp)
  4b46bd:	de e9                	fsubrp %st,%st(1)
  4b46bf:	db bd 30 ff ff ff    	fstpt  -0xd0(%rbp)
  4b46c5:	db 6d a0             	fldt   -0x60(%rbp)
  4b46c8:	de e2                	fsubp  %st,%st(2)
  4b46ca:	db ad b0 fd ff ff    	fldt   -0x250(%rbp)
  4b46d0:	db 6d 90             	fldt   -0x70(%rbp)
  4b46d3:	de e9                	fsubrp %st,%st(1)
  4b46d5:	db 7d a0             	fstpt  -0x60(%rbp)
  4b46d8:	db ad d0 fd ff ff    	fldt   -0x230(%rbp)
  4b46de:	db ad c0 fd ff ff    	fldt   -0x240(%rbp)
  4b46e4:	de e9                	fsubrp %st,%st(1)
  4b46e6:	d9 cb                	fxch   %st(3)
  4b46e8:	d9 ca                	fxch   %st(2)
  4b46ea:	d9 c9                	fxch   %st(1)
  4b46ec:	d9 c3                	fld    %st(3)
  4b46ee:	48 83 c3 01          	add    $0x1,%rbx
  4b46f2:	49 83 c0 20          	add    $0x20,%r8
  4b46f6:	49 83 c3 20          	add    $0x20,%r11
  4b46fa:	d8 c1                	fadd   %st(1),%st
  4b46fc:	49 83 c2 20          	add    $0x20,%r10
  4b4700:	49 83 c1 20          	add    $0x20,%r9
  4b4704:	48 83 c7 20          	add    $0x20,%rdi
  4b4708:	48 83 c6 20          	add    $0x20,%rsi
  4b470c:	48 83 c1 20          	add    $0x20,%rcx
  4b4710:	48 83 c0 20          	add    $0x20,%rax
  4b4714:	db 7d 90             	fstpt  -0x70(%rbp)
  4b4717:	d9 c2                	fld    %st(2)
  4b4719:	db ad 50 ff ff ff    	fldt   -0xb0(%rbp)
  4b471f:	dc c1                	fadd   %st,%st(1)
  4b4721:	d9 c9                	fxch   %st(1)
  4b4723:	db bd 50 ff ff ff    	fstpt  -0xb0(%rbp)
  4b4729:	d9 cc                	fxch   %st(4)
  4b472b:	de e1                	fsubp  %st,%st(1)
  4b472d:	d9 ca                	fxch   %st(2)
  4b472f:	de e3                	fsubp  %st,%st(3)
  4b4731:	d9 c0                	fld    %st(0)
  4b4733:	db ad 60 ff ff ff    	fldt   -0xa0(%rbp)
  4b4739:	dc c1                	fadd   %st,%st(1)
  4b473b:	db 6d 80             	fldt   -0x80(%rbp)
  4b473e:	d9 c0                	fld    %st(0)
  4b4740:	db ad 70 ff ff ff    	fldt   -0x90(%rbp)
  4b4746:	dc c1                	fadd   %st,%st(1)
  4b4748:	d9 cd                	fxch   %st(5)
  4b474a:	de e3                	fsubp  %st,%st(3)
  4b474c:	d9 c9                	fxch   %st(1)
  4b474e:	de e4                	fsubp  %st,%st(4)
  4b4750:	db 6d 90             	fldt   -0x70(%rbp)
  4b4753:	d8 c3                	fadd   %st(3),%st
  4b4755:	db bd c0 fe ff ff    	fstpt  -0x140(%rbp)
  4b475b:	db ad 50 ff ff ff    	fldt   -0xb0(%rbp)
  4b4761:	d9 c0                	fld    %st(0)
  4b4763:	d8 c2                	fadd   %st(2),%st
  4b4765:	db 7d 80             	fstpt  -0x80(%rbp)
  4b4768:	d9 c5                	fld    %st(5)
  4b476a:	d8 c5                	fadd   %st(5),%st
  4b476c:	db bd 70 ff ff ff    	fstpt  -0x90(%rbp)
  4b4772:	d9 c6                	fld    %st(6)
  4b4774:	d8 e3                	fsub   %st(3),%st
  4b4776:	db bd 60 ff ff ff    	fstpt  -0xa0(%rbp)
  4b477c:	db 6d 90             	fldt   -0x70(%rbp)
  4b477f:	de e4                	fsubp  %st,%st(4)
  4b4781:	d9 cb                	fxch   %st(3)
  4b4783:	db bd b0 fe ff ff    	fstpt  -0x150(%rbp)
  4b4789:	de ea                	fsubrp %st,%st(2)
  4b478b:	d9 c9                	fxch   %st(1)
  4b478d:	db bd a0 fe ff ff    	fstpt  -0x160(%rbp)
  4b4793:	d9 ca                	fxch   %st(2)
  4b4795:	de e1                	fsubp  %st,%st(1)
  4b4797:	db bd 50 ff ff ff    	fstpt  -0xb0(%rbp)
  4b479d:	de c1                	faddp  %st,%st(1)
  4b479f:	db bd 40 ff ff ff    	fstpt  -0xc0(%rbp)
  4b47a5:	db ad e0 fe ff ff    	fldt   -0x120(%rbp)
  4b47ab:	d9 c0                	fld    %st(0)
  4b47ad:	db 6d a0             	fldt   -0x60(%rbp)
  4b47b0:	dc c1                	fadd   %st,%st(1)
  4b47b2:	d9 c9                	fxch   %st(1)
  4b47b4:	db 7d 90             	fstpt  -0x70(%rbp)
  4b47b7:	db ad 10 ff ff ff    	fldt   -0xf0(%rbp)
  4b47bd:	d9 c0                	fld    %st(0)
  4b47bf:	db ad f0 fe ff ff    	fldt   -0x110(%rbp)
  4b47c5:	dc c1                	fadd   %st,%st(1)
  4b47c7:	d9 cb                	fxch   %st(3)
  4b47c9:	de e4                	fsubp  %st,%st(4)
  4b47cb:	d9 cb                	fxch   %st(3)
  4b47cd:	db 7d a0             	fstpt  -0x60(%rbp)
  4b47d0:	de e1                	fsubp  %st,%st(1)
  4b47d2:	db ad 30 ff ff ff    	fldt   -0xd0(%rbp)
  4b47d8:	d9 c0                	fld    %st(0)
  4b47da:	db 6d c0             	fldt   -0x40(%rbp)
  4b47dd:	de c1                	faddp  %st,%st(1)
  4b47df:	db ad 20 ff ff ff    	fldt   -0xe0(%rbp)
  4b47e5:	d9 c0                	fld    %st(0)
  4b47e7:	db ad 00 ff ff ff    	fldt   -0x100(%rbp)
  4b47ed:	dc c1                	fadd   %st,%st(1)
  4b47ef:	db 6d c0             	fldt   -0x40(%rbp)
  4b47f2:	de ed                	fsubrp %st,%st(5)
  4b47f4:	de ea                	fsubrp %st,%st(2)
  4b47f6:	db 6d 90             	fldt   -0x70(%rbp)
  4b47f9:	d9 c0                	fld    %st(0)
  4b47fb:	d8 c4                	fadd   %st(4),%st
  4b47fd:	db 7d 90             	fstpt  -0x70(%rbp)
  4b4800:	d9 ce                	fxch   %st(6)
  4b4802:	d9 c0                	fld    %st(0)
  4b4804:	db 7d c0             	fstpt  -0x40(%rbp)
  4b4807:	d8 c1                	fadd   %st(1),%st
  4b4809:	db 6d a0             	fldt   -0x60(%rbp)
  4b480c:	d8 c3                	fadd   %st(3),%st
  4b480e:	db bd 30 ff ff ff    	fstpt  -0xd0(%rbp)
  4b4814:	4c 8b ad 20 fe ff ff 	mov    -0x1e0(%rbp),%r13
  4b481b:	4c 8b a5 60 fe ff ff 	mov    -0x1a0(%rbp),%r12
  4b4822:	d9 c5                	fld    %st(5)
  4b4824:	d8 e5                	fsub   %st(5),%st
  4b4826:	db bd 20 ff ff ff    	fstpt  -0xe0(%rbp)
  4b482c:	d9 cb                	fxch   %st(3)
  4b482e:	de ee                	fsubrp %st,%st(6)
  4b4830:	db 6d c0             	fldt   -0x40(%rbp)
  4b4833:	de e1                	fsubp  %st,%st(1)
  4b4835:	db 6d a0             	fldt   -0x60(%rbp)
  4b4838:	de e2                	fsubp  %st,%st(2)
  4b483a:	d9 cc                	fxch   %st(4)
  4b483c:	de c3                	faddp  %st,%st(3)
  4b483e:	db 6d b0             	fldt   -0x50(%rbp)
  4b4841:	db 6d 90             	fldt   -0x70(%rbp)
  4b4844:	de c9                	fmulp  %st,%st(1)
  4b4846:	db ad d0 fe ff ff    	fldt   -0x130(%rbp)
  4b484c:	d9 c0                	fld    %st(0)
  4b484e:	d8 cc                	fmul   %st(4),%st
  4b4850:	de ea                	fsubrp %st,%st(2)
  4b4852:	d9 c9                	fxch   %st(1)
  4b4854:	db 7d c0             	fstpt  -0x40(%rbp)
  4b4857:	db 6d 90             	fldt   -0x70(%rbp)
  4b485a:	de c9                	fmulp  %st,%st(1)
  4b485c:	db 6d b0             	fldt   -0x50(%rbp)
  4b485f:	de cb                	fmulp  %st,%st(3)
  4b4861:	de c2                	faddp  %st,%st(2)
  4b4863:	d9 c9                	fxch   %st(1)
  4b4865:	db 7d b0             	fstpt  -0x50(%rbp)
  4b4868:	41 db 2e             	fldt   (%r14)
  4b486b:	41 db 6e 10          	fldt   0x10(%r14)
  4b486f:	db ad 30 ff ff ff    	fldt   -0xd0(%rbp)
  4b4875:	d8 ca                	fmul   %st(2),%st
  4b4877:	db ad 20 ff ff ff    	fldt   -0xe0(%rbp)
  4b487d:	d8 ca                	fmul   %st(2),%st
  4b487f:	de e9                	fsubrp %st,%st(1)
  4b4881:	db 7d a0             	fstpt  -0x60(%rbp)
  4b4884:	db ad 30 ff ff ff    	fldt   -0xd0(%rbp)
  4b488a:	de c9                	fmulp  %st,%st(1)
  4b488c:	db ad 20 ff ff ff    	fldt   -0xe0(%rbp)
  4b4892:	de ca                	fmulp  %st,%st(2)
  4b4894:	de c1                	faddp  %st,%st(1)
  4b4896:	db 7d 90             	fstpt  -0x70(%rbp)
  4b4899:	41 db 6d 00          	fldt   0x0(%r13)
  4b489d:	41 db 6d 10          	fldt   0x10(%r13)
  4b48a1:	d9 c1                	fld    %st(1)
  4b48a3:	d8 ce                	fmul   %st(6),%st
  4b48a5:	d9 c1                	fld    %st(1)
  4b48a7:	d8 ce                	fmul   %st(6),%st
  4b48a9:	de e9                	fsubrp %st,%st(1)
  4b48ab:	d9 c9                	fxch   %st(1)
  4b48ad:	de ce                	fmulp  %st,%st(6)
  4b48af:	d9 c9                	fxch   %st(1)
  4b48b1:	de cc                	fmulp  %st,%st(4)
  4b48b3:	d9 cc                	fxch   %st(4)
  4b48b5:	de c3                	faddp  %st,%st(3)
  4b48b7:	41 db 2c 24          	fldt   (%r12)
  4b48bb:	41 db 6c 24 10       	fldt   0x10(%r12)
  4b48c0:	d9 c1                	fld    %st(1)
  4b48c2:	d8 cb                	fmul   %st(3),%st
  4b48c4:	d9 c1                	fld    %st(1)
  4b48c6:	d8 cd                	fmul   %st(5),%st
  4b48c8:	de e9                	fsubrp %st,%st(1)
  4b48ca:	d9 c9                	fxch   %st(1)
  4b48cc:	de cb                	fmulp  %st,%st(3)
  4b48ce:	d9 c9                	fxch   %st(1)
  4b48d0:	de cb                	fmulp  %st,%st(3)
  4b48d2:	d9 ca                	fxch   %st(2)
  4b48d4:	de c1                	faddp  %st,%st(1)
  4b48d6:	db ad c0 fe ff ff    	fldt   -0x140(%rbp)
  4b48dc:	d9 c0                	fld    %st(0)
  4b48de:	db 6d c0             	fldt   -0x40(%rbp)
  4b48e1:	de c1                	faddp  %st,%st(1)
  4b48e3:	41 db 78 e0          	fstpt  -0x20(%r8)
  4b48e7:	db 6d 80             	fldt   -0x80(%rbp)
  4b48ea:	db 6d b0             	fldt   -0x50(%rbp)
  4b48ed:	de c1                	faddp  %st,%st(1)
  4b48ef:	41 db 78 f0          	fstpt  -0x10(%r8)
  4b48f3:	db ad 70 ff ff ff    	fldt   -0x90(%rbp)
  4b48f9:	db 6d a0             	fldt   -0x60(%rbp)
  4b48fc:	de c1                	faddp  %st,%st(1)
  4b48fe:	41 db 7b e0          	fstpt  -0x20(%r11)
  4b4902:	db ad 60 ff ff ff    	fldt   -0xa0(%rbp)
  4b4908:	4c 8b ad 40 fe ff ff 	mov    -0x1c0(%rbp),%r13
  4b490f:	4c 8b a5 48 fe ff ff 	mov    -0x1b8(%rbp),%r12
  4b4916:	db 6d 90             	fldt   -0x70(%rbp)
  4b4919:	4c 01 ad 78 fe ff ff 	add    %r13,-0x188(%rbp)
  4b4920:	de c1                	faddp  %st,%st(1)
  4b4922:	41 db 7b f0          	fstpt  -0x10(%r11)
  4b4926:	db ad b0 fe ff ff    	fldt   -0x150(%rbp)
  4b492c:	d9 c0                	fld    %st(0)
  4b492e:	d8 c6                	fadd   %st(6),%st
  4b4930:	41 db 7a e0          	fstpt  -0x20(%r10)
  4b4934:	db ad a0 fe ff ff    	fldt   -0x160(%rbp)
  4b493a:	d9 c0                	fld    %st(0)
  4b493c:	d8 c6                	fadd   %st(6),%st
  4b493e:	41 db 7a f0          	fstpt  -0x10(%r10)
  4b4942:	db ad 50 ff ff ff    	fldt   -0xb0(%rbp)
  4b4948:	d8 c5                	fadd   %st(5),%st
  4b494a:	41 db 79 e0          	fstpt  -0x20(%r9)
  4b494e:	db ad 40 ff ff ff    	fldt   -0xc0(%rbp)
  4b4954:	d8 c4                	fadd   %st(4),%st
  4b4956:	41 db 79 f0          	fstpt  -0x10(%r9)
  4b495a:	db 6d c0             	fldt   -0x40(%rbp)
  4b495d:	de eb                	fsubrp %st,%st(3)
  4b495f:	d9 ca                	fxch   %st(2)
  4b4961:	db 7f e0             	fstpt  -0x20(%rdi)
  4b4964:	db 6d 80             	fldt   -0x80(%rbp)
  4b4967:	db 6d b0             	fldt   -0x50(%rbp)
  4b496a:	de e9                	fsubrp %st,%st(1)
  4b496c:	db 7f f0             	fstpt  -0x10(%rdi)
  4b496f:	db ad 70 ff ff ff    	fldt   -0x90(%rbp)
  4b4975:	db 6d a0             	fldt   -0x60(%rbp)
  4b4978:	de e9                	fsubrp %st,%st(1)
  4b497a:	db 7e e0             	fstpt  -0x20(%rsi)
  4b497d:	db ad 60 ff ff ff    	fldt   -0xa0(%rbp)
  4b4983:	db 6d 90             	fldt   -0x70(%rbp)
  4b4986:	de e9                	fsubrp %st,%st(1)
  4b4988:	db 7e f0             	fstpt  -0x10(%rsi)
  4b498b:	de e5                	fsubp  %st,%st(5)
  4b498d:	d9 cc                	fxch   %st(4)
  4b498f:	db 79 e0             	fstpt  -0x20(%rcx)
  4b4992:	d9 ca                	fxch   %st(2)
  4b4994:	de eb                	fsubrp %st,%st(3)
  4b4996:	d9 ca                	fxch   %st(2)
  4b4998:	db 79 f0             	fstpt  -0x10(%rcx)
  4b499b:	db ad 50 ff ff ff    	fldt   -0xb0(%rbp)
  4b49a1:	de e2                	fsubp  %st,%st(2)
  4b49a3:	d9 c9                	fxch   %st(1)
  4b49a5:	db 78 e0             	fstpt  -0x20(%rax)
  4b49a8:	db ad 40 ff ff ff    	fldt   -0xc0(%rbp)
  4b49ae:	de e1                	fsubp  %st,%st(1)
  4b49b0:	db 78 f0             	fstpt  -0x10(%rax)
  4b49b3:	4c 01 a5 80 fe ff ff 	add    %r12,-0x180(%rbp)
  4b49ba:	4c 8b ad 38 fe ff ff 	mov    -0x1c8(%rbp),%r13
  4b49c1:	4c 8b a5 50 fe ff ff 	mov    -0x1b0(%rbp),%r12
  4b49c8:	4c 01 ad 88 fe ff ff 	add    %r13,-0x178(%rbp)
  4b49cf:	4c 01 a5 90 fe ff ff 	add    %r12,-0x170(%rbp)
  4b49d6:	4c 8b ad 28 fe ff ff 	mov    -0x1d8(%rbp),%r13
  4b49dd:	4c 03 bd 30 fe ff ff 	add    -0x1d0(%rbp),%r15
  4b49e4:	4c 01 ad 98 fe ff ff 	add    %r13,-0x168(%rbp)
  4b49eb:	48 03 95 58 fe ff ff 	add    -0x1a8(%rbp),%rdx
  4b49f2:	48 39 9d 68 fe ff ff 	cmp    %rbx,-0x198(%rbp)
  4b49f9:	0f 85 e1 f9 ff ff    	jne    4b43e0 <void micron::math::fft::__impl::__engine<long double>::__butterflies<8ul, false>(micron::math::complex<long double>*, unsigned long, unsigned long, unsigned long, unsigned long) const+0x1a0>
  4b49ff:	48 8b 5d d8          	mov    -0x28(%rbp),%rbx
  4b4a03:	4c 8b 65 e0          	mov    -0x20(%rbp),%r12
  4b4a07:	4c 8b 6d e8          	mov    -0x18(%rbp),%r13
  4b4a0b:	4c 8b 75 f0          	mov    -0x10(%rbp),%r14
  4b4a0f:	4c 8b 7d f8          	mov    -0x8(%rbp),%r15
  4b4a13:	c9                   	leave
  4b4a14:	c3                   	ret
  4b4a15:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  4b4a1c:	00 00 00 
  4b4a1f:	90                   	nop


00000000004b4c70 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const>:
  4b4c70:	48 3b 8f 80 00 00 00 	cmp    0x80(%rdi),%rcx
  4b4c77:	0f 84 b3 03 00 00    	je     4b5030 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x3c0>
  4b4c7d:	55                   	push   %rbp
  4b4c7e:	48 89 c8             	mov    %rcx,%rax
  4b4c81:	48 89 e5             	mov    %rsp,%rbp
  4b4c84:	48 83 ec 70          	sub    $0x70,%rsp
  4b4c88:	4c 89 65 e0          	mov    %r12,-0x20(%rbp)
  4b4c8c:	49 89 d4             	mov    %rdx,%r12
  4b4c8f:	48 8b 57 70          	mov    0x70(%rdi),%rdx
  4b4c93:	48 89 5d d8          	mov    %rbx,-0x28(%rbp)
  4b4c97:	4c 89 c3             	mov    %r8,%rbx
  4b4c9a:	4c 8b 0c ca          	mov    (%rdx,%rcx,8),%r9
  4b4c9e:	48 8b 4f 58          	mov    0x58(%rdi),%rcx
  4b4ca2:	4c 89 7d f8          	mov    %r15,-0x8(%rbp)
  4b4ca6:	49 89 ff             	mov    %rdi,%r15
  4b4ca9:	4c 8b 5c c1 08       	mov    0x8(%rcx,%rax,8),%r11
  4b4cae:	49 83 fb 01          	cmp    $0x1,%r11
  4b4cb2:	0f 84 10 02 00 00    	je     4b4ec8 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x258>
  4b4cb8:	4d 85 c9             	test   %r9,%r9
  4b4cbb:	0f 84 e0 01 00 00    	je     4b4ea1 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x231>
  4b4cc1:	4c 89 cf             	mov    %r9,%rdi
  4b4cc4:	4c 89 6d e8          	mov    %r13,-0x18(%rbp)
  4b4cc8:	4d 89 dd             	mov    %r11,%r13
  4b4ccb:	48 c1 e3 05          	shl    $0x5,%rbx
  4b4ccf:	49 0f af f8          	imul   %r8,%rdi
  4b4cd3:	4d 89 c8             	mov    %r9,%r8
  4b4cd6:	4c 89 75 f0          	mov    %r14,-0x10(%rbp)
  4b4cda:	49 c1 e5 05          	shl    $0x5,%r13
  4b4cde:	4c 8d 70 01          	lea    0x1(%rax),%r14
  4b4ce2:	4c 89 e2             	mov    %r12,%rdx
  4b4ce5:	45 31 d2             	xor    %r10d,%r10d
  4b4ce8:	48 89 7c 24 38       	mov    %rdi,0x38(%rsp)
  4b4ced:	41 83 e0 03          	and    $0x3,%r8d
  4b4cf1:	0f 84 c5 00 00 00    	je     4b4dbc <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x14c>
  4b4cf7:	49 83 f8 01          	cmp    $0x1,%r8
  4b4cfb:	74 60                	je     4b4d5d <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0xed>
  4b4cfd:	49 83 f8 02          	cmp    $0x2,%r8
  4b4d01:	0f 85 39 03 00 00    	jne    4b5040 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x3d0>
  4b4d07:	4c 8b 44 24 38       	mov    0x38(%rsp),%r8
  4b4d0c:	4c 89 f1             	mov    %r14,%rcx
  4b4d0f:	4c 89 ff             	mov    %r15,%rdi
  4b4d12:	48 89 44 24 08       	mov    %rax,0x8(%rsp)
  4b4d17:	4c 89 54 24 10       	mov    %r10,0x10(%rsp)
  4b4d1c:	4c 89 5c 24 18       	mov    %r11,0x18(%rsp)
  4b4d21:	4c 89 4c 24 20       	mov    %r9,0x20(%rsp)
  4b4d26:	48 89 54 24 28       	mov    %rdx,0x28(%rsp)
  4b4d2b:	48 89 74 24 30       	mov    %rsi,0x30(%rsp)
  4b4d30:	e8 3b ff ff ff       	call   4b4c70 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const>
  4b4d35:	4c 8b 54 24 10       	mov    0x10(%rsp),%r10
  4b4d3a:	48 8b 74 24 30       	mov    0x30(%rsp),%rsi
  4b4d3f:	48 8b 54 24 28       	mov    0x28(%rsp),%rdx
  4b4d44:	48 8b 44 24 08       	mov    0x8(%rsp),%rax
  4b4d49:	4c 8b 5c 24 18       	mov    0x18(%rsp),%r11
  4b4d4e:	4c 8b 4c 24 20       	mov    0x20(%rsp),%r9
  4b4d53:	49 83 c2 01          	add    $0x1,%r10
  4b4d57:	48 01 de             	add    %rbx,%rsi
  4b4d5a:	4c 01 ea             	add    %r13,%rdx
  4b4d5d:	4c 8b 44 24 38       	mov    0x38(%rsp),%r8
  4b4d62:	4c 89 f1             	mov    %r14,%rcx
  4b4d65:	4c 89 ff             	mov    %r15,%rdi
  4b4d68:	48 89 44 24 08       	mov    %rax,0x8(%rsp)
  4b4d6d:	4c 89 54 24 10       	mov    %r10,0x10(%rsp)
  4b4d72:	4c 89 5c 24 18       	mov    %r11,0x18(%rsp)
  4b4d77:	4c 89 4c 24 20       	mov    %r9,0x20(%rsp)
  4b4d7c:	48 89 54 24 28       	mov    %rdx,0x28(%rsp)
  4b4d81:	48 89 74 24 30       	mov    %rsi,0x30(%rsp)
  4b4d86:	e8 e5 fe ff ff       	call   4b4c70 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const>
  4b4d8b:	4c 8b 54 24 10       	mov    0x10(%rsp),%r10
  4b4d90:	48 8b 74 24 30       	mov    0x30(%rsp),%rsi
  4b4d95:	48 8b 54 24 28       	mov    0x28(%rsp),%rdx
  4b4d9a:	4c 8b 4c 24 20       	mov    0x20(%rsp),%r9
  4b4d9f:	49 83 c2 01          	add    $0x1,%r10
  4b4da3:	48 01 de             	add    %rbx,%rsi
  4b4da6:	4c 8b 5c 24 18       	mov    0x18(%rsp),%r11
  4b4dab:	48 8b 44 24 08       	mov    0x8(%rsp),%rax
  4b4db0:	4c 01 ea             	add    %r13,%rdx
  4b4db3:	4d 39 d1             	cmp    %r10,%r9
  4b4db6:	0f 84 dd 00 00 00    	je     4b4e99 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x229>
  4b4dbc:	4c 89 4c 24 20       	mov    %r9,0x20(%rsp)
  4b4dc1:	4c 89 5c 24 18       	mov    %r11,0x18(%rsp)
  4b4dc6:	48 89 44 24 08       	mov    %rax,0x8(%rsp)
  4b4dcb:	4c 89 64 24 10       	mov    %r12,0x10(%rsp)
  4b4dd0:	4d 89 d4             	mov    %r10,%r12
  4b4dd3:	4c 8b 44 24 38       	mov    0x38(%rsp),%r8
  4b4dd8:	4c 89 f1             	mov    %r14,%rcx
  4b4ddb:	4c 89 ff             	mov    %r15,%rdi
  4b4dde:	48 89 54 24 28       	mov    %rdx,0x28(%rsp)
  4b4de3:	48 89 74 24 30       	mov    %rsi,0x30(%rsp)
  4b4de8:	49 83 c4 04          	add    $0x4,%r12
  4b4dec:	e8 7f fe ff ff       	call   4b4c70 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const>
  4b4df1:	48 8b 74 24 30       	mov    0x30(%rsp),%rsi
  4b4df6:	4c 89 f1             	mov    %r14,%rcx
  4b4df9:	4c 89 ff             	mov    %r15,%rdi
  4b4dfc:	48 8b 54 24 28       	mov    0x28(%rsp),%rdx
  4b4e01:	4c 8b 44 24 38       	mov    0x38(%rsp),%r8
  4b4e06:	48 01 de             	add    %rbx,%rsi
  4b4e09:	4c 01 ea             	add    %r13,%rdx
  4b4e0c:	48 89 74 24 30       	mov    %rsi,0x30(%rsp)
  4b4e11:	48 89 54 24 28       	mov    %rdx,0x28(%rsp)
  4b4e16:	e8 55 fe ff ff       	call   4b4c70 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const>
  4b4e1b:	48 8b 74 24 30       	mov    0x30(%rsp),%rsi
  4b4e20:	4c 89 f1             	mov    %r14,%rcx
  4b4e23:	4c 89 ff             	mov    %r15,%rdi
  4b4e26:	48 8b 54 24 28       	mov    0x28(%rsp),%rdx
  4b4e2b:	4c 8b 44 24 38       	mov    0x38(%rsp),%r8
  4b4e30:	48 01 de             	add    %rbx,%rsi
  4b4e33:	4c 01 ea             	add    %r13,%rdx
  4b4e36:	48 89 74 24 30       	mov    %rsi,0x30(%rsp)
  4b4e3b:	48 89 54 24 28       	mov    %rdx,0x28(%rsp)
  4b4e40:	e8 2b fe ff ff       	call   4b4c70 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const>
  4b4e45:	48 8b 74 24 30       	mov    0x30(%rsp),%rsi
  4b4e4a:	4c 89 f1             	mov    %r14,%rcx
  4b4e4d:	4c 89 ff             	mov    %r15,%rdi
  4b4e50:	48 8b 54 24 28       	mov    0x28(%rsp),%rdx
  4b4e55:	4c 8b 44 24 38       	mov    0x38(%rsp),%r8
  4b4e5a:	48 01 de             	add    %rbx,%rsi
  4b4e5d:	4c 01 ea             	add    %r13,%rdx
  4b4e60:	48 89 74 24 30       	mov    %rsi,0x30(%rsp)
  4b4e65:	48 89 54 24 28       	mov    %rdx,0x28(%rsp)
  4b4e6a:	e8 01 fe ff ff       	call   4b4c70 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const>
  4b4e6f:	48 8b 74 24 30       	mov    0x30(%rsp),%rsi
  4b4e74:	48 8b 54 24 28       	mov    0x28(%rsp),%rdx
  4b4e79:	48 01 de             	add    %rbx,%rsi
  4b4e7c:	4c 01 ea             	add    %r13,%rdx
  4b4e7f:	4c 39 64 24 20       	cmp    %r12,0x20(%rsp)
  4b4e84:	0f 85 49 ff ff ff    	jne    4b4dd3 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x163>
  4b4e8a:	4c 8b 5c 24 18       	mov    0x18(%rsp),%r11
  4b4e8f:	4c 8b 64 24 10       	mov    0x10(%rsp),%r12
  4b4e94:	48 8b 44 24 08       	mov    0x8(%rsp),%rax
  4b4e99:	4c 8b 6d e8          	mov    -0x18(%rbp),%r13
  4b4e9d:	4c 8b 75 f0          	mov    -0x10(%rbp),%r14
  4b4ea1:	48 8b 5d d8          	mov    -0x28(%rbp),%rbx
  4b4ea5:	4c 89 e6             	mov    %r12,%rsi
  4b4ea8:	4c 89 ff             	mov    %r15,%rdi
  4b4eab:	4c 8b 65 e0          	mov    -0x20(%rbp),%r12
  4b4eaf:	4c 8b 7d f8          	mov    -0x8(%rbp),%r15
  4b4eb3:	4d 89 d8             	mov    %r11,%r8
  4b4eb6:	31 c9                	xor    %ecx,%ecx
  4b4eb8:	48 89 c2             	mov    %rax,%rdx
  4b4ebb:	c9                   	leave
  4b4ebc:	e9 5f fb ff ff       	jmp    4b4a20 <void micron::math::fft::__impl::__engine<long double>::__combine_stage<false>(micron::math::complex<long double>*, unsigned long, unsigned long, unsigned long) const>
  4b4ec1:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  4b4ec8:	4d 85 c9             	test   %r9,%r9
  4b4ecb:	74 d4                	je     4b4ea1 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x231>
  4b4ecd:	49 c1 e1 05          	shl    $0x5,%r9
  4b4ed1:	45 31 c0             	xor    %r8d,%r8d
  4b4ed4:	48 83 fb 01          	cmp    $0x1,%rbx
  4b4ed8:	0f 85 b2 01 00 00    	jne    4b5090 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x420>
  4b4ede:	49 8d 59 e0          	lea    -0x20(%r9),%rbx
  4b4ee2:	48 c1 eb 05          	shr    $0x5,%rbx
  4b4ee6:	48 83 c3 01          	add    $0x1,%rbx
  4b4eea:	83 e3 07             	and    $0x7,%ebx
  4b4eed:	0f 84 9d 00 00 00    	je     4b4f90 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x320>
  4b4ef3:	48 83 fb 01          	cmp    $0x1,%rbx
  4b4ef7:	74 7e                	je     4b4f77 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x307>
  4b4ef9:	48 83 fb 02          	cmp    $0x2,%rbx
  4b4efd:	74 68                	je     4b4f67 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x2f7>
  4b4eff:	48 83 fb 03          	cmp    $0x3,%rbx
  4b4f03:	74 52                	je     4b4f57 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x2e7>
  4b4f05:	48 83 fb 04          	cmp    $0x4,%rbx
  4b4f09:	74 3c                	je     4b4f47 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x2d7>
  4b4f0b:	48 83 fb 05          	cmp    $0x5,%rbx
  4b4f0f:	74 26                	je     4b4f37 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x2c7>
  4b4f11:	48 83 fb 06          	cmp    $0x6,%rbx
  4b4f15:	74 10                	je     4b4f27 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x2b7>
  4b4f17:	c5 7e 6f 3e          	vmovdqu (%rsi),%ymm15
  4b4f1b:	41 b8 20 00 00 00    	mov    $0x20,%r8d
  4b4f21:	c4 41 7e 7f 3c 24    	vmovdqu %ymm15,(%r12)
  4b4f27:	c4 a1 7e 6f 04 06    	vmovdqu (%rsi,%r8,1),%ymm0
  4b4f2d:	c4 81 7e 7f 04 04    	vmovdqu %ymm0,(%r12,%r8,1)
  4b4f33:	49 83 c0 20          	add    $0x20,%r8
  4b4f37:	c4 a1 7e 6f 0c 06    	vmovdqu (%rsi,%r8,1),%ymm1
  4b4f3d:	c4 81 7e 7f 0c 04    	vmovdqu %ymm1,(%r12,%r8,1)
  4b4f43:	49 83 c0 20          	add    $0x20,%r8
  4b4f47:	c4 a1 7e 6f 14 06    	vmovdqu (%rsi,%r8,1),%ymm2
  4b4f4d:	c4 81 7e 7f 14 04    	vmovdqu %ymm2,(%r12,%r8,1)
  4b4f53:	49 83 c0 20          	add    $0x20,%r8
  4b4f57:	c4 a1 7e 6f 1c 06    	vmovdqu (%rsi,%r8,1),%ymm3
  4b4f5d:	c4 81 7e 7f 1c 04    	vmovdqu %ymm3,(%r12,%r8,1)
  4b4f63:	49 83 c0 20          	add    $0x20,%r8
  4b4f67:	c4 a1 7e 6f 24 06    	vmovdqu (%rsi,%r8,1),%ymm4
  4b4f6d:	c4 81 7e 7f 24 04    	vmovdqu %ymm4,(%r12,%r8,1)
  4b4f73:	49 83 c0 20          	add    $0x20,%r8
  4b4f77:	c4 a1 7e 6f 2c 06    	vmovdqu (%rsi,%r8,1),%ymm5
  4b4f7d:	c4 81 7e 7f 2c 04    	vmovdqu %ymm5,(%r12,%r8,1)
  4b4f83:	49 83 c0 20          	add    $0x20,%r8
  4b4f87:	4d 39 c8             	cmp    %r9,%r8
  4b4f8a:	0f 84 96 00 00 00    	je     4b5026 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x3b6>
  4b4f90:	c4 a1 7e 6f 34 06    	vmovdqu (%rsi,%r8,1),%ymm6
  4b4f96:	c4 81 7e 7f 34 04    	vmovdqu %ymm6,(%r12,%r8,1)
  4b4f9c:	c4 a1 7e 6f 7c 06 20 	vmovdqu 0x20(%rsi,%r8,1),%ymm7
  4b4fa3:	c4 81 7e 7f 7c 04 20 	vmovdqu %ymm7,0x20(%r12,%r8,1)
  4b4faa:	c4 21 7e 6f 44 06 40 	vmovdqu 0x40(%rsi,%r8,1),%ymm8
  4b4fb1:	c4 01 7e 7f 44 04 40 	vmovdqu %ymm8,0x40(%r12,%r8,1)
  4b4fb8:	c4 21 7e 6f 4c 06 60 	vmovdqu 0x60(%rsi,%r8,1),%ymm9
  4b4fbf:	c4 01 7e 7f 4c 04 60 	vmovdqu %ymm9,0x60(%r12,%r8,1)
  4b4fc6:	c4 21 7e 6f 94 06 80 	vmovdqu 0x80(%rsi,%r8,1),%ymm10
  4b4fcd:	00 00 00 
  4b4fd0:	c4 01 7e 7f 94 04 80 	vmovdqu %ymm10,0x80(%r12,%r8,1)
  4b4fd7:	00 00 00 
  4b4fda:	c4 21 7e 6f 9c 06 a0 	vmovdqu 0xa0(%rsi,%r8,1),%ymm11
  4b4fe1:	00 00 00 
  4b4fe4:	c4 01 7e 7f 9c 04 a0 	vmovdqu %ymm11,0xa0(%r12,%r8,1)
  4b4feb:	00 00 00 
  4b4fee:	c4 21 7e 6f a4 06 c0 	vmovdqu 0xc0(%rsi,%r8,1),%ymm12
  4b4ff5:	00 00 00 
  4b4ff8:	c4 01 7e 7f a4 04 c0 	vmovdqu %ymm12,0xc0(%r12,%r8,1)
  4b4fff:	00 00 00 
  4b5002:	c4 21 7e 6f ac 06 e0 	vmovdqu 0xe0(%rsi,%r8,1),%ymm13
  4b5009:	00 00 00 
  4b500c:	c4 01 7e 7f ac 04 e0 	vmovdqu %ymm13,0xe0(%r12,%r8,1)
  4b5013:	00 00 00 
  4b5016:	49 81 c0 00 01 00 00 	add    $0x100,%r8
  4b501d:	4d 39 c8             	cmp    %r9,%r8
  4b5020:	0f 85 6a ff ff ff    	jne    4b4f90 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x320>
  4b5026:	c5 f8 77             	vzeroupper
  4b5029:	e9 73 fe ff ff       	jmp    4b4ea1 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x231>
  4b502e:	66 90                	xchg   %ax,%ax
  4b5030:	c5 7e 6f 36          	vmovdqu (%rsi),%ymm14
  4b5034:	c5 7e 7f 32          	vmovdqu %ymm14,(%rdx)
  4b5038:	c5 f8 77             	vzeroupper
  4b503b:	c3                   	ret
  4b503c:	0f 1f 40 00          	nopl   0x0(%rax)
  4b5040:	49 89 f8             	mov    %rdi,%r8
  4b5043:	4c 89 f1             	mov    %r14,%rcx
  4b5046:	4c 89 ff             	mov    %r15,%rdi
  4b5049:	48 89 44 24 18       	mov    %rax,0x18(%rsp)
  4b504e:	4c 89 5c 24 20       	mov    %r11,0x20(%rsp)
  4b5053:	4c 89 4c 24 28       	mov    %r9,0x28(%rsp)
  4b5058:	48 89 74 24 30       	mov    %rsi,0x30(%rsp)
  4b505d:	e8 0e fc ff ff       	call   4b4c70 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const>
  4b5062:	48 8b 74 24 30       	mov    0x30(%rsp),%rsi
  4b5067:	48 8b 44 24 18       	mov    0x18(%rsp),%rax
  4b506c:	41 ba 01 00 00 00    	mov    $0x1,%r10d
  4b5072:	4c 8b 5c 24 20       	mov    0x20(%rsp),%r11
  4b5077:	4c 8b 4c 24 28       	mov    0x28(%rsp),%r9
  4b507c:	4b 8d 14 2c          	lea    (%r12,%r13,1),%rdx
  4b5080:	48 01 de             	add    %rbx,%rsi
  4b5083:	e9 7f fc ff ff       	jmp    4b4d07 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x97>
  4b5088:	0f 1f 84 00 00 00 00 	nopl   0x0(%rax,%rax,1)
  4b508f:	00 
  4b5090:	4b 8d 3c 0c          	lea    (%r12,%r9,1),%rdi
  4b5094:	49 83 e9 20          	sub    $0x20,%r9
  4b5098:	48 c1 e3 05          	shl    $0x5,%rbx
  4b509c:	48 89 f2             	mov    %rsi,%rdx
  4b509f:	49 c1 e9 05          	shr    $0x5,%r9
  4b50a3:	4c 89 e1             	mov    %r12,%rcx
  4b50a6:	49 83 c1 01          	add    $0x1,%r9
  4b50aa:	41 83 e1 07          	and    $0x7,%r9d
  4b50ae:	0f 84 a4 00 00 00    	je     4b5158 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x4e8>
  4b50b4:	49 83 f9 01          	cmp    $0x1,%r9
  4b50b8:	0f 84 81 00 00 00    	je     4b513f <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x4cf>
  4b50be:	49 83 f9 02          	cmp    $0x2,%r9
  4b50c2:	74 6b                	je     4b512f <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x4bf>
  4b50c4:	49 83 f9 03          	cmp    $0x3,%r9
  4b50c8:	74 55                	je     4b511f <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x4af>
  4b50ca:	49 83 f9 04          	cmp    $0x4,%r9
  4b50ce:	74 3f                	je     4b510f <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x49f>
  4b50d0:	49 83 f9 05          	cmp    $0x5,%r9
  4b50d4:	74 29                	je     4b50ff <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x48f>
  4b50d6:	49 83 f9 06          	cmp    $0x6,%r9
  4b50da:	74 13                	je     4b50ef <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x47f>
  4b50dc:	c5 fe 6f 06          	vmovdqu (%rsi),%ymm0
  4b50e0:	48 8d 14 1e          	lea    (%rsi,%rbx,1),%rdx
  4b50e4:	49 8d 4c 24 20       	lea    0x20(%r12),%rcx
  4b50e9:	c4 c1 7e 7f 04 24    	vmovdqu %ymm0,(%r12)
  4b50ef:	c5 fe 6f 0a          	vmovdqu (%rdx),%ymm1
  4b50f3:	48 83 c1 20          	add    $0x20,%rcx
  4b50f7:	48 01 da             	add    %rbx,%rdx
  4b50fa:	c5 fe 7f 49 e0       	vmovdqu %ymm1,-0x20(%rcx)
  4b50ff:	c5 fe 6f 12          	vmovdqu (%rdx),%ymm2
  4b5103:	48 83 c1 20          	add    $0x20,%rcx
  4b5107:	48 01 da             	add    %rbx,%rdx
  4b510a:	c5 fe 7f 51 e0       	vmovdqu %ymm2,-0x20(%rcx)
  4b510f:	c5 fe 6f 1a          	vmovdqu (%rdx),%ymm3
  4b5113:	48 83 c1 20          	add    $0x20,%rcx
  4b5117:	48 01 da             	add    %rbx,%rdx
  4b511a:	c5 fe 7f 59 e0       	vmovdqu %ymm3,-0x20(%rcx)
  4b511f:	c5 fe 6f 22          	vmovdqu (%rdx),%ymm4
  4b5123:	48 83 c1 20          	add    $0x20,%rcx
  4b5127:	48 01 da             	add    %rbx,%rdx
  4b512a:	c5 fe 7f 61 e0       	vmovdqu %ymm4,-0x20(%rcx)
  4b512f:	c5 fe 6f 2a          	vmovdqu (%rdx),%ymm5
  4b5133:	48 83 c1 20          	add    $0x20,%rcx
  4b5137:	48 01 da             	add    %rbx,%rdx
  4b513a:	c5 fe 7f 69 e0       	vmovdqu %ymm5,-0x20(%rcx)
  4b513f:	c5 fe 6f 32          	vmovdqu (%rdx),%ymm6
  4b5143:	48 83 c1 20          	add    $0x20,%rcx
  4b5147:	48 01 da             	add    %rbx,%rdx
  4b514a:	c5 fe 7f 71 e0       	vmovdqu %ymm6,-0x20(%rcx)
  4b514f:	48 39 f9             	cmp    %rdi,%rcx
  4b5152:	0f 84 ce fe ff ff    	je     4b5026 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x3b6>
  4b5158:	c5 fe 6f 3a          	vmovdqu (%rdx),%ymm7
  4b515c:	48 01 da             	add    %rbx,%rdx
  4b515f:	48 81 c1 00 01 00 00 	add    $0x100,%rcx
  4b5166:	c5 fe 7f b9 00 ff ff 	vmovdqu %ymm7,-0x100(%rcx)
  4b516d:	ff 
  4b516e:	c5 7e 6f 02          	vmovdqu (%rdx),%ymm8
  4b5172:	48 01 da             	add    %rbx,%rdx
  4b5175:	c5 7e 7f 81 20 ff ff 	vmovdqu %ymm8,-0xe0(%rcx)
  4b517c:	ff 
  4b517d:	c5 7e 6f 0a          	vmovdqu (%rdx),%ymm9
  4b5181:	48 01 da             	add    %rbx,%rdx
  4b5184:	c5 7e 7f 89 40 ff ff 	vmovdqu %ymm9,-0xc0(%rcx)
  4b518b:	ff 
  4b518c:	c5 7e 6f 12          	vmovdqu (%rdx),%ymm10
  4b5190:	48 01 da             	add    %rbx,%rdx
  4b5193:	c5 7e 7f 91 60 ff ff 	vmovdqu %ymm10,-0xa0(%rcx)
  4b519a:	ff 
  4b519b:	c5 7e 6f 1a          	vmovdqu (%rdx),%ymm11
  4b519f:	48 01 da             	add    %rbx,%rdx
  4b51a2:	c5 7e 7f 59 80       	vmovdqu %ymm11,-0x80(%rcx)
  4b51a7:	c5 7e 6f 22          	vmovdqu (%rdx),%ymm12
  4b51ab:	48 01 da             	add    %rbx,%rdx
  4b51ae:	c5 7e 7f 61 a0       	vmovdqu %ymm12,-0x60(%rcx)
  4b51b3:	c5 7e 6f 2a          	vmovdqu (%rdx),%ymm13
  4b51b7:	48 01 da             	add    %rbx,%rdx
  4b51ba:	c5 7e 7f 69 c0       	vmovdqu %ymm13,-0x40(%rcx)
  4b51bf:	c5 7e 6f 32          	vmovdqu (%rdx),%ymm14
  4b51c3:	48 01 da             	add    %rbx,%rdx
  4b51c6:	c5 7e 7f 71 e0       	vmovdqu %ymm14,-0x20(%rcx)
  4b51cb:	48 39 f9             	cmp    %rdi,%rcx
  4b51ce:	75 88                	jne    4b5158 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x4e8>
  4b51d0:	e9 51 fe ff ff       	jmp    4b5026 <void micron::math::fft::__impl::__engine<long double>::__smooth_stage<false>(micron::math::complex<long double> const*, micron::math::complex<long double>*, unsigned long, unsigned long) const+0x3b6>
  4b51d5:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
  4b51dc:	00 00 00 
  4b51df:	90                   	nop

