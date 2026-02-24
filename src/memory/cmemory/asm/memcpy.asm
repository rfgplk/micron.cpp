default rel
section .text

%include "memcpy.inc"

; ============================================================
; ABI selection
; ============================================================

%ifdef WINDOWS
    ; Windows x64:
    ; rcx = dst
    ; rdx = src
    ; r8  = size
    %define ARG_DST rcx
    %define ARG_SRC rdx
    %define ARG_SIZ r8
%else
    ; System V:
    ; rdi = dst
    ; rsi = src
    ; rdx = size
    %define ARG_DST rdi
    %define ARG_SRC rsi
    %define ARG_SIZ rdx
%endif


; ============================================================
; ================   SSE 16  ================================
; ============================================================

global small_memcpy_sse_16
small_memcpy_sse_16:
    memcpy_sse_16 ARG_DST, ARG_SRC, ARG_SIZ, rax, rcx
    ret

global small_memcpy_sse_16_1
small_memcpy_sse_16_1:
    memcpy_sse_16_1 ARG_DST, ARG_SRC, ARG_SIZ, rax, rcx
    ret


; ============================================================
; ================   SSE 64  ================================
; ============================================================

global small_memcpy_sse_64
small_memcpy_sse_64:
    memcpy_sse_64_ret ARG_DST, ARG_SRC, ARG_SIZ, rax, rcx, xmm0, xmm1, xmm2, xmm3

global small_memcpy_sse_64_1
small_memcpy_sse_64_1:
    memcpy_sse_64_1_ret ARG_DST, ARG_SRC, ARG_SIZ, rax, rcx, xmm0, xmm1, xmm2, xmm3


; ============================================================
; ================   SSE 128  ===============================
; ============================================================

global small_memcpy_sse_128
small_memcpy_sse_128:
    memcpy_sse_128_ret ARG_DST, ARG_SRC, ARG_SIZ, rax, rcx, xmm0, xmm1, xmm2, xmm3

global small_memcpy_sse_128_1
small_memcpy_sse_128_1:
    memcpy_sse_128_1_ret ARG_DST, ARG_SRC, ARG_SIZ, rax, rcx, xmm0, xmm1, xmm2, xmm3


; ============================================================
; ================   AVX (256-bit loads not used) ============
; ============================================================

global small_memcpy_avx_16
small_memcpy_avx_16:
    memcpy_avx_16 ARG_DST, ARG_SRC, ARG_SIZ, rax, rcx
    ret

global small_memcpy_avx_16_1
small_memcpy_avx_16_1:
    memcpy_avx_16_1 ARG_DST, ARG_SRC, ARG_SIZ, rax, rcx
    ret

global small_memcpy_avx_64
small_memcpy_avx_64:
    memcpy_avx_64_ret ARG_DST, ARG_SRC, ARG_SIZ, rax, rcx, xmm0, xmm1, xmm2, xmm3

global small_memcpy_avx_64_1
small_memcpy_avx_64_1:
    memcpy_avx_64_1_ret ARG_DST, ARG_SRC, ARG_SIZ, rax, rcx, xmm0, xmm1, xmm2, xmm3

global small_memcpy_avx_128
small_memcpy_avx_128:
    memcpy_avx_128_ret ARG_DST, ARG_SRC, ARG_SIZ, rax, rcx, xmm0, xmm1, xmm2, xmm3

global small_memcpy_avx_128_1
small_memcpy_avx_128_1:
    memcpy_avx_128_1_ret ARG_DST, ARG_SRC, ARG_SIZ, rax, rcx, xmm0, xmm1, xmm2, xmm3


; ============================================================
; ================   AVX2  ==================================
; ============================================================

global small_memcpy_avx2_64
small_memcpy_avx2_64:
    memcpy_avx2_64_ret ARG_DST, ARG_SRC, ARG_SIZ, rax, rcx, ymm0, ymm1

global small_memcpy_avx2_64_1
small_memcpy_avx2_64_1:
    memcpy_avx2_64_1_ret ARG_DST, ARG_SRC, ARG_SIZ, rax, rcx, ymm0, ymm1

global small_memcpy_avx2_128
small_memcpy_avx2_128:
    memcpy_avx2_128_ret ARG_DST, ARG_SRC, ARG_SIZ, rax, rcx, ymm0, ymm1, ymm2, ymm3

global small_memcpy_avx2_128_1
small_memcpy_avx2_128_1:
    memcpy_avx2_128_1_ret ARG_DST, ARG_SRC, ARG_SIZ, rax, rcx, ymm0, ymm1, ymm2, ymm3
