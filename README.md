<img align="left" style="width:300px" src="https://github.com/user-attachments/assets/8d544599-d4a3-4b8a-a61a-b83eb2a30b64" alt="micron_logo_default" width="300"/> 

<div align="left">

### the micron core library 🦅 <img src="https://img.shields.io/badge/indev-green">

#### a core library implementation (& redesign) of libc and the C++ Standard Library

**micron** is a comprehensive core library; a collection of algorithms, containers, iterators, functions, and OS interfaces; a header-only core system library written in c++23 targeting the Linux syscall API.
Unlike library collections such as Boost et al., *micron* does not intend to merely *augment* the STL, but entirely replace it.

</div>

[![Linux](https://img.shields.io/badge/Linux-FCC624?logo=linux&logoColor=black)](#)
![Version](https://img.shields.io/badge/version-1.9.12.0-green)
[![License](https://img.shields.io/badge/License-Boost_1.0-lightblue.svg)](https://www.boost.org/LICENSE_1_0.txt)
[![C++23](https://img.shields.io/badge/C++-23-blue.svg)](https://en.cppreference.com/w/cpp/23)


------

<br/>

<br/>


> [!WARNING]
> micron is still in active development, the ABI may change at any point, and without notice.

#### Features
  - a *fully functional*, templated C++ standard library implementation, designed from the **ground up** with modern principles in mind
  - completely self-contained, self-hosted, and freestanding; with *no dependencies on external code whatsoever*; not even the traditional C standard library
  - algorithmic functions are designed around pure functional semantics where applicable
  - written entirely in c++23
  - a high performance, cache-aware algorithmic base architecture
  - provides an innovative foundation for systems-level development, reimagining conventional approaches to low-level programming


##### Using the Library

All necessary code is self-contained within the `src/` directory. Since *micron* is freestanding, it relies on no external sources; no other files or libraries are necessary. Simply include any header file you want into your project and compile. Multi-threaded code builds two ways: a **hosted** default that links the system `pthread`, and a from-scratch freestanding backend. See *Threading & Concurrency* below. For examples, check out the `examples/` directory.


##### Installation

First, clone the repository via ssh:

`git clone --depth=1 git@github.com:rfgplk/micron.cpp.git`

or https:

`git clone --depth=1 https://github.com/rfgplk/micron.cpp.git`


Below are the specific steps you need to take to properly set up micron for your desired target.

###### the Duck build tool

The authoritative build tool for micron (and micron powered projects) is `duck`. Duck itself is a no-build tool, inspired by Alexey Kutepov's (better known as Tsoding) nob idea. Duck does not read any build files (except for .duck batch command files), instead it orchestrates and marshalls compiler flags directly to the compiler.

To build it run:

`sh scripts/bootstap_duck.sh` 

or

`/usr/bin/g++ -std=c++26 -Ofast -fmodulo-sched -fmodulo-sched-allow-regmoves -fgcse-sm -fgcse-las -march=native -ffreestanding -nostdlib -nostdlib++ -fno-stack-protector -fexceptions -frtti -fasynchronous-unwind-tables -D__micron_eh -Wl,--eh-frame-hdr -m64 -Wall -Wextra -Wpedantic -Wno-variadic-macros -Wno-inline -fext-numeric-literals -Wno-odr -Wno-lto-type-mismatch -fdiagnostics-color=always -fconcepts-diagnostics-depth=2 tools/src/main.cc ./start/start.s ./start/start.cpp ./start/eh_runtime.cpp -I./src -L./libs/ -o bin/duck`

and then compile any one of our examples tests, ie:

`duck run tests/rigor/vector.cpp`

###### amd64 / x86_64 / i386 (x86)

The simplest, most straightforward installation; just copy all the files in `src/` and `external/` to either your desired location; or to the system header include directories `/usr/include/` or `/usr/local/include/`. Either use `cp -r`, `rsync`, or you can run `scripts/install_local.py` and `scripts/install_externals.py`, which will automatically copy all files to `/usr/include/micron` and `/usr/include/external` (NOTE: directories will be created if they don't exist).

###### AArch64 / armv7-a (ARM32)

The same exact steps as above. If you are cross compiling on amd64 for arm32 or aarch64, you should manually copy the source files to the include path of your cross compiler, which usually differs from system wide include paths. `scripts/install_local_linaro.py` will do that for you (if using the linaro toolchain on fedora), but exact paths may differ based on your configuration, so double check. Hint: `echo | /usr/gcc-linaro/bin/arm-none-linux-gnueabihf-c++ -E -Wp,-v -` will tell you which include directories the compiler uses. 

###### Freestanding builds
 
In order to compile micron binaries in freestanding mode (not linking against glibc or any system objects), you'll first need to run `scripts/install_start.py` which copies over all the `start/` files (containing _start and various other init code) to `/usr/src/mc_start`. Then you'll need to compile your binaries by providing the path to the start source files, example: 

Neither the path nor the `sudo` is mandatory: `install_start.py <dir>` takes a destination, and duck finds the crt via `--start <dir>`, else `$MICRON_START`, else `/usr/src/mc_start`. To build straight out of a clone with no install at all, use `duck build x.cpp -k --start ./start -i .` -- the `-i .` matters, because the crt sources pull `<micron/...>` off the include path and the repo-root `micron -> src` symlink is what resolves it in-tree.

```bash
/usr/bin/g++ -std=c++26 -Ofast -march=native -fmodulo-sched -fmodulo-sched-allow-regmoves -fgcse-sm -fgcse-las -ffreestanding -nostdlib -nostdlib++ -fno-stack-protector -fno-exceptions -fno-rtti -m64 -Wall -Wextra -Wpedantic -Wno-variadic-macros -Wno-inline -flto=8 -Wno-odr -Wno-lto-type-mismatch -Wno-variadic-macros -Wno-inline tools/src/main.cc /usr/src/mc_start/start.s /usr/src/mc_start/start.cpp -I./src -L./libs/ -o bin/duck
```

This installation guide serves only as a rough suggestion, exact paths may depend on your use case and configuration.

###### GCC and Clang

`duck` has separate GCC and Clang flag profiles. Select Clang with `--clang`; its optimization,
warning, LTO, freestanding, and cross-compilation flags are translated to Clang spellings.

```bash
CXX=clang++ sh scripts/bootstap_duck.sh
duck compile examples/ --clang -O2 -o bin/examples-clang
```

The complete compile matrices are compiler-specific. The Clang matrix covers amd64, i386, ARMv7,
AArch64, ISA tiers, optimization levels, hosted/freestanding builds, and real links; GCC-only C++26
reflection rows remain in the GCC matrix.

```bash
duck batch parallel verify_compile_gcc.duck
duck batch parallel verify_compile_clang.duck
```

###### x86 ISA levels

micron's x86 floor is **SSE2**, it runs on any amd64 CPU, back to 2003.
Tiers are natively dispatched via `duck --isa`:

```bash
duck compile src/ --x86 --isa base    # -march=x86-64      SSE2                  any x86-64 (2003+)
duck compile src/ --x86 --isa v2      # -march=x86-64-v2   +SSE4.2, POPCNT       Nehalem  (2008+)
duck compile src/ --x86 --isa v3      # -march=x86-64-v3   +AVX2, BMI1/2, FMA    Haswell  (2013+)
duck compile src/ --x86 --isa v4      # -march=x86-64-v4   +AVX-512              Skylake-X (2017+)
duck compile src/ --x86               # -march=native (default) -- whatever this box is
```

The invariant is that **no function emits an instruction its build flags did not authorize**, so an
`--isa base` binary contains no AVX/AVX2/BMI/SSE4 instructions at all and genuinely executes on a
pre-AVX2 core.

##### Philosophy

**All core library code adheres to the following design principles**:
- in all instances where functional equivalence exists between *micron* and the STL, or any third-party library, *micron* must demonstrate superior performance
- in all conceivable scenarios, this code grants the developer absolute control, both of execution and compilation
- functionality must be preserved with any arbitrary data type
- in all instances, *micron* must maintain seamless interoperability with the STL and any other library offering equivalent functionality
- in all cases, performance always takes precedence over safety, with the developer assuming full responsibility for code validity and security
- all functions follow a strict side effect free formulation (pure functions)

in short:
- the written code, in its explicit form, stands as the ultimate arbiter of truth, unyielding and devoid of ambiguity, embodying the essence of how code should perform.

###### Is micron entirely self-sufficient?

Yes, *micron* relies on no external code other than what is included in this repository — meaning as long as you have a working `g++/clang++` compiler, you can compile and run it anywhere. Threading used to be the one exception: it required linking `pthread`. **That is no longer strictly true.** On the `micron-thread-tls` branch, micron ships its own freestanding threading backend. The hosted default still uses `pthread` (and auto-links it).

###### Architecture Support

*micron* is built and tuned first for **x86_64 (amd64)**. Support for other CPU architectures is tiered as follows:

| Tier | Architectures | Status |
|------|---------------|--------|
| 🟢 **Full** | `amd64` / `x86_64`, `arm32` / `armv7-a` | Fully supported and tested. |
| 🟡 **Effective** | `arm64` / `aarch64`, `i386` / `x86` | Compiles properly, untested. (you may run into bugs!) |
| 🔵 **Future** | `RISC-V`, `POWER` (`ppc64`) | Planned in the future. No backend present today. |

> [!IMPORTANT]
> *micron* targets **Linux specifically**. It is built directly on Linux syscalls, ABI, and kernel conventions throughout; it is **NOT** a portable POSIX library. Some code may happen to build and run on other POSIX systems (the BSDs, macOS, etc.), but this is neither guaranteed nor supported. Linux is the only supported operating system. We will release a dedicated macOS version eventually.


##### Threading & Concurrency

micron's threading is **dual-backend**, selected at compile time via the `__micron_freestanding` macro. The *same source API* compiles against either:

- **Hosted (default)** Threads are backed by the system POSIX `pthread`.
- **Freestanding** A from-scratch backend built directly on the `clone3` syscall with micron's own per-thread thread-local storage.

##### Reflection (C++26)

micron implements the C++26 static reflection facility ([P2996]) natively -- **including `std::meta` itself**.

- **`meta.hpp`** -- `micron::meta`, implementation of `[meta.reflection]`.
- **`reflect.hpp`**

```cpp
struct packet { u32 id; f64 weight; };

static_assert(mc::reflect::field_count<packet> == 2);

packet p{ 7, 2.5 };
mc::reflect::for_each_field(p, [](auto name, auto &value) {
  mc::io::println(name.data(), " = ", value);       // id = 7
});                                                 // weight = 2.5
```

Reflection is reimplemented fully in-tree and carries **no libstdc++ dependency and works freestanding**.

To build it you need to pass `-freflection` + `-std=c++26`, have at least gcc 16+ and define `MICRON_REFLECTION`.

> [!IMPORTANT]
> If a translation unit wants libstdc++'s real containers *and* reflection, include the libstdc++ headers **first**: micron then detects libstdc++ and defers to its `<meta>`.

###### Known limitations (experimental)

- Known TLS flakiness for very small (512KB) `group_thread` stacks under `-O2`; **use ≥4MB stacks** for now.

##### Code Coverage & Validation

Currently we are aiming for (near) 100% code coverage, of all functions and for (within reason) all inputs/domains. However, as of now the testing suites are still being written.

###### Conformance with the STL

*micron* currently provides numerous containers and functions which have existing implementations in the C++ Standard Library. Although *most* of these functions do generallyhave the same interfaces and functionality, there are minute core differences (in certain cases, significant ones) which you must be aware of. Do not assume all containers are functionally identical to the STL, because they are not.

> [!IMPORTANT]
> Documentation for the *micron* library does not currently exist, although the source is intended to be structured in a legible and understandable enough way to serve as documentation for the time being. *micron* is specifically designed for Linux; see the Architecture Support tier list above for per-architecture CPU status. Other operating systems and kernels are unsupported.

 
##### Libraries

All headers live under `src/` and may be included directly. Each top-level module exposes an umbrella header (e.g. `array.hpp`, `vector.hpp`, `math.hpp`) that re-exports its submodule, and a matching directory containing the individual implementations. The following list groups the modules by purpose:

###### Containers and data structures
- **`array/`** -- fixed-size, constexpr, immutable, persistent, frozen, contiguous and bisecting array variants
- **`vector/`** -- growable contiguous sequences (`vector`, `ivector`, `fvector`, `pvector`, `svector`, `convector`, `circle_vector`)
- **`string/`** -- string types and views (`sstring`, `istring`, `rope`, `unistring`, `string_view`), formatting and numeric conversions
- **`maps/`** -- open-addressing and tree-backed hash maps (`robin`, `hopscotch`, `swiss`, `b_map`, `immutable`, `itable`)
- **`trees/`** -- tree containers (B-tree, red-black, radix)
- **`heap/`** -- heap and priority structures (binary, binomial, fibonacci, quake, bloom filter, heapq)
- **`queue/`** -- FIFO queues (`queue`, `conqueue`, `iqueue`, `lambda_queue`, `spsc_queue`)
- **`stacks/`** -- LIFO stacks (`stack`, `fstack`, `istack`, `sstack`, `constack`, `cactus`)
- **`linux/`** -- Linux/POSIX layer covering syscalls, sysctl, polling, users and ELF
- **`dynamic/`** -- dynamic loading
- **`elf/`** -- ELF loader and related functionality
- **`hash/`** -- hash function family (`zzz`, `xxhash`, `fnv`, `murmur`, `crc`, `bernstein`, `fib`, `checksum`).
- **`sort/`** -- sorting algorithms (quick, merge, heap, radix, bitonic, comb, counting, insertion, bubble, stable, selection)
- **`algorithm/`** -- generic container algorithms (`find`, `filter`, `fold`, `accumulate`, arithmetic, data, unroll) plus a functional-programming variant suite (`fp*`)
- **`algorithm/lazy/`** (`micron::lz``) -- the lazy counterpart to `fp`
- **`simd/`** -- SIMD primitives, intrinsics, dispatch and per-architecture backends (`amd64`, `arm32`, `arm64`) for 128/256/512-bit registers and NEON

###### Graphics and GPU
- **`gfx/`** -- fundamental graphics layer
- **`gfx/gl`** -- openGL graphics stack
- **`gfx/vk`** -- Vulkan graphics stack

###### Memory
- **`memory/`** -- allocation, addressing, lifetime, and pointer machinery; the home of micron's memory stack
- **`memory/cmemory/`** -- vectorized `memcpy`/`memmove`/`memset`/`memcmp`/`memchr` routines
- **[`memory/allocation/`](src/memory/allocation/README.md)** -- allocators, explicit arenas, memory resources, kernel-side allocation, and the `abcmalloc` general-purpose allocator
- **`memory/pointers/`** -- smart-pointer family (`unique`, `shared`, `weak`, `atomic`, `hazard`, `sentinel`, `global`, `thread`, `void`)

###### Numerics and compute
- **`math/`** -- arithmetic, trigonometry, logarithms, square roots, activations, special functions, branchless helpers and dispatch
- **`math/blas/`** -- BLAS levels 1–3 with extensions and tag-based dispatch
- **`math/arbint/`** -- arbitrary precision integer support and associated utilities
- **`math/linalg/`** -- linear algebra (decompositions, polynomials, Householder, pseudoinverse, Schur)
- **`math/matrix/`** -- fixed- and dynamic-shape matrices with packed and viewed forms
- **`math/graph/`** -- packed/stable adjacency, edge-list, CSR, dense and bit graphs; conversions, certificates, and serial algorithms
- **`math/compute.hpp`** -- opt-in static-shape computation DAG with reusable storage, tensor aliases, persistent state and zero-copy sharing
- **`math/quants/`** -- vectors, tensors, quaternions and dynamic vector quantities
- **`math/quaternions/`** -- quaternion algebra, Euler conversions, rotations, kinematics, interpolation
- **`math/integrate/`** -- numerical integration (quadrature, Romberg, Simpson, Gauss, Monte Carlo, derivatives)
- **`math/splines/`** -- interpolation primitives (linear, cubic, monotone-cubic, B-spline, ND curves, smoothing)
- **`math/manifolds/`** -- differential-geometry primitives (embedded manifolds, Lie groups, tangent spaces, metrics)
- **`math/rng/`** -- random-number engines, distributions, hardware sources, Ziggurat sampler
- **`math/simd/`** -- SIMD-accelerated transcendentals (`exp`, `log`, `sqrt`, `trig`, manipulation)
- **`math/__asm/`** -- hand-written x86 assembly kernels (rsqrt/sqrt/divps for SSE and AVX, hardware RNG)

###### Concurrency

These modules build under both the hosted (`pthread`) and the freestanding backend; see *Threading & Concurrency* above.

- **`thread/`** -- thread primitives, pools, arenas, scheduling, CPU pinning, callbacks and thread-type variants
- **`mutex/`** -- mutex / lock implementations (`spin`, `queue`, `recursive`, `unique`, `guard`, `auto`), barriers, RCU, once-flags, tokens
- **`atomic/`** -- atomic operations, atomic flags and low-level intrinsics
- **`sync/`** -- synchronization primitives (`futex`, `future`/`promise`, `latch`, `semaphore`, `channel`, `async`, `defer`, `expect`, `inlet`, `invoke`, `pause`, `until`, `when`, `yield`, `contract`)
- **`parallel/`** -- parallel-execution helpers (`for`, `pipeline`, `poll`)
- **`tasks/`** -- lightweight task abstraction

###### OS and I/O
- **`io/`** -- high-level I/O: files, filesystems (incl. concurrent), paths, pipes, streams, formatting, console, serial, stdin/stdout/stderr, FTW, real-path resolution, **flash (io_uring-native file I/O)**
- **`io/graph.hpp`** -- opt-in edge-list, adjacency-list, Matrix Market, DIMACS, and versioned binary graph I/O
- **`io/posix/`** -- POSIX I/O wrappers (block, dir, file, terminal, volatile, iosys)
- **`io/term/`** -- ANSI terminal helpers
- **`io/uxin/`** -- input-device layer (event devices, key mapping, polling, virtual devices, Wayland reader)

###### Internal
- **`bits/`** -- compile-time architecture, container, exception and syscall-code dispatch headers
- **`asm/`** -- `_start` entry stub and C-side bootstrap
- **`__special/`** -- compiler-required STL replacements (`initializer_list`, `index_sequence`, `meta`, and a transitional `pthread` shim
- **`meta.hpp` / `reflect.hpp`** -- C++26 static reflection
- **`std.hpp`** -- single mega-header that pulls in the whole library


#### License
Licensed under the Boost Software License, except the 'abcmalloc' memory allocator, which is licensed under the MIT License
