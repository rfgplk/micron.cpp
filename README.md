<img align="left" style="width:300px" src="https://github.com/user-attachments/assets/8d544599-d4a3-4b8a-a61a-b83eb2a30b64" alt="micron_logo_default" width="300"/> 

<div align="left">

### the micron core library 🦅 <img src="https://img.shields.io/badge/indev-green">

#### a core library implementation (& redesign) of libc and the C++ Standard Library

**micron** is a comprehensive core library; a collection of algorithms, containers, iterators, functions, and OS interfaces; a header-only core system library written in c++23 targeting the Linux syscall API.
Unlike library collections such as Boost et al., *micron* does not intend to merely *augment* the STL, but entirely replace it.

</div>

![Linux](https://img.shields.io/badge/Linux-FCC624?logo=linux&logoColor=black)
![C++23+](https://img.shields.io/badge/C%2B%2B-23%2B-blue.svg)
[![Boost License](https://img.shields.io/badge/License-Boost_1_0-lightblue.svg)](LICENSE_1_0.txt)

------

<br/>

<br/>


> [!WARNING]
> This branch is the **barebones** variant. Its primary targets are Linux kernel modules and bare-metal images. A hosted Linux backend is also available.

## Features

- Header-only C++23-compatible library code; Duck currently defaults to `-std=c++26`.
- No libc, STL, or third-party runtime dependency in micron itself.
- One compile-time operating-system seam: `micron::port`.
- A scalar SIMD tier for targets where vector registers are unavailable.
- A small barebones allocator, `micron::bb`, for kernel and metal builds.
- Direct support for amd64, i386, ARMv7-A, AArch64, and ARMv7-M Cortex-M subtargets.
- The Duck build tool for GCC and Clang, with compile matrices and target-specific recipes.

## Target profiles

The backend is selected at compile time. `MICRON_PORT_KERNEL` and `MICRON_PORT_METAL` are mutually exclusive; with neither defined, the Linux userspace backend is selected.

| Profile | Build form | Port backend | Allocator | Output |
| --- | --- | --- | --- | --- |
| Linux userspace | ordinary Duck build | Linux syscalls | `abcmalloc` by default | executable |
| Linux kernel | `duck ... --kernel` or the example kbuild | kernel symbols | `micron::bb` | relocatable module object |
| Bare metal | `duck ... --metal` or `examples/metal/Makefile` | board hooks | `micron::bb` | linked image |

`--kernel` and `--metal` are not `duck run` targets: the former is linked by kbuild, while the latter needs a board or boot-loader environment. Use the workflows documented below.

Kernel and metal builds define `MICRON_NO_SIMD`, `MICRON_NO_FP`, and `MICRON_NO_TLS`.
They do not use floating-point/vector registers or ELF TLS. The generic implementation is selected
by capability, not merely by CPU architecture.

## Quick start

### Build Duck

Duck is micron's no-build-file compiler driver. It drives GCC, Clang, or NASM directly; the recipes
are stored in `tools/src/`.

```sh
sh scripts/bootstap_duck.sh
```

The script writes `bin/duck`. An installed `duck` on `PATH` works too.

### Run a hosted example

```sh
duck run examples/concepts.cpp -O2 -o bin/examples
```

The source examples include headers through the repository's `src/` tree. For a program outside the
repository, use the public include shape and add the repository root as an include path:

```cpp
#include <micron/print.hpp>
#include <micron/vector.hpp>

int main()
{
  micron::vector<int> values{ 4, 1, 3, 2 };
  micron::println("size=", values.size(), " first=", values[0]);
  return 0;
}
```

From a clone, the `micron -> src` symlink makes `<micron/...>` resolve with `-i .`:

```sh
duck run my_program.cpp -i . -i ./src -O2 -o bin/my_program
```

Use focused headers whenever possible. `std.hpp` supplies foundational types, allocation hooks,
errors, and configuration; it is not a whole-library mega-header.

### Install headers or the freestanding CRT

If an installed header tree is needed:

```sh
sudo python3 scripts/install_local.py /usr/local/include/micronbb
sudo python3 scripts/install_externals.py /usr/local/include/external
```

`install_local.py` copies headers directly into the destination directory; configure the compiler's
include path to match that layout. The script's default destination is `/usr/include/micronbb`.

Freestanding entry code is installed separately:

```sh
sudo python3 scripts/install_start.py
```

The barebones CRT default is `/usr/src/mc_start_bb`. A checkout can use `--start ./start` instead;
when using the repository's `<micron/...>` includes, also pass `-i . -i ./src`.

## OS layer: `micron::port`

`micron::port` is the only public layer that names the operating environment. Depend on the narrow
facet you need instead of including the umbrella `port/port.hpp` everywhere.

| Header | Main facilities |
| --- | --- |
| `port/panic.hpp` | `write_diag`, `flush_diag`, `halt`, `halt_local` |
| `port/pages.hpp` | page allocation, reserve/commit/decommit, protection, heap extent, readability |
| `port/yield.hpp` | `yield`, `cpu_relax` |
| `port/clock.hpp` | monotonic/realtime clocks, frequency, sleeping |
| `port/ident.hpp` | process, execution, CPU, and liveness identifiers |
| `port/wait.hpp` | `wait`, `wake`, and the Linux futex hook |
| `port/rawmap.hpp` | raw emergency mappings for facilities such as exception support |
| `port/irq.hpp` | interrupt save/restore; a no-op in Linux userspace |

For example:

```cpp
#include <micron/port/clock.hpp>
#include <micron/port/pages.hpp>
#include <micron/print.hpp>

int main()
{
  const auto before = micron::port::mono_ticks();
  auto block = micron::port::page_alloc(micron::port::page_size);

  micron::println("page_size=", micron::port::page_size,
                  " readable=", micron::port::addr_readable(block.ptr));

  micron::port::page_free(block);
  micron::println("elapsed=", micron::port::mono_ticks() - before, " ns");
  return 0;
}
```

The Linux backend issues syscalls. The kernel backend calls the kernel's exported facilities. The
metal backend calls weak board hooks in `start/metal/mc_mport.cpp`; a real board replaces those
hooks with UART, halt, clock, and optional memory implementations.

## Printing and formatting

`src/print.hpp` is the universal diagnostic printer. It does not open descriptors or depend on a
stream layer.

```cpp
#include <micron/print.hpp>

micron::print("without newline");
micron::println(" value=", 42, " ok=", true);
micron::printn("same line: ", 1, ' ', 2, '\n');
```

`micron::io::print`, `micron::io::println`, and `micron::io::printn` remain valid aliases for
existing callers. For a caller-owned destination, use `printk(sink, value)` with a sink implementing
`put()` and `flush()`.

## Allocators and restricted targets

Hosted Linux normally uses `abcmalloc`. Kernel and metal builds automatically select the generated
barebones allocator and expose it as `micron::bb`; the compatibility `abc` namespace is mapped to
that allocator where older keep-set headers still name it.

To exercise the barebones allocator in a hosted test:

```sh
duck test tests/rigor/bb_allocator.cpp -O2 \
  --def MICRON_BAREBONES_ALLOC --def MICRON_NO_ZZZ_HASH -o bin/bb-test
```

For metal, the heap must be supplied explicitly. The examples use `MICRON_BB_PORT_POOL`; boards
with a static or linker-provided pool can use the corresponding `MICRON_BB_STATIC_POOL` or
`MICRON_BB_LINKER_POOL` configuration.

The most important target gates are:

| Define/flag | Meaning |
| --- | --- |
| `--kernel` | Linux kernel-module code generation; produces a raw object for kbuild |
| `--metal` | Bare-metal image code generation and link; requires a heap pool |
| `-k` | Freestanding, no C++ exceptions or RTTI |
| `-ke` / `--eh` | Freestanding build with micron's exception trampoline |
| `MICRON_NO_SIMD` | Force scalar SIMD backends |
| `MICRON_NO_FP` | Promise that hardware floating point is unavailable |
| `MICRON_NO_TLS` | Remove ELF TLS requirements |
| `MICRON_ALLOW_GENERIC_ARCH` | Opt into the scalar generic arch tier for an unrecognised target |

Do not infer target capability from `__AVX2__` or from the CPU architecture alone. Code that needs
vector registers must follow micron's capability macros and remain valid through the generic tier.

## Architectures and ISA selection

The direct architecture targets are:

- amd64 / x86-64;
- i386 / 32-bit x86;
- ARMv7-A (`--arm`, hard-float NEON);
- AArch64 (`--arm64` / `--aarch64`);
- ARMv7-M through `--cortex-m <cpu>` with `--metal`, for example `cortex-m3` or `cortex-m4`.

On x86, `--isa` selects the instruction-set floor:

```sh
duck compile examples/simd.cpp --x86 --isa base -O2
duck compile examples/simd.cpp --x86 --isa v2 -O2
duck compile examples/simd.cpp --x86 --isa v3 -O2
duck compile examples/simd.cpp --x86 --isa v4 -O2
```

`base` is x86-64 with SSE2, `v2` adds the x86-64-v2 instructions, `v3` adds AVX2/BMI/FMA, and
`v4` adds AVX-512. The default is `native`; use `base` for a portable x86-64 build or a board
image that must not inherit the build host's ISA.

## Building a Linux kernel module

The complete example lives in [`examples/kernel_module`](examples/kernel_module). It separates
Linux's C-only module glue from the C++ micron payload:

- `mod_main.c` owns `module_init`, `module_exit`, and Linux module metadata;
- `mod_demo.cpp` uses micron containers, sorting, hashing, strings, printing, and `port`;
- `Kbuild` supplies the no-FPU, no-TLS, no-exceptions configuration;
- `mc_kport.c` and `mc_libgcc.cpp` provide the target support objects.

Requirements are the running kernel's development headers and a kernel configuration that permits
loading the module. Build and inspect it with:

```sh
make -C examples/kernel_module
make -C examples/kernel_module check
```

To load the demonstration module on a test machine:

```sh
make -C examples/kernel_module load
make -C examples/kernel_module unload
```

Loading requires the appropriate privileges and may be blocked by Secure Boot or module-signing
policy. The demo reports its checks through `dmesg`; a failure returns a negative errno and refuses
module initialization.

For a standalone object rather than the complete kbuild flow, Duck's kernel recipe is:

```sh
duck compile my_module.cpp --x86 --kernel -o bin/kernel
```

This is an object build, not an executable. Kbuild must supply the final module link and Linux
module metadata.

## Building and booting bare metal

The maintained QEMU board examples are in [`examples/metal`](examples/metal). The Makefile builds
the board port separately, links it with `metal_demo.cpp`, checks the image, boots it, and grades
the serial output.

```sh
make -C examples/metal ARCH=i386 run
```

Other configured targets are:

```sh
make -C examples/metal ARCH=amd64 run
make -C examples/metal ARCH=arm32 run
make -C examples/metal ARCH=arm64 run
make -C examples/metal ARCH=stm32 MCU=cortex-m4 run
```

`ARCH=stm32` also supports `MCU=cortex-m3` and the matching QEMU machine. Build and gate all
configured images with:

```sh
make -C examples/metal all-images
```

The metal demo deliberately exercises more than startup: constructors, vectors, sorting, a hash
map, strings, the heap extent, address readability, and diagnostic printing. A successful boot
prints `ALL CHECKS PASSED`. The QEMU board port supplies the UART and halt behavior; real hardware
replaces those hooks in a board-specific file.

The amd64 `METAL_ENTRY=lm` form is for a loader that has already entered long mode:

```sh
make -C examples/metal ARCH=amd64 METAL_ENTRY=lm image
```

It is a linkable image shape, but the repository's QEMU flow does not boot it because no loader in
the tree hands over in that state.

## Design rules and limitations

- Linux is the supported hosted and kernel operating system. Bare metal uses its own board port;
  it is not a portable POSIX layer.
- `max_t` results use the micron convention: non-negative values are successful byte/count results;
  negative values are `-errno` failures. Configuration/setup failures may use micron exceptions.
- The library is not a drop-in implementation of `std`; matching facilities may have different
  ownership, error, allocation, and iterator contracts.
- Header-only does not mean every header is valid in every target profile. Floating-point APIs,
  vector APIs, TLS-dependent code, and facilities requiring an operating-system service must be
  selected only where the target provides them.
- Keep the compiler's target flags consistent with the board or kernel ABI. In particular, do not
  build a metal image with `-march=native` unless the image is guaranteed to run on that same CPU.

`ISSUES.md` records known defects and unsupported combinations. They are maintainer-facing references;
this README is the user-facing starting point.

## License

The micron library is distributed under the [Boost Software License 1.0](LICENSE_1_0.txt). The
`abcmalloc` allocator retains its MIT license; the generated barebones allocator follows its
upstream license and synchronization rules.
