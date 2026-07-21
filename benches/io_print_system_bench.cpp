//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/io/echo.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/std.hpp"

namespace
{

[[gnu::always_inline]] inline u64
now_ns() noexcept
{
  micron::timespec_t ts{};
  micron::clock_gettime(micron::clock_monotonic, ts);
  return static_cast<u64>(ts.tv_sec) * 1000000000ULL + static_cast<u64>(ts.tv_nsec);
}

u32
u_to_dec(u64 v, char *out) noexcept
{
  char t[24];
  u32 n = 0;
  if ( v == 0 )
    t[n++] = '0';
  else
    while ( v ) {
      t[n++] = static_cast<char>('0' + (v % 10));
      v /= 10;
    }
  for ( u32 i = 0; i < n; ++i ) out[i] = t[n - 1 - i];
  return n;
}

u64
parse_kv(const char *a, const char *key) noexcept
{
  const char *p = a;
  const char *k = key;
  while ( *k && *p == *k ) {
    ++p;
    ++k;
  }
  if ( *k ) return 0;
  u64 v = 0;
  while ( *p >= '0' && *p <= '9' ) v = v * 10 + static_cast<u64>(*p++ - '0');
  return v;
}

bool
str_eq(const char *a, const char *b) noexcept
{
  while ( *a && *a == *b ) {
    ++a;
    ++b;
  }
  return *a == *b;
}

}      // namespace

int
main(int argc, char **argv)
{
  u64 n = 100000;
  int mode = 0;
  const char *mode_name = "println";
  for ( int i = 1; i < argc; ++i ) {
    if ( str_eq(argv[i], "--mode=println") ) {
      mode = 0;
      mode_name = "println";
    } else if ( str_eq(argv[i], "--mode=echofd") ) {
      mode = 1;
      mode_name = "echofd";
    } else if ( str_eq(argv[i], "--mode=rawline") ) {
      mode = 2;
      mode_name = "rawline";
    } else if ( str_eq(argv[i], "--mode=rawbatch") ) {
      mode = 3;
      mode_name = "rawbatch";
    } else if ( u64 v = parse_kv(argv[i], "--n=") )
      n = v;
  }

  const micron::fd_t out = micron::io::stdout;
  const u64 t0 = now_ns();

  if ( mode == 0 ) {
    for ( u64 i = 0; i < n; ++i ) micron::io::println("log line number ", i, " some payload text");
    micron::io::fflush(out);
  } else if ( mode == 1 ) {
    for ( u64 i = 0; i < n; ++i ) micron::io::echo(out, "log line number ", i, " some payload text");
  } else if ( mode == 2 ) {
    char buf[96];
    for ( u64 i = 0; i < n; ++i ) {
      u32 p = 0;
      const char *a = "log line number ";
      while ( *a ) buf[p++] = *a++;
      p += u_to_dec(i, buf + p);
      const char *b = " some payload text\n";
      while ( *b ) buf[p++] = *b++;
      micron::posix::write(1, buf, p);
    }
  } else {
    char big[4096];
    u32 used = 0;
    char line[96];
    for ( u64 i = 0; i < n; ++i ) {
      u32 p = 0;
      const char *a = "log line number ";
      while ( *a ) line[p++] = *a++;
      p += u_to_dec(i, line + p);
      const char *b = " some payload text\n";
      while ( *b ) line[p++] = *b++;
      if ( used + p > sizeof(big) ) {
        micron::posix::write(1, big, used);
        used = 0;
      }
      for ( u32 k = 0; k < p; ++k ) big[used++] = line[k];
    }
    if ( used ) micron::posix::write(1, big, used);
  }

  const u64 t1 = now_ns();
  const f64 ns_per_line = static_cast<f64>(t1 - t0) / static_cast<f64>(n);

  char rep[160];
  u32 rp = 0;
  const char *pfx = "[io_print_system] mode=";
  while ( *pfx ) rep[rp++] = *pfx++;
  const char *mn = mode_name;
  while ( *mn ) rep[rp++] = *mn++;
  const char *nl = " n=";
  while ( *nl ) rep[rp++] = *nl++;
  rp += u_to_dec(n, rep + rp);
  const char *pl = " ns/line=";
  while ( *pl ) rep[rp++] = *pl++;
  rp += u_to_dec(static_cast<u64>(ns_per_line + 0.5), rep + rp);
  rep[rp++] = '\n';
  micron::posix::write(2, rep, rp);
  return 0;
}
