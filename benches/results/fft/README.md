# FFT baseline — 2026-11-06

This is the historical baseline from before the SIMD tuning pass. The subsequent implementation, measurements, counter analysis, and reproduction instructions are in [the tuning report](../../../docs/FFT_TUNING.md). Current sources include those optimizations; use `original-to-tuned.patch` in an isolated copy to reconstruct the earlier benchmark, as described there.

Hardware: Intel Core i7-5960X, 8 cores / 16 threads, 20 MiB L3. Linux, CPU 2 affinity. Frequency/turbo were not locked. Compiler: GCC 16.2.1, C++23, x86-64/SSE2 base ISA. No runtime dispatch or microarchitecture-specific FFT kernel.

Sources: micron master `277641f` plus this uncommitted FFT implementation; Eigen `45b31bd`, default bundled KissFFT backend. Both executables used the same duck code-generation flags:

```sh
duck build benches/fft_bench.cpp --std c++23 --isa base --perf --fp --no-ssp --no-lto -i . -i ./src -o bin/fft/bench
duck build benches/fft_eigen_bench.cpp --std c++23 --isa base --perf --fp --no-ssp --no-lto -i /code/Git/eigen -o bin/fft/eigen
taskset -c 2 bin/fft/bench/fft_bench > benches/results/fft/gcc16-base-micron.csv
taskset -c 2 bin/fft/eigen/fft_eigen_bench > benches/results/fft/gcc16-base-eigen.csv
```

Duck emitted `-O3 -fmodulo-sched -fmodulo-sched-allow-regmoves -fgcse-sm -fgcse-las -march=x86-64 -m64 -fno-stack-protector -fstack-clash-protection -fstrict-overflow -fext-numeric-literals -fno-lto -funroll-loops -fno-omit-frame-pointer`, with its warning flags. No fast-math flag was enabled.

Seven samples per row; CSV reports median, minimum and maximum nanoseconds per call. Workspaces and input/output arrays are prepared before timed execution. Both C2C cases use identical double-precision inputs and unscaled forward, out-of-place transforms. Eigen is warmed once to prepare its mutable cache. The executables ran sequentially, without competing benchmark jobs. Compiler barriers keep output stores observable.

`plan_lifecycle` includes construction/destruction and precomputation; `workspace_lifecycle` includes allocation, value initialization and destruction. These are separate from prepared execution. They are not a comparison with Eigen's construction API, which does not expose the same explicit prepare operation.

| N | micron median ns | Eigen KissFFT median ns | micron / Eigen |
|---:|---:|---:|---:|
| 16 | 245.5 | 104.2 | 2.36 |
| 64 | 815.1 | 520.1 | 1.57 |
| 256 | 4321.5 | 2487.8 | 1.74 |
| 1000 | 22846.3 | 11789.9 | 1.94 |
| 1009 | 150842.0 | 2229974.3 | 0.07 |
| 1024 | 25978.0 | 11615.4 | 2.24 |
| 4096 | 103382.3 | 58967.6 | 1.75 |
| 16384 | 563855.2 | 322926.7 | 1.75 |

The smooth-length cases remain slower than Eigen in this baseline. The prime-length case benefits from Bluestein rather than KissFFT's generic quadratic radix. This is not a claim against Eigen's alternative FFT backends, its Tensor FFT implementation, or vendor libraries. No benchmark results are invented for libraries without a comparable available FFT path.

Micron's CSV also records packed real FFTs, DCT-II, fixed-kernel convolution, and 2D C2C. These establish regression baselines; they do not establish cross-library superiority. Portable butterfly scheduling and data-layout improvements remain useful performance work. Current ND/real scratch deliberately materializes the full complex logical volume; lower-memory packed execution is another optimization opportunity.

## Validation recorded with this baseline

All **29** cells in `tests/rigor/fft.duck` passed after the final butterfly/layout implementation:

- GCC and Clang: direct DFT/DCT/DST oracles, layout/convolution contracts, 256-bit fixtures, executor/concurrency tests.
- SSE2 base, AVX2, FFT scalar packet fallback, and both compilers at `-Ofast` (duck's normal optimization default).
- GCC ASan+UBSan layout tests; Clang ASan+UBSan core and parallel tests.
- Freestanding without exceptions (`-k`) and with micron exceptions (`-ke`).
- i386, ARMv7-A and AArch64 freestanding precision tests with both compilers; GCC layout tests on all three. ARM targets executed under QEMU; the i386 precision binary also passed an explicit `qemu-i386-static` run.
- Hosted and freestanding two-TU linkage, both link orders, with GCC and Clang (`tests/build/fft_odr.sh`).
- `math.hpp` compile coverage on native GCC and AArch64 Clang; freestanding runnable example.

Sanitizer failures explicitly use exit code 77 so they cannot collide with the test harness's success sentinel of 1.

Allocation counters check zero allocation/deallocation during prepared C2C, real FFT, DCT/DST and convolution execution. Parallel tests share an immutable plan across separate workspaces and exercise all-accepted, all-rejected and partially rejected executor submissions. The freestanding AArch64 precision binary has no unresolved symbols; the amd64 freestanding core binary has no dynamic section.

This is focused FFT validation, not a claim that the repository's entire compile/test matrix ran. Timing results are native amd64 only; cross-target tests establish correctness, not performance.

## Correctness review — 2026-11-13

The review fixed four defect classes in the uncommitted implementation:

- Finite complex division could overflow intermediate sums or lose small products and ratios, even when the quotient was representable. `tests/rigor/fft_core_regressions.cpp` covers both denominator branches, the extreme ranges, constant evaluation, and seeded comparisons with a wider independent oracle.
- GCC could merge AVX2 floating sign masks under `-Ofast`, making forward and inverse quarter-turns agree. Integer masks retain distinct rotations. `tests/rigor/fft_packets.cpp` covers both precisions, conjugation, rotations, multiplication, aliasing, and exact buffer extents.
- Frequency helpers could return zero for representable subnormal bins, or NaN at DC, because the shared reciprocal overflowed or underflowed. `tests/rigor/fft_helper_regressions.cpp` covers both extremes.
- Empty multidimensional convolution could reject a valid zero-volume shape while computing unused padding volumes. `tests/rigor/fft_convolution_regressions.cpp` covers every zero-axis position and all four convolution modes on 32-bit and 64-bit targets.

All 68 cells in the expanded `tests/rigor/fft.duck` passed: the original 29 plus 9 division, 13 packet, 9 frequency-helper, and 8 empty-convolution configurations. These include GCC, Clang, ASan/UBSan, scalar packets, SSE2, AVX2, fast math, freestanding execution, i386, ARMv7-A, and AArch64. Seven additional arbint FFT configurations passed, including forced tiling, a larger transform cap, sanitizers, and all four architectures. The audit found no additional internal UAF, out-of-bounds access, integer wraparound, or C++ undefined behavior under the documented caller contracts. These are review and test findings, not a proof of their absence.

`audit-base.csv` and `audit-v3.csv` compare binaries saved before this review with the patched implementation. Each covers 34 workloads, with three alternating pairs and seven internal samples per invocation. The geometric mean paired speedups are 0.9981× for both SSE2 and AVX2, within timing noise. `audit-targeted.csv` retains nine extra pairs for the initial outliers; their median speedups range from 0.994× to 1.016×. Raw samples, environment metadata, and the four patched header hashes are retained alongside the summaries.

`benches/complex_div_bench.cpp` compares ordinary complex division against the original Smith formula, using seven alternating pairs of seven-sample medians. Build it with `--std c++23 --perf --fp --no-ssp --no-lto` and `--isa base` or `--isa v3`, adding `--clang` for Clang. `audit-division-summary.csv` records paired speedups from runs with measured sibling-core utilization below 7%: GCC base 1.17×/1.31×/1.09×, Clang base 1.03×/1.58×/1.14×, and GCC AVX2 1.09×/1.44×/1.02× for float/double/long double. Other runs under competing compile/test jobs, including slowdowns, remain in `audit-division.csv`; this does not establish a performance guarantee under contended SMT workloads. Extreme-range arithmetic requires the strict floating-point behavior described in `docs/FFT.md`.
