// Copyright (c) 2026 David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
#include "../src/chrono/clock.hpp"
#include "../src/io/console.hpp"
#include "../src/math/blas/blas.hpp"
#include "../src/memory/allocation/abcmalloc/stats.hpp"

namespace mx = micron::math::blas::mixed;
using micron::math::bfloat16;
using micron::math::float16;
template<class T> using buffer = micron::vector<T, micron::allocator_serial<>, false>;

template<class Work, class Output>
bool
measure(const char *name, usize rows, u64 setup_ns, usize bytes, u64 duration, Work work, Output &out)
{
  if ( !work() ) return false;
  const auto before = abc::stats();
  const u64 start = micron::chrono::mono_ns();
  usize calls{};
  u64 elapsed{};
  do {
    if ( !work() ) return false;
    __asm__ volatile("" : : "r"(out.data()) : "memory");
    ++calls;
    elapsed = micron::chrono::mono_ns() - start;
  } while ( elapsed < duration );
  const auto after = abc::stats();
  micron::io::println("type=", name, " rows=", rows, " ns_per_call=", f64(elapsed) / f64(calls), " calls=", calls, " setup_ns=", setup_ns,
                      " storage_bytes=", bytes, " allocs=", after.alloc_requests - before.alloc_requests,
                      " frees=", after.dealloc_requests - before.dealloc_requests, " checksum=", f64(out[0]) + f64(out[out.size() - 1]));
  return before.enabled && before.alloc_requests == after.alloc_requests && before.dealloc_requests == after.dealloc_requests;
}

template<class W>
f64
value(W w)
{
  if constexpr ( mx::accumulator<W> )
    return f64(w);
  else
    return f64(w.to_float());
}

template<class W>
bool
floating(const char *name, usize rows, u64 duration)
{
  const u64 start = micron::chrono::mono_ns();
  buffer<f32> a(rows * 64), out(rows * 64);
  buffer<W> b(64 * 64);
  micron::math::matrix::pack::workspace<f32> panels;
  micron::math::matrix::pack::scoped_workspace<f32> bound(panels);
  for ( usize i = 0; i < a.size(); ++i ) a[i] = f32(i32((i * 3) % 37) - 18) / f32(19);
  for ( usize i = 0; i < b.size(); ++i ) b[i] = W(f32(i32((i * 7) % 29) - 14) / f32(17));
  const u64 setup = micron::chrono::mono_ns() - start;
  auto work = [&] {
    if constexpr ( micron::is_same_v<W, f32> ) {
      micron::math::blas::level3::gemm_row(false, true, rows, 64, 64, f32(1), a.data(), 64, b.data(), 64, f32(0), out.data(), 64);
      return true;
    } else
      return mx::gemm_row(false, true, rows, 64, 64, f32(1), a.data(), 64, b.data(), 64, f32(0), out.data(), 64) == mx::status::ok;
  };
  if ( !work() ) return false;
  for ( usize i = 0; i < rows; ++i )
    for ( usize j = 0; j < 64; ++j ) {
      f64 expected{};
      for ( usize z = 0; z < 64; ++z ) expected += f64(a[i * 64 + z]) * value(b[j * 64 + z]);
      f64 d = f64(out[i * 64 + j]) - expected;
      if ( (d < 0 ? -d : d) > f64(2e-5) ) return false;
    }
  return measure(name, rows, setup, 64 * 64 * sizeof(W), duration, work, out);
}

template<class Acc>
bool
integer(const char *name, usize rows, u64 duration)
{
  const u64 start = micron::chrono::mono_ns();
  buffer<i8> a(rows * 64), b(64 * 64);
  buffer<Acc> out(rows * 64);
  for ( usize i = 0; i < a.size(); ++i ) a[i] = i8(i32((i * 3) % 256) - 128);
  for ( usize i = 0; i < b.size(); ++i ) b[i] = i8(i32((i * 7) % 256) - 128);
  const u64 setup = micron::chrono::mono_ns() - start;
  auto work
      = [&] { return mx::gemm_i8_row(false, true, rows, 64, 64, a.data(), 64, -3, b.data(), 64, 0, out.data(), 64) == mx::status::ok; };
  if ( !work() ) return false;
  for ( usize i = 0; i < rows; ++i )
    for ( usize j = 0; j < 64; ++j ) {
      i64 expected{};
      for ( usize z = 0; z < 64; ++z ) expected += (i64(a[i * 64 + z]) + 3) * i64(b[j * 64 + z]);
      if ( out[i * 64 + j] != expected ) return false;
    }
  return measure(name, rows, setup, 64 * 64, duration, work, out);
}

int
main(int argc, char **argv)
{
  u64 milliseconds = 100;
  if ( argc == 2 ) {
    milliseconds = 0;
    for ( const char *p = argv[1]; *p; ++p ) {
      if ( *p < '0' || *p > '9' ) return 2;
      milliseconds = milliseconds * 10 + u64(*p - '0');
    }
  }
  const u64 duration = milliseconds * 1000000;
  for ( usize n : { usize(1), usize(32), usize(512) } )
    if ( !floating<f32>("f32", n, duration) || !floating<float16>("f16", n, duration) || !floating<bfloat16>("bf16", n, duration)
         || !integer<i32>("i8-i32", n, duration) || !integer<i64>("i8-i64", n, duration) )
      return 2;
  return 0;
}
