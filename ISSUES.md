# Known Issues (as of 2026-06-01)

## Building / optimizations
- under `-Ofast`/`-ffast-math` + LTO, `micron::numeric_limits<F>::max()` / `-max()` / `infinity()` can constant-fold to 0/-0 when used as a sentinel
- `[[gnu::flatten]]` transitively inlines all fns and blows up LTO compile time
- the `-flto` flag is still mandatory under ASan testing
- certain heavy abc tests need `vm.overcommit_memory=0|1` otherwise they'll fail at RUNTIME with `critical_error` (mmap refused)

## Bugs / Limitations you should know
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
- `src/math/simd/trig.hpp` + `src/math/quaternions/batched.hpp`: NEON f32 on ARM, but f64 only on amd64/arm64; arm32 double-precision trig/quaternion falls back to scalar
- arm32: reading CNTVCT (`mrrc p15,1,…c14`) faults SIGILL under qemu/PL0
