<img align="left" style="width:300px" src="https://github.com/user-attachments/assets/8d544599-d4a3-4b8a-a61a-b83eb2a30b64" alt="micron_logo_default" width="300"/> 

<div align="left">

### the micron core library 🦅 <img src="https://img.shields.io/badge/indev-green">

#### a core library implementation (& redesign) of libc and the C++ Standard Library

**micron** is a comprehensive core library; a collection of algorithms, containers, iterators, functions, and OS interfaces; a header-only core system library written in c++23 targeting the Linux syscall API.
Unlike library collections such as Boost et al., *micron* does not intend to merely *augment* the STL, but entirely replace it.

</div>

[![Linux](https://img.shields.io/badge/Linux-FCC624?logo=linux&logoColor=black)](#)
![Version](https://img.shields.io/badge/version-1.7.0.1-green)
[![License](https://img.shields.io/badge/License-Boost_1.0-lightblue.svg)](https://www.boost.org/LICENSE_1_0.txt)
[![C++23](https://img.shields.io/badge/C++-23-blue.svg)](https://en.cppreference.com/w/cpp/23)


------

<br/>

<br/>


> [!WARNING]
> micron is still in active development, and as of v0.9 has reached stability across our supported platforms. Regardless, the ABI may change at any point, and without notice.

#### Features
  - a *fully functional*, templated C++ standard library implementation, designed from the **ground up** with modern principles in mind
  - completely self-contained, self-hosted, and freestanding; with *no dependencies on external code whatsoever*; not even the traditional C standard library
  - all functions are guaranteed to be side effect free, ensuring deterministic and predictable behavior across the codebase
  - written entirely in c++23
  - a high performance, cache-aware algorithmic base architecture
  - provides an innovative foundation for systems-level development, reimagining conventional approaches to low-level programming


##### Using the Library

All necessary code is self-contained within the `src/` directory. Since *micron* is freestanding, it relies on no external sources; no other files or libraries are necessary (*with the sole exception of `pthreads` for multi-threading, more on that below*). Simply include any header file you want into your project and compile. For examples, check out the `examples/` directory (currently being added).


##### Installation

First, clone the repository via ssh:

`git clone --depth=1 git@github.com:rfgplk/micron.cpp.git`

or https:

`git clone --depth=1 https://github.com/rfgplk/micron.cpp.git`


Below are the specific steps you need to take to properly set up micron for your desired target.

###### amd64 / x86_64 / i386 (x86)

The simplest, most straightforward installation; just copy all the files in `src/` and `external/` to either your desired location; or to the system header include directories `/usr/include/` or `/usr/local/include/`. Either use `cp -r`, `rsync`, or you can run `scripts/install_local.py` and `scripts/install_externals.py`, which will automatically copy all files to `/usr/include/micron` and `/usr/include/external` (NOTE: directories will be created if they don't exist).

###### AArch64 / armv7-a (ARM32)

The same exact steps as above. If you are cross compiling on amd64 for arm32 or aarch64, you should manually copy the source files to the include path of your cross compiler, which usually differs from system wide include paths. `scripts/install_local_linaro.py` will do that for you (if using the linaro toolchain on fedora), but exact paths may differ based on your configuration, so double check. Hint: `echo | /usr/gcc-linaro/bin/arm-none-linux-gnueabihf-c++ -E -Wp,-v -` will tell you which include directories the compiler uses. 

###### Freestanding builds
 
In order to compile micron binaries in freestanding mode (not linking against glibc or any system objects), you'll first need to run `scripts/install_start.py` which copies over all the `start/` files (containing _start and various other init code) to `/usr/src/mc_start`. Then you'll need to compile your binaries by providing the path to the start source files, example: 

```bash
/usr/bin/g++ -std=c++26 -Ofast -mavx2 -mbmi -march=native -fmodulo-sched -fmodulo-sched-allow-regmoves -fgcse-sm -fgcse-las -ffreestanding -nostdlib -nostdlib++ -fno-stack-protector -fno-exceptions -fno-rtti -m64 -Wall -Wextra -Wpedantic -Wno-variadic-macros -Wno-inline -flto=8 -Wno-odr -Wno-lto-type-mismatch -Wno-variadic-macros -Wno-inline tools/src/main.cc /usr/src/mc_start/start.s /usr/src/mc_start/start.cpp -I./src -L./libs/ -o bin/duck
```

This installation guide serves only as a rough suggestion, exact paths may depend on your use case and configuration.

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

Yes, *micron* relies on no external code other than what is included in this repository. Meaning as long as you have a working `g++/clang++` compiler, you can compile and run it anywhere. The sole exception being, if you wish to use multithreading (or any thread related code), you **must** link against pthread. I will get around eventually to implementing a threading model from scratch, just haven't had the time lately. 

###### Architecture Support

*micron* is built and tuned first for **x86_64 (amd64)**. Support for other CPU architectures is tiered as follows:

| Tier | Architectures | Status |
|------|---------------|--------|
| 🟢 **Full** | `amd64` / `x86_64`, `arm32` / `armv7-a` | Fully supported and tested. |
| 🟡 **Effective** | `arm64` / `aarch64`, `i386` / `x86` | Compiles properly, untested. (you may run into bugs!) |
| 🔵 **Future** | `RISC-V`, `POWER` (`ppc64`) | Planned in the future. No backend present today. |

> [!IMPORTANT]
> *micron* targets **Linux specifically**. It is built directly on Linux syscalls, ABI, and kernel conventions throughout; it is **NOT** a portable POSIX library. Some code may happen to build and run on other POSIX systems (the BSDs, macOS, etc.), but this is neither guaranteed nor supported. Linux is the only supported operating system. We will release a dedicated macOS version eventually.


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
- **`linux/`** -- Linux/POSIX layer covering syscalls, sysctl, polling, users and ELF parsing
- **`hash/`** -- hash function family (`zzz`, `xxhash`, `fnv`, `murmur`, `crc`, `bernstein`, `fib`, `checksum`); prefer `zzz`
- **`sort/`** -- sorting algorithms (quick, merge, heap, radix, bitonic, comb, counting, insertion, bubble, stable, selection)
- **`algorithm/`** -- generic container algorithms (`find`, `filter`, `fold`, `accumulate`, arithmetic, data, unroll) plus a functional-programming variant suite (`fp*`)
- **`simd/`** -- SIMD primitives, intrinsics, dispatch and per-architecture backends (`amd64`, `arm32`, `arm64`) for 128/256/512-bit registers and NEON

###### Graphics and GPU
- **`gfx/`** -- fundamental graphics layer
- **`gfx/gl`** -- openGL graphics stack
- **`gfx/vk`** -- Vulkan graphics stack

###### Memory
- **`memory/`** -- allocation, addressing, lifetime, and pointer machinery; the home of micron's memory stack
- **`memory/cmemory/`** -- vectorized `memcpy`/`memmove`/`memset`/`memcmp`/`memchr` routines (use these whenever possible)
- **`memory/allocation/`** -- allocators, memory resources, kernel-side allocation, and the `abcmalloc` general-purpose allocator
- **`memory/pointers/`** -- smart-pointer family (`unique`, `shared`, `weak`, `atomic`, `hazard`, `sentinel`, `global`, `thread`, `void`)

###### Numerics and compute
- **`math/`** -- arithmetic, trigonometry, logarithms, square roots, activations, special functions, branchless helpers and dispatch
- **`math/blas/`** -- BLAS levels 1–3 with extensions and tag-based dispatch
- **`math/linalg/`** -- linear algebra (decompositions, polynomials, Householder, pseudoinverse, Schur)
- **`math/matrix/`** -- fixed- and dynamic-shape matrices with packed and viewed forms
- **`math/quants/`** -- vectors, tensors, quaternions and dynamic vector quantities
- **`math/quaternions/`** -- quaternion algebra, Euler conversions, rotations, kinematics, interpolation
- **`math/integrate/`** -- numerical integration (quadrature, Romberg, Simpson, Gauss, Monte Carlo, derivatives)
- **`math/splines/`** -- interpolation primitives (linear, cubic, monotone-cubic, B-spline, ND curves, smoothing)
- **`math/manifolds/`** -- differential-geometry primitives (embedded manifolds, Lie groups, tangent spaces, metrics)
- **`math/rng/`** -- random-number engines, distributions, hardware sources, Ziggurat sampler
- **`math/simd/`** -- SIMD-accelerated transcendentals (`exp`, `log`, `sqrt`, `trig`, manipulation)
- **`math/__asm/`** -- hand-written x86 assembly kernels (rsqrt/sqrt/divps for SSE and AVX, hardware RNG)

###### Concurrency
- **`thread/`** -- thread primitives, pools, arenas, scheduling, CPU pinning, callbacks and thread-type variants
- **`mutex/`** -- mutex / lock implementations (`spin`, `queue`, `recursive`, `unique`, `guard`, `auto`), barriers, RCU, once-flags, tokens
- **`atomic/`** -- atomic operations, atomic flags and low-level intrinsics
- **`sync/`** -- synchronization primitives (`futex`, `future`/`promise`, `latch`, `semaphore`, `channel`, `async`, `defer`, `expect`, `inlet`, `invoke`, `pause`, `until`, `when`, `yield`, `contract`)
- **`parallel/`** -- parallel-execution helpers (`for`, `pipeline`, `poll`)
- **`tasks/`** -- lightweight task abstraction

###### OS and I/O
- **`io/`** -- high-level I/O: files, filesystems (incl. concurrent), paths, pipes, streams, formatting, console, serial, stdin/stdout/stderr, FTW, real-path resolution
- **`io/posix/`** -- POSIX I/O wrappers (block, dir, file, terminal, volatile, iosys)
- **`io/term/`** -- ANSI terminal helpers
- **`io/uxin/`** -- input-device layer (event devices, key mapping, polling, virtual devices, Wayland reader)

###### Internal
- **`bits/`** -- compile-time architecture, container, exception and syscall-code dispatch headers
- **`asm/`** -- `_start` entry stub and C-side bootstrap
- **`__special/`** -- compiler-required STL replacements (`initializer_list`, `index_sequence`, `pthread` shim)
- **`std.hpp`** -- single mega-header that pulls in the whole library


#### License
Licensed under the Boost Software License, except the 'abcmalloc' memory allocator, which is licensed under the MIT License
