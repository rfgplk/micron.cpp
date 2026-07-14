# Known Issues (as of 2026-06-04)

## x86 ISA levels

micron's x86 floor is **SSE2**. `-mavx2 -mbmi` are no longer required: every 256-bit path is gated on `__micron_x86_avx2`, which gcc defines from `-march`, and
degrades to two 16-byte SSE2 halves below it. Tier picking is natively supported via `duck --isa`:

| `--isa` | `-march=` | gives you | oldest core |
|---|---|---|---|
| `base` | `x86-64` | SSE2 | any x86-64 (2003+) |
| `v2` | `x86-64-v2` | +SSE3..SSE4.2, POPCNT | Nehalem (2008+) |
| `v3` | `x86-64-v3` | +AVX, AVX2, BMI1/2, FMA | Haswell (2013+) |
| `v4` | `x86-64-v4` | +AVX-512 | Skylake-X (2017+) |
| `native` (default) | `native` | this box | — |

The invariant: **no function emits an instruction its build flags did not authorize.** An `--isa base`
object contains zero AVX/AVX2/AVX-512/BMI/SSE4 instructions, so it really does run on a pre-AVX2 core.

Things that are ISA-gated *by nature* and are simply absent below their tier:
- `simd::w*` / `simd::z*` (the v256/v512 **class** types): a 256-bit vector has no narrower form.
  Need AVX2 / AVX-512F.
- `simd::sse::*` carries a per-tier `gnu::target`: the ~230 SSE2 wrappers stay callable at `base`,
  while the SSE3/SSSE3/SSE4.1/SSE4.2 ones demand their ISA.
- `micron::math::mk`'s **packed** overloads (`sin(V)`, `exp(V)`, ...) need AVX2+FMA (or NEON). The scalar
  `mk::` surface is unaffected and works at every ISA. Gated on: `__micron_math_packed`.
  Note `mk::packed_real` still admits `f128`/`d128` at SSE2 while no kernels cover them, most of those
  kernels are *already* 128-bit code merely trapped behind the AVX2 gate.
- `hashes::zzz*` is AVX2-specific on x86 (NEON on arm), it is written directly on 256-bit intrinsics, no fallback.

## Hashing

- **An SSE2 build and an AVX2 build do not produce the same hash values.** `default_hash_128`/`_64` are
  `zzz128`/`zzz` where zzz exists and fall back to `murmur128`/`rapidhash` below AVX2. Fine for
  in-memory containers; **not** for anything persisted or sent over a wire. `-DMICRON_NO_ZZZ_HASH`
  forces the ISA-free defaults everywhere so the values agree across tiers.
- **`hashes::xxhash64` requires an aligned `src` and throws `library_error` otherwise** (`xx.hpp:175`).

## Building / optimizations
- currently, you _cannot_ use micron alongside the STL (or any glibc) code; technically you can (if you poison the right headers and work around defines) but if you try you will _almost certainly_ run into conflicting type declarations. If you truly want to include micron code alongside glibc (say in a legacy codebase) my recommendation is to splice the micron code/headers you want verbatim rather than pulling in the whole thing. Most micron external fns map cleanly to glibc aliases, so you shouldn't have much trouble.
- under `-Ofast`/`-ffast-math` + LTO, `micron::numeric_limits<F>::max()` / `-max()` / `infinity()` can constant-fold to 0/-0 when used as a sentinel
- `[[gnu::flatten]]` transitively inlines all fns and blows up LTO compile time
- the `-flto` flag is still mandatory under ASan testing
- certain heavy abc tests need `vm.overcommit_memory=0|1` otherwise they'll fail at RUNTIME with `critical_error` (mmap refused)

## Bugs / Limitations you should know
- `simd::memcpy512` / `memset512` / `memcmp512` etc. remain **loaded**: they carry
  `target("avx512f")`, are ungated, and are exported through `cmemory.hpp`, so calling one on a
  non-AVX-512 CPU is a SIGILL.
- `src/simd/arch/{load,shift}_amd64.hpp` are not self-contained, including `simd/load.hpp` or
  `simd/shift.hpp` standalone fails on an include-order cycle.
- `src/regex/regex.hpp`: large bounded intervals (`a{50,99}`) overflow, falsly triggers no match
- `src/regex/regex.hpp`: `cmatch<>` local scratch is O(maxi*maxslots) (~27KB stack for nested-group patterns); _very_ heavy comptime for large patterns
- base move-assign does NOT free the destination's old buffer: `src/memory/allocation/core_resource.hpp` (`operator=(&&)`) delegate without `free()`; `resource_types/mutable_resource.hpp` copy-assign likewise. Microns containers free themselves by design, but be careful
- `micron::alloc<T>(bytes)` returns RAW memory; doesn't perform default init
- io: cached `st_size` not invalidated after writes
- io: termios struct layout is kernel-ABI dependent; not fully cross platform
- io: global stdout/stderr buffers have no thread-safety locks whatsoever
- io: dir-open does exists/is_dir checks before `open(O_DIRECTORY)`
- io arm64: `io::path`/`io::dir` directory-open throws `io_error` under qemu-aarch64
- LSan (and other sanitizers) may report a benign ~8 KB "leak" from the `make_global` stdout/stderr stream process-lifetime allocs

## Platform / Arch gaps
- `src/simd/strings.hpp` is x86-only (AVX2/SSE2 + scalar fallback); no NEON yet
- `src/simd/fma.hpp` is x86-only (`_mm_fmadd_*` / `_mm256_fmadd_*`)
- `src/math/simd/trig.hpp` + `src/math/quaternions/batched.hpp`: NEON f32 on ARM, but f64 only on amd64/arm64; arm32 double-precision trig/quaternion falls back to scalar
- arm32: reading CNTVCT (`mrrc p15,1,…c14`) faults SIGILL under qemu/PL0
- arm32: `tests/coro/t_parallel_{compact,scan}` abort with `except::memory_error_abc_dealloc_size` (abcmalloc's 32-bit `__ABC_EMBED` profile); unrelated to the coroutine runtime, which passes
- arm32: `tests/coro/t_parallel_{map,quick,radix,sort}` fail to compile against the Linaro sysroot — `conflicting declaration 'typedef __time_t time_t'` / `suseconds_t` between micron's typedefs and glibc's `bits/types/time_t.h`

## Compiler hazards
- **`__attribute__((naked))` does NOT suppress the stack-protector prologue.** Under `-fstack-protector-all`
  gcc prepends the canary spill *inside* a naked function:

  ```
  ldr r3, [pc, #..]   ; &__stack_chk_guard
  ldr r3, [r3]
  str r3, [sp, #4]    ; <-- ABOVE sp; a naked fn reserved no frame
  <the naked body>
  ```

  On x86 gcc emits no canary for naked functions (and the store would land in the 128-byte red zone
  anyway), which is why amd64 never noticed while every fiber switch and every `clone3` spawn silently
  corrupted memory on arm32. **Every naked function must carry `__micron_no_ssp`** (`src/bits/__arch.hpp`).
  Current users: `src/bits/__ar.hpp`, `src/linux/sys/clone.hpp`, `src/linux/sys/signal.hpp` (via
  `naked_fn` in `src/attributes.hpp`), `src/memory/allocation/abcmalloc/doctor.hpp`.
  `start/start.cpp` is exempt only because freestanding builds already pass `-fno-stack-protector`.
- aarch64 gcc **ignores** `__attribute__((naked))` entirely and emits a prologue/epilogue anyway; both
  `__ar.hpp` and `clone.hpp` work around it by emitting the routines as file-scope `asm()` blocks
