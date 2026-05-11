//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Extensive BLAS / linear algebra benchmark for the micron math kernels
//
//   L1: dot, axpy, scal, asum, nrm2, iamax, copy   (linear time)
//   L2: gemv (no-trans, trans), ger                (quadratic)
//   L3: gemm (NN/NT/TN/TT), syrk, symm             (cubic)
//   linalg: trace, frobenius_norm, det<N>, inv<N>, solve<N>, kron

#include "../external/bbench/bench.hpp"

#include "../src/io/console.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/math/blas/blas.hpp"
#include "../src/math/linalg.hpp"
#include "../src/math/matrix/dynmat.hpp"
#include "../src/math/matrix/matrices.hpp"
#include "../src/math/quants/vec.hpp"
#include "../src/std.hpp"

namespace
{

using blas_events = bbench::event_group<bbench::hardware_cycles, bbench::hardware_instructions, bbench::branches, bbench::branch_misses>;

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 4;

[[gnu::always_inline]] inline u64
lcg_next(u64 &s) noexcept
{
  s = s * 6364136223846793005ULL + 1442695040888963407ULL;
  return s;
}

template <typename T>
void
fill_pattern(T *p, u64 n, u64 seed) noexcept
{
  for ( u64 i = 0; i < n; ++i ) {
    const f64 r = static_cast<f64>(lcg_next(seed) >> 11) * 0x1.0p-53;
    p[i] = static_cast<T>(r - 0.5);
  }
}

[[gnu::always_inline]] inline void
clobber(const void *p) noexcept
{
  asm volatile("" : : "r"(p) : "memory");
}

f64
median_f64(f64 *xs, u32 n) noexcept
{
  for ( u32 i = 1; i < n; ++i ) {
    const f64 key = xs[i];
    u32 j = i;
    while ( j > 0 && xs[j - 1] > key ) {
      xs[j] = xs[j - 1];
      --j;
    }
    xs[j] = key;
  }
  return xs[n / 2];
}

struct fmt2 {
  u64 whole;
  u32 frac_x100;
};

[[gnu::always_inline]] inline fmt2
to_fmt2(f64 v) noexcept
{
  if ( v < 0 ) v = 0;
  const u64 scaled = static_cast<u64>(v * 100.0 + 0.5);
  return { scaled / 100, static_cast<u32>(scaled % 100) };
}

struct line {
  char buf[256];
  u32 pos;

  constexpr line() noexcept : pos(0) {}

  void
  s(const char *p) noexcept
  {
    while ( *p ) buf[pos++] = *p++;
  }

  void
  pad_to(u32 end_col, u32 written) noexcept
  {
    const u32 want = end_col >= written ? end_col - written : 0;
    if ( want < pos )
      buf[pos++] = ' ';
    else
      while ( pos < want ) buf[pos++] = ' ';
  }

  void
  u_at(u64 v, u32 end_col) noexcept
  {
    char tmp[24];
    u32 n = 0;
    if ( v == 0 )
      tmp[n++] = '0';
    else {
      u64 vv = v;
      while ( vv ) {
        tmp[n++] = '0' + (vv % 10);
        vv /= 10;
      }
    }
    pad_to(end_col, n);
    while ( n ) buf[pos++] = tmp[--n];
  }

  void
  f2_at(fmt2 f, u32 end_col) noexcept
  {
    char tmp[24];
    u32 n = 0;
    u64 w = f.whole;
    if ( w == 0 )
      tmp[n++] = '0';
    else
      while ( w ) {
        tmp[n++] = '0' + (w % 10);
        w /= 10;
      }
    pad_to(end_col, n + 3);
    while ( n ) buf[pos++] = tmp[--n];
    buf[pos++] = '.';
    buf[pos++] = '0' + static_cast<char>(f.frac_x100 / 10);
    buf[pos++] = '0' + static_cast<char>(f.frac_x100 % 10);
  }

  void
  s_at(const char *p, u32 end_col) noexcept
  {
    u32 n = 0;
    while ( p[n] ) ++n;
    pad_to(end_col, n);
    while ( *p ) buf[pos++] = *p++;
  }

  const char *
  str() noexcept
  {
    buf[pos] = '\0';
    return buf;
  }
};

struct row {
  const char *name;
  u64 n;
  u64 ops;
  f64 cyc_per_op;
  f64 ipc;
  f64 gflops;
  f64 bmiss_rate;
};

[[gnu::cold]] void
print_header(const char *title, const char *ops_unit, const char *flop_col_label)
{
  micron::io::println("");
  micron::io::println("[", title, "]    units: ", ops_unit, "    rate column: ", flop_col_label);
  line h;
  h.s("routine");
  h.s_at("size", 32);
  h.s_at("ops", 44);
  h.s_at("cyc/op", 54);
  h.s_at("IPC", 62);
  h.s_at(flop_col_label, 72);
  h.s_at("bmiss%", 80);
  micron::io::println(h.str());
  micron::io::println("--------------------------------------------------------------------------------");
}

[[gnu::cold]] void
print_row(const row &r)
{
  const fmt2 cpo = to_fmt2(r.cyc_per_op);
  const fmt2 ipc = to_fmt2(r.ipc);
  const fmt2 gf = to_fmt2(r.gflops);
  const fmt2 bm = to_fmt2(r.bmiss_rate * 100.0);
  line ln;
  ln.s(r.name);
  ln.u_at(r.n, 32);
  ln.u_at(r.ops, 44);
  ln.f2_at(cpo, 54);
  ln.f2_at(ipc, 62);
  ln.f2_at(gf, 72);
  ln.f2_at(bm, 80);
  micron::io::println(ln.str());
}

template <typename Kernel>
row
measure(const char *name, u64 size_descr, u64 ops_per_rep, u64 reps, Kernel &&kernel) noexcept
{
  for ( u64 i = 0; i < WARMUP_REPS; ++i ) kernel();

  f64 cpo_samples[K_MEASUREMENTS];
  f64 ipc_samples[K_MEASUREMENTS];
  f64 gf_samples[K_MEASUREMENTS];
  f64 bm_samples[K_MEASUREMENTS];

  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    blas_events evs{ bbench::quiet{} };
    evs.open();
    evs.begin();
    for ( u64 i = 0; i < reps; ++i ) kernel();
    evs.end();
    const auto cyc = static_cast<u64>(evs.get<bbench::hardware_cycles>().retrieve());
    const auto ins = static_cast<u64>(evs.get<bbench::hardware_instructions>().retrieve());
    const auto br = static_cast<u64>(evs.get<bbench::branches>().retrieve());
    const auto bm = static_cast<u64>(evs.get<bbench::branch_misses>().retrieve());
    const f64 total_ops = static_cast<f64>(reps) * static_cast<f64>(ops_per_rep);
    cpo_samples[m] = static_cast<f64>(cyc) / total_ops;
    ipc_samples[m] = cyc > 0 ? static_cast<f64>(ins) / static_cast<f64>(cyc) : 0.0;

    gf_samples[m] = cyc > 0 ? total_ops / static_cast<f64>(cyc) : 0.0;
    bm_samples[m] = br > 0 ? static_cast<f64>(bm) / static_cast<f64>(br) : 0.0;
  }

  return row{ name,
              size_descr,
              ops_per_rep,
              median_f64(cpo_samples, K_MEASUREMENTS),
              median_f64(ipc_samples, K_MEASUREMENTS),
              median_f64(gf_samples, K_MEASUREMENTS),
              median_f64(bm_samples, K_MEASUREMENTS) };
}

constexpr u64 TARGET_OPS = 1ULL << 26;

[[gnu::always_inline]] inline u64
calibrate_reps(u64 ops_per_rep) noexcept
{
  if ( ops_per_rep == 0 ) return 64;
  u64 r = TARGET_OPS / ops_per_rep;
  if ( r < 8 ) r = 8;
  if ( r > 1ULL << 22 ) r = 1ULL << 22;
  return r;
}

constexpr u64 L1_SIZES[] = {
  64, 1ULL << 12, 1ULL << 16, 1ULL << 20, 1ULL << 22,
};

void
sweep_level1_f64()
{
  print_header("BLAS L1 f64", "elements", "ops/cyc");
  for ( u64 n : L1_SIZES ) {
    mc::math::dynmat<f64> X(1, n);
    mc::math::dynmat<f64> Y(1, n);
    fill_pattern<f64>(X.data(), n, 0xDEAD);
    fill_pattern<f64>(Y.data(), n, 0xBEEF);

    f64 *x = X.data();
    f64 *y = Y.data();
    static volatile f64 sink = 0;
    const u64 reps = calibrate_reps(n);

    print_row(measure("L1 axpy        ", n, n, reps, [&] {
      mc::math::blas::level1::axpy<f64>(1.5, x, x + n, y);
      clobber(y);
    }));
    print_row(measure("L1 scal        ", n, n, reps, [&] {
      mc::math::blas::level1::scal<f64>(0.999, x, x + n);
      clobber(x);
    }));
    print_row(measure("L1 dot         ", n, n, reps, [&] { sink = mc::math::blas::level1::dot<f64>(x, x + n, y); }));
    print_row(measure("L1 asum        ", n, n, reps, [&] { sink = mc::math::blas::level1::asum<f64>(x, x + n); }));
    print_row(measure("L1 nrm2        ", n, n, reps, [&] { sink = mc::math::blas::level1::nrm2<f64>(x, x + n); }));
    print_row(measure("L1 nrm2_fast   ", n, n, reps, [&] { sink = mc::math::blas::level1::nrm2_fast<f64>(x, x + n); }));
    print_row(measure("L1 iamax       ", n, n, reps, [&] { sink = static_cast<f64>(mc::math::blas::level1::iamax<f64>(x, x + n)); }));
    print_row(measure("L1 copy        ", n, n, reps, [&] {
      mc::math::blas::level1::copy<f64>(x, x + n, y);
      clobber(y);
    }));
    (void)sink;
  }
}

void
sweep_level1_f32()
{
  print_header("BLAS L1 f32", "elements", "ops/cyc");
  for ( u64 n : L1_SIZES ) {
    mc::math::dynmat<f32> X(1, n);
    mc::math::dynmat<f32> Y(1, n);
    fill_pattern<f32>(X.data(), n, 0x1234);
    fill_pattern<f32>(Y.data(), n, 0x5678);

    f32 *x = X.data();
    f32 *y = Y.data();
    static volatile f32 sink = 0;
    const u64 reps = calibrate_reps(n);

    print_row(measure("L1 axpy        ", n, n, reps, [&] {
      mc::math::blas::level1::axpy<f32>(1.5f, x, x + n, y);
      clobber(y);
    }));
    print_row(measure("L1 dot         ", n, n, reps, [&] { sink = mc::math::blas::level1::dot<f32>(x, x + n, y); }));
    print_row(measure("L1 nrm2        ", n, n, reps, [&] { sink = mc::math::blas::level1::nrm2<f32>(x, x + n); }));
    print_row(measure("L1 nrm2_fast   ", n, n, reps, [&] { sink = mc::math::blas::level1::nrm2_fast<f32>(x, x + n); }));
    (void)sink;
  }
}

constexpr u64 L2_DIMS[] = { 32, 128, 512, 1024 };

void
sweep_level2_f64()
{
  print_header("BLAS L2 f64", "flops", "flop/cyc");
  for ( u64 d : L2_DIMS ) {
    mc::math::dynmat<f64> A(d, d);
    mc::math::dynmat<f64> X(1, d);
    mc::math::dynmat<f64> Y(1, d);
    fill_pattern<f64>(A.data(), d * d, 0xAAAA);
    fill_pattern<f64>(X.data(), d, 0xBBBB);
    fill_pattern<f64>(Y.data(), d, 0xCCCC);
    auto Av = mc::math::as_row_view(A);

    const u64 flops_gemv = 2ULL * d * d;
    const u64 flops_ger = 2ULL * d * d;
    const u64 reps = calibrate_reps(flops_gemv);

    print_row(measure("L2 gemv-N      ", d, flops_gemv, reps, [&] {
      mc::math::quants::vec_view<f64> xv{ X.data(), d, 1 };
      mc::math::quants::vec_view<f64> yv{ Y.data(), d, 1 };
      mc::math::blas::level2::gemv<mc::math::blas::op::none, f64>(1.0, Av, xv, 0.0, yv);
      clobber(Y.data());
    }));
    print_row(measure("L2 gemv-T      ", d, flops_gemv, reps, [&] {
      mc::math::quants::vec_view<f64> xv{ X.data(), d, 1 };
      mc::math::quants::vec_view<f64> yv{ Y.data(), d, 1 };
      mc::math::blas::level2::gemv<mc::math::blas::op::trans, f64>(1.0, Av, xv, 0.0, yv);
      clobber(Y.data());
    }));
    print_row(measure("L2 ger         ", d, flops_ger, reps, [&] {
      mc::math::quants::vec_view<f64> xv{ X.data(), d, 1 };
      mc::math::quants::vec_view<f64> yv{ Y.data(), d, 1 };
      mc::math::blas::level2::ger<f64>(0.001, xv, yv, Av);
      clobber(A.data());
    }));
  }
}

constexpr u64 L3_DIMS[] = { 32, 128, 512 };

void
sweep_level3_f64()
{
  print_header("BLAS L3 f64", "flops", "flop/cyc");
  for ( u64 d : L3_DIMS ) {
    mc::math::dynmat<f64> A(d, d);
    mc::math::dynmat<f64> B(d, d);
    mc::math::dynmat<f64> C(d, d);
    fill_pattern<f64>(A.data(), d * d, 0x1111);
    fill_pattern<f64>(B.data(), d * d, 0x2222);
    fill_pattern<f64>(C.data(), d * d, 0x3333);
    auto Av = mc::math::as_row_view(A);
    auto Bv = mc::math::as_row_view(B);
    auto Cv = mc::math::as_row_view(C);

    const u64 flops = 2ULL * d * d * d;

    u64 reps = calibrate_reps(flops);
    if ( d >= 512 ) reps = reps < 4 ? 4 : (reps > 16 ? 16 : reps);

    print_row(measure("L3 gemm-NN     ", d, flops, reps, [&] {
      mc::math::blas::level3::gemm(f64(1.0), Av, Bv, f64(0.0), Cv);
      clobber(C.data());
    }));
    print_row(measure("L3 gemm-NT     ", d, flops, reps, [&] {
      mc::math::blas::level3::gemm<mc::math::blas::op::none, mc::math::blas::op::trans>(f64(1.0), Av, Bv, f64(0.0), Cv);
      clobber(C.data());
    }));
    print_row(measure("L3 gemm-TN     ", d, flops, reps, [&] {
      mc::math::blas::level3::gemm<mc::math::blas::op::trans, mc::math::blas::op::none>(f64(1.0), Av, Bv, f64(0.0), Cv);
      clobber(C.data());
    }));
    print_row(measure("L3 gemm-TT     ", d, flops, reps, [&] {
      mc::math::blas::level3::gemm<mc::math::blas::op::trans, mc::math::blas::op::trans>(f64(1.0), Av, Bv, f64(0.0), Cv);
      clobber(C.data());
    }));
  }
}

void
sweep_level3_f32()
{
  print_header("BLAS L3 f32", "flops", "flop/cyc");
  for ( u64 d : L3_DIMS ) {
    mc::math::dynmat<f32> A(d, d);
    mc::math::dynmat<f32> B(d, d);
    mc::math::dynmat<f32> C(d, d);
    fill_pattern<f32>(A.data(), d * d, 0x4444);
    fill_pattern<f32>(B.data(), d * d, 0x5555);
    fill_pattern<f32>(C.data(), d * d, 0x6666);
    auto Av = mc::math::as_row_view(A);
    auto Bv = mc::math::as_row_view(B);
    auto Cv = mc::math::as_row_view(C);

    const u64 flops = 2ULL * d * d * d;
    u64 reps = calibrate_reps(flops);
    if ( d >= 512 ) reps = reps < 4 ? 4 : (reps > 16 ? 16 : reps);

    print_row(measure("L3 gemm-NN     ", d, flops, reps, [&] {
      mc::math::blas::level3::gemm(f32(1.0f), Av, Bv, f32(0.0f), Cv);
      clobber(C.data());
    }));
  }
}

template <u64 N>
void
sweep_linalg_static()
{
  const char *tag = N == 2 ? "linalg N=2 " : N == 3 ? "linalg N=3 " : N == 4 ? "linalg N=4 " : "linalg N=8 ";
  print_header(tag, "matrices", "ops/cyc");

  mc::math::mat<f64, N, N> A{};
  for ( u64 i = 0; i < N * N; ++i ) A.data[i] = 0.5 - static_cast<f64>(i) / static_cast<f64>(N * N);

  for ( u64 i = 0; i < N; ++i ) A.data[i * N + i] += static_cast<f64>(N + 1);

  mc::math::vec<f64, N> b{};
  for ( u64 i = 0; i < N; ++i ) b.data[i] = static_cast<f64>(i + 1);

  static volatile f64 sink = 0;
  const u64 reps = calibrate_reps(N * N * N);

  print_row(measure("trace          ", N, 1, reps, [&] { sink = mc::math::linalg::trace<f64, N>(A); }));
  print_row(measure("frobenius_norm ", N, 1, reps, [&] { sink = mc::math::linalg::frobenius_norm<f64, N, N>(A); }));
  print_row(measure("det            ", N, 1, reps, [&] { sink = mc::math::linalg::det<f64, N>(A); }));
  print_row(measure("inv            ", N, 1, reps / 2, [&] {
    auto X = mc::math::linalg::inv<f64, N>(A);
    clobber(X.data);
  }));
  print_row(measure("solve          ", N, 1, reps / 2, [&] {
    auto x = mc::math::linalg::solve<f64, N>(A, b);
    clobber(x.data);
  }));
  print_row(measure("lu decomp      ", N, 1, reps, [&] {
    auto r = mc::math::linalg::decomp::lu<f64, N>(A);
    clobber(r.L.data);
    clobber(r.U.data);
  }));
  (void)sink;
}

};     // namespace

int
main(void)
{
  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(0);
  micron::posix::sched_setaffinity(0, sizeof(set), set);

  micron::io::println("=== micron BLAS / linalg benchmark ===");
  micron::io::println("L1 sizes: 64, 4 K, 64 K, 1 M, 4 M elements (covers L1/L2/LLC/RAM)");
  micron::io::println("L2 dims:  32, 128, 512, 1024 (square)");
  micron::io::println("L3 dims:  32, 128, 512 (square)");
  micron::io::println("perf events: cycles + instructions + branches + branch-misses");

  sweep_level1_f64();
  sweep_level1_f32();
  sweep_level2_f64();
  sweep_level3_f64();
  sweep_level3_f32();

  sweep_linalg_static<2>();
  sweep_linalg_static<3>();
  sweep_linalg_static<4>();
  sweep_linalg_static<8>();

  micron::io::println("");
  micron::io::println("=== done ===");
  return 0;
}
