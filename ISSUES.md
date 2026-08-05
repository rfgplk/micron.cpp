# Known Issues (as of 2026-08-04)

## Hashing

- **An SSE2 build and an AVX2 build do not produce the same hash values.** `default_hash_128`/`_64` are
  `zzz128`/`zzz` where zzz exists and fall back to `murmur128`/`rapidhash` below AVX2. Fine for
  in-memory containers; **not** for anything persisted or sent over a wire. `-DMICRON_NO_ZZZ_HASH`
  forces the ISA-free defaults everywhere so the values agree across tiers.
- `hashes::xxhash64` accepts unaligned `src` (fixed 2026-08-04). It used to throw `library_error`;
  it now goes through the `__load32`/`__load64` memcpy punning helpers in `hash/__load.hpp` like the
  other kernels. Byte-identical output — the `tests/hash/hash_vectors` known-answer vectors are the gate.

## Known-failing tests

`tests/rigor/FAILING.md` is the current baseline of rigor tests that fail on a stock amd64 hosted
run

## Building / optimizations
- currently, you _cannot_ use micron alongside the STL (or any glibc) code; technically you can (if you poison the right headers and work around defines) but if you try you will _almost certainly_ run into conflicting type declarations. If you truly want to include micron code alongside glibc (say in a legacy codebase) my recommendation is to splice the micron code/headers you want verbatim rather than pulling in the whole thing. Most micron external fns map cleanly to glibc aliases, so you shouldn't have much trouble.
- under `-Ofast`/`-ffast-math` + LTO, `micron::numeric_limits<F>::max()` / `-max()` / `infinity()` can constant-fold to 0/-0 when used as a sentinel
- `[[gnu::flatten]]` transitively inlines all fns and blows up LTO compile time
- the `-flto` flag is still mandatory under ASan testing
- **AddressSanitizer does not report in a TU that pulls `src/std.hpp`**: abcmallocs `operator new` 
  wins over ASans
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
- format: `to_double` (`src/string/format.hpp`) is a naive accumulate-and-divide parser — not
  correctly rounded, and the container overload ignores `e` exponents entirely. It cannot parse
  shortest-form converter output back bit-exact
- format: `d2f_buffered`/`d2e_buffered` (`to_fixed`/`to_scientific`) truncate the Ryu digit stream
  at the precision cut instead of rounding the last kept digit.
- io: cached `st_size` not invalidated after writes
- io: termios struct layout is kernel-ABI dependent; not fully cross platform
- io: global stdout/stderr buffers have no thread-safety locks whatsoever
- io: dir-open does exists/is_dir checks before `open(O_DIRECTORY)`
- io arm64: `io::path`/`io::dir` directory-open throws `io_error` under qemu-aarch64
- LSan (and other sanitizers) may report a benign ~8 KB "leak" from the `make_global` stdout/stderr stream process-lifetime allocs
- `tests/coro/t_aio_inline` is **load-sensitive and flaky in batch runs**
- 32-bit + threads: a rigor test that keeps **8 live `auto_thread`s** throws `critical_error` from
  `operator new` on `--i386`. VA exhaustion, 64-bit is immune. **Likely mitigated 2026-08-04**
- width-32 abcmalloc: the buddy needs a sheet strictly exceeding `2 * n` to serve `n`, so a request
  at `__alloc_limit` (64 MB) carves 128 MB + 256 KB from a 1 GB VA reservation, roughly 7 such
  sheets before `__va_carve` falls through to a plain `sys_allocator` mmap. Over `__alloc_limit` is
  a hard `abort_state()` (`sys_exit(11)`), not a `nullptr`, and it bypasses the io flush, so a
  buffered `println` trail vanishes with it.
- freestanding: a `thread_local`'s destructor drops silently once a thread holds more than
  `MICRON_TDTOR_CAP` (default 128) of them. The Itanium ABI gives the construction site no way to
  react to a failed `__cxa_thread_atexit`, so the registration is counted in
  `micron::__tdtor_dropped` and otherwise lost. Raise the cap if you need more; it costs
  `cap * 2 * sizeof(void*)` of `.tbss` per thread.
- freestanding: nothing joins `__global_threadpool` at process exit, so a pool worker's
  `thread_local`s are never destroyed

## Platform / Arch gaps
- `src/simd/strings.hpp` is x86-only (AVX2/SSE2 + scalar fallback); no NEON yet
- `src/simd/fma.hpp` is x86-only (`_mm_fmadd_*` / `_mm256_fmadd_*`)
- `src/math/simd/trig.hpp` + `src/math/quaternions/batched.hpp`: NEON f32 on ARM, but f64 only on amd64/arm64; arm32 double-precision trig/quaternion falls back to scalar
- arm32: reading CNTVCT (`mrrc p15,1,…c14`) faults SIGILL under qemu/PL0
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
- aarch64 gcc **ignores** `__attribute__((naked))` entirely and emits a prologue/epilogue anyway; both
  `__ar.hpp` and `clone.hpp` work around it by emitting the routines as file-scope `asm()` blocks
- **`-Ofast` (`-fno-signed-zeros`) merges calls fed compile-time `+0.0` / `-0.0` constants** — the
  linaro aarch64 build CSEd `double_to_string(0.0)` and `double_to_string(-0.0)` into one result, so a
  test comparing both signs saw the wrong string. Runtime values are unaffected (the converters read
  raw bits). In tests, launder constant ±0 bit patterns through a `volatile` u64/u32 first
  (`tests/rigor/rigor_format_ryu.cpp` `f64_opaque`).
