# Known Issues (as of 2026-08-14)

## Hashing

- **An SSE2 build and an AVX2 build do not produce the same hash values.** `default_hash_128`/`_64` are
  `zzz128`/`zzz` where zzz exists and fall back to `murmur128`/`rapidhash` below AVX2. Fine for
  in-memory containers; **not** for anything persisted or sent over a wire. `-DMICRON_NO_ZZZ_HASH`
  forces the ISA-free defaults everywhere so the values agree across tiers.
- `hashes::xxhash64` accepts unaligned `src` (fixed 2026-08-04). It used to throw `library_error`;
  it now goes through the `__load32`/`__load64` memcpy punning helpers in `hash/__load.hpp` like the
  other kernels. Byte-identical output — the `tests/hash/hash_vectors` known-answer vectors are the gate.

## Containers

  **convector -- locks, then leaks the handle out** in `T &operator[]`, `at`, `front`, `back`, both
  `slice<T> operator[]` forms and `into_bytes()`. `at_n(iterator)`

- **`convector`'s `fast_mutex` shares a cache line with the metadata it guards**
- **`ivector::insert()` cannot insert at the end.**
- **`clear()` is O(n) even for a trivially destructible element type, by design.**

## Known-failing tests

- **An ASAN-detected error grades as PASS.** AddressSanitizer's default error exit code is `1`, and `1` is exactly snowball's success sentinel (`tools/src/recipes/gnu/qemu.hh`)

`tests/rigor/FAILING.md` is the current baseline of rigor tests that fail on a stock amd64 hosted
run

- `tests/rigor/memcmp.cpp` and `tests/rigor/memory.cpp` do not link
- **`tests/rigor/rigor_algo_core_imperative.cpp` returns 6 FAIL (require) on `--arm`**, in the

## Building / optimizations
- micron and libstdc++/glibc **can** coexist in one TU, but it isn't recommended. You may hit conflict issues, although most have been pruned
- `src/__special/compare` hosted build uses libstdc++'s real `<compare>` rather than declaring the comparison categories itself. `<compare>` is not a leaf; libstdc++'s `<bits/stl_iterator.h>` uses it for `three_way_comparable_with`; freestanding self-hosts
- under `-Ofast`/`-ffast-math` + LTO, `micron::numeric_limits<F>::max()` / `-max()` / `infinity()` can constant-fold to 0/-0 when used as a sentinel
- `[[gnu::flatten]]` transitively inlines all fns and blows up LTO compile time
- the `-flto` flag is still mandatory under ASan testing
- `-DMICRON_ABCMALLOC_DISABLE_STD` **does not build**
- **ASan still cannot see container out-of-bounds**, only double-free/UAF
  `allocator_types/serial_allocator.hpp:32` rounds every request through
  `to_granularity<page_size>` (`policies.hpp:27`), containers issue `malloc(4096)` and an overflow past the logical end never reaches a redzone. Needs either a
  sanitizer-time granularity of 1 or `__asan_poison_memory_region`
- **`--tsan` / `--asan` cannot compile ANY TU that reaches `src/thread/thread.hpp`** — 26
- **`duck` rejects a multi-token flag string passed as one argv entry**
- certain heavy abc tests need `vm.overcommit_memory=0|1` otherwise they'll fail at RUNTIME with `critical_error` (mmap refused)

## Bugs / Limitations you should know
- **`external/bbench` can only be built against the working tree with `-i .`.**
- **`micron::buffer` is accepted by `io::coro::read_file` but cannot be grown
- **`io::printk` cannot print a pointer-to-volatile.**
- **`micron::hashes::z64` and `zz64` are weak. Neither is a default.**
- **`duck --clang` is broken on anything that pulls in AVX**
- **micron is single-TU BY DESIGN.**
- **`hopscotch_map::begin()`/`end()` iterate the raw slot array**
- **`micron::list` / `micron::doublelist` `begin()`/`end()` are not an iteration pair.**
- **`v256<T,F>::shuffle()` (`simd/types/simd256.hpp:1678`) uses an unguarded PSHUFB**
- **`v128<T,F>::shuffle()` returns an uninitialised `v128` for every `T` other than `i128`.**
- **`micron::function`'s move ctor / move-assign / `swap` are unconditionally `noexcept`**
- **`micron::function<void(Args...)>` rejects a target that returns non-`void`.**
- **`micron::lazy` rejects lvalues, and `__impl::__thunk` is not assignable.**
- **`src/closures.hpp` closes `namespace micron` at line 22**
- **`micron::function::target<G>()` tags types by the address of a function-local `static const int`**
- `src/simd/arch/{load,shift}_amd64.hpp` are not self-contained, including `simd/load.hpp` or
  `simd/shift.hpp` standalone fails on an include-order cycle.
- `src/regex/regex.hpp`: large bounded intervals (`a{50,99}`) overflow, falsly triggers no match
- `src/regex/regex.hpp`: `cmatch<>` local scratch is O(maxi*maxslots) (~27KB stack for nested-group patterns); _very_ heavy comptime for large patterns
- base move-assign does NOT free the destination's old buffer: `src/memory/allocation/core_resource.hpp` (`operator=(&&)`) delegate without `free()`; `resource_types/mutable_resource.hpp` copy-assign likewise. Microns containers free themselves by design, but be careful
- `micron::alloc<T>(bytes)` returns RAW memory; doesn't perform default init
- **`MICRON_ABCMALLOC_STD` is ON by default**
  **`from_chars` is strict and consumes the whole range, no leading or trailing whitespace**
- **there are now two `%g` semantics in the (d2g_buffered) and (to_general), they don't yield the same results** 
- io: cached `st_size` not invalidated after writes
- io: termios struct layout is kernel-ABI dependent; not fully cross platform
- io: global stdout/stderr buffers have no thread-safety locks whatsoever
- io: dir-open does exists/is_dir checks before `open(O_DIRECTORY)`
- io arm64: `io::path`/`io::dir` directory-open throws `io_error` under qemu-aarch64
- LSan (and other sanitizers) may report a benign ~8 KB "leak" from the `make_global` stdout/stderr stream process-lifetime allocs
- `tests/coro/t_aio_inline` is **load-sensitive and flaky in batch runs**
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

## Concurrency / locking

  - **try_lock() can answer false for the few instructions between the
  lock going free and the release letting go of the node** (measured 41 and 112 refusals in 10000
  rounds, 0 in three other runs)
- **Exceeding `MICRON_MCS_DEPTH` is loud through `lock()` and a silent unbounded livelock through
  every `try_lock`-based path.**
- **`mcs_lock` detects re-entrancy and raises; `clh_lock` cannot and self-deadlocks.** `try_lock()`
- **`shared_mutex` has no shared→exclusive upgrade, and attempting one blocks the whole lock**
- **`recursive_lock::unlock()` from a non-owner is a silent no-op.**
- **`~mcs_lock()` clears only the destroying thread's slot entry**
- `mutex/rcu.hpp` is entirely `#if 0` (`WARNING: DO NOT USE, NON FUNCTIONAL + BROKEN`);
- **The FIFO locks convoy under preemption, and `mcs_lock` worst of the three.**
- gcc emits a `-Warray-bounds` false positive for `lock_guard<M>::~lock_guard`'s member-function-pointer call when `M` is a one-byte lock with an empty `[[no_unique_address]]` member

## Platform / Arch gaps
- `src/simd/fma.hpp` is x86-only (`_mm_fmadd_*` / `_mm256_fmadd_*`)
- `src/math/simd/trig.hpp` + `src/math/quaternions/batched.hpp`: NEON f32 on ARM, but f64 only on amd64/arm64; arm32 double-precision trig/quaternion falls back to scalar

## Math (vectors / matrices)

- **The matrix move ctor/assign zero the source** (`matrix/bits.hpp`); deliberate
- **`mat<T,R,C> a * b` is element-wise, not a matrix product**; deliberate
- **`int_matrix_base_avx`'s variadic ctor is unconstrained on argument type.**

## Algorithms

<<<<<<< HEAD
- **Map and tree `any_of` / `find` / `find_if` / `contains_if` never early-exit.**
=======
- **Several graph names are still delegated conveniences.**
  `boykov_kolmogorov` calls Dinic; `king_ordering` and `sloan_ordering` call reverse Cuthill-McKee;
  `minimum_degree_ordering` calls smallest-last. The community-family implementations remain the existing
  simplified variants
- **Map `any_of` / `find` / `find_if` / `contains_if` never early-exit.**
>>>>>>> 0584710 (* fix compile issues and libc interop)
- **`fp::nub_by` and `fp::unique(c, eq)` are O(n^2)**
- **`search` / `find_end` fall back to an O(nm) skip scan for a pattern wider than
  `__impl::kmp_stack_max` (256)**
- **The `_n` family in `algorithm/memory.hpp` does not have one count convention, and three of them
  change convention based on the value of the count.** `simd::memcpy256` / `memcmp256` / `memset256`
  all compute `bytes = count * sizeof(T)`, so their count is in **elements**; `micron::memset` and
  `bytecmp` take **bytes**. The dispatch in `memory.hpp` picks between them on `cnt % 32` / `cnt % 16`:
 - **`micron::memcmp` is a value comparison, not a byte comparison, for any element wider than a
  byte.**
- **The container overload of `merge()` does not merge; it concatenates instead.**
- **A NaN differential test is meaningless under `-Ofast`.**

- **`micron::swap` SILENTLY FAILS TO LINK FOR A MOVE-ONLY TYPE.**

## Compiler hazards
- **`#pragma GCC optimize("no-fast-math", …)` does not protect an `always_inline` body, and on x86 the flag that actually rewrites your divide is not an optimize option at all.** (actual wtf?)
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

## chrono

- **Timeout-carrying syscalls with no wrapper**, `semtimedop`, `mq_timedsend`/`mq_timedreceive`, `io_pgetevents`, `recvmmsg`, `pselect6`, and a
  *timed* `rt_sigtimedwait`
- **`i386: ftime = 35` is exposed as if usable.**
- **A dlopened module's C++ static destructors do not run.**
- **`handle_t::open_path` runs a module's initializers over `best_effort` relocations.**
  `__legacy_opts` (`src/linux/elf/elf.hpp:120`) is `{ best_effort, run_init = true, ... }` and
  `__load_module_from_path` does *not* load `DT_NEEDED`. So opening any module with an import that
  `__resolve_across_loaded` cannot satisfy leaves the slot unbound and then calls the constructor
  through it -- `handle_t::open_path("/usr/lib64/libibverbs/librxe-rdmav59.so")` SIGSEGVs the host
  process. Reproduced against unmodified `src/`; unrelated to `count_dynsyms`.
- **`micron::dso`/`elf::handle_t` are best-effort.**
- **`dynamic_error()` is a fixed 192-byte thread_local buffer.**
- **`dl_iterate_phdr` reports host modules with a null `dlpi_phdr`.**
- **micron's `r_debug` chain is its own, and a hosted gdb will not find it.**
- **The host-module snapshot is invalidated entirely on every unmap.**
- **Only executable mappings become host modules.**
- **`resolve_dependency` is guarded via `AT_SECURE`.**
- **Static TLS relocations are refused** -- and `unsupported` is fatal in *every* mode.
- **The `*_name` tables have no entry for several things for now.**
- **`micron::user_hz` is a hardcoded `constexpr 100`** (`linux/process/resource.hpp:362`).
- **`posix::sysconf` has only `_SC_CLK_TCK` and `_SC_PAGESIZE` implemented**
- **The vDSO fast path resolves nothing under qemu-user.**
- **`chrono::core_hz()` is only as stable as the rig.**
- **armv7-a reads no cycle counter by default.**
- **`chrono/tz.hpp` reads TZif v2/v3 only.**
- **Offsets render to minute resolution.**
