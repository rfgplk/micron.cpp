//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#if !defined(__clang__)
#error "micron: the native WebAssembly backend requires Clang"
#endif
#if defined(__wasi__) || defined(__EMSCRIPTEN__)
#error "micron: use the native wasm32-unknown-unknown target"
#endif
#if defined(MICRON_PORT_KERNEL) || defined(MICRON_PORT_METAL) || defined(MICRON_ENABLE_ATTACH) || defined(MICRON_ATTACH_MODULE)
#error "micron: native WebAssembly cannot use a Linux, metal or ELF module runtime"
#endif
#if defined(__FAST_MATH__)
#error "micron: the WebAssembly numeric backend requires strict floating-point compilation"
#endif

#if defined(__wasm_simd128__) && !defined(MICRON_NO_SIMD)
#define __micron_wasm_simd128 1
#define __micron_simd_width 128
#else
#define __micron_simd_generic 1
#define __micron_simd_width 0
#endif
#define __micron_simd_wide_composite 1

#if defined(__wasm_atomics__)
#define __micron_wasm_atomics 1
#endif
#if defined(__wasm_bulk_memory__)
#define __micron_wasm_bulk_memory 1
#endif
#if defined(MICRON_WASM_THREADS)
#if !defined(__micron_wasm_atomics) || !defined(__micron_wasm_bulk_memory)
#error "micron: MICRON_WASM_THREADS requires -matomics -mbulk-memory and shared memory"
#endif
#if defined(MICRON_NO_TLS)
#error "micron: WebAssembly workers require thread-local storage"
#endif
#define __micron_wasm_threads 1
#endif

#if defined(__WASM_EXCEPTIONS__)
#define __micron_wasm_eh 1
#ifndef __micron_eh
#define __micron_eh 1
#endif
#elif defined(__micron_eh)
#error "micron: WebAssembly exceptions require -fwasm-exceptions"
#endif

#define __micron_wasm_abi_version 1
#define __micron_wasm_page_size 65536u
#define __micron_wasm_allocation_granularity 4096u
