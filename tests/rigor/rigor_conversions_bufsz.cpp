// rigor_conversions_bufsz.cpp -- the buffer-size contract of the float writers, and of every
// porcelain layered on top of them.
//
// conversions/fixed.hpp states the contract at its head: "every writer returns the byte count, or
// 0 when the result does not fit -- never a silently truncated string". That is a real
// improvement over the old `pos < buf_sz` clamping, but it made the CAPACITY ARGUMENT
// load-bearing at every call site, and six of them were not updated for it. This suite pins all
// of it at once:
//
//   1. the size guards must not overflow. `1u + prec` was computed in u32, so a precision of
//      0xFFFFFFFF wrapped `need` to 0, the `buf_sz < need` test passed, and the fraction loop ran
//      ~4G times into a 1400-byte stack buffer -- reachable from format("{:.4294967295f}", 1.0),
//      i.e. from a RUNTIME format string. verified SIGSEGV before the fix.
//   2. a writer never writes past the capacity it was given, and never reports more than it wrote.
//   3. d2f_size/d2e_size/d2g_size agree with what the writers actually produce, so a caller can
//      size a buffer from them. d2g_size is new: %g has no closed-form length (the trim depends
//      on the digits) and d2g_buffered used to validate the caller's capacity against the
//      UNTRIMMED width, refusing buffers that could hold the answer perfectly well.
//   4. the hstring porcelain (to_fixed and friends) is TOTAL -- it used to hand the writers a
//      stale literal 350 while its array was 1100, so any result in between came back 0 and
//      hstring(buf, buf+0) THREW: to_fixed(1e308, 45) aborted the process (SIGABRT).
//   5. the sstring porcelain truncates, as a fixed-capacity type always did, rather than
//      answering an empty string.
//   6. the two format frames that carry their own scratch -- io::echof's sink path and the
//      container/map/pair element adapter -- honour formatter<T>::buf_size like format_one does.
//      On the flat 72 both silently dropped any float field wider than that.
//
// oracle: the writers given a buffer that is always big enough. every layer above them must agree
// with that, byte for byte, or truncate it at a documented point.

#include "../../src/io/echo.hpp"
#include "../../src/string/format.hpp"
#include "../../src/string/strings.hpp"
#include "../../src/vector.hpp"

#include "../support/oracles.hpp"

using mtest::prng;
namespace ry = micron::__impl::__ryu;
using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

// the capacity probes below are quadratic-ish in the rendering width (each round re-runs the
// writer, and a %f of 1e308 at precision 400 is a 700-byte bignum expansion), so the counts are
// deliberately modest and the sweep probes the BOUNDARY capacities rather than every one of them
#if defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
constexpr static const usize N_FUZZ = 600;
#else
constexpr static const usize N_FUZZ = 6000;
#endif

// the widest any f64 rendering can be: 309 integer digits + '.' + 1074 fraction + sign
constexpr static const usize BIG = 1600;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// a canaried buffer. the writer is handed `cap`, and every byte from cap to the end of the array
// must still hold the canary afterwards -- that is what catches an overflowing size guard rather
// than merely a wrong return value.

struct canary_buf {
  static constexpr usize PAD = 64;
  char mem[BIG + PAD];

  char *
  arm() noexcept
  {
    for ( usize i = 0; i < BIG + PAD; ++i ) mem[i] = '\xA5';
    return mem;
  }

  bool
  intact_from(usize cap) const noexcept
  {
    for ( usize i = cap; i < BIG + PAD; ++i )
      if ( mem[i] != '\xA5' ) return false;
    return true;
  }
};

static f64
f64_from_bits(u64 b) noexcept
{
  f64 x;
  __builtin_memcpy(&x, &b, 8);
  return x;
}

// a spread of values that exercises every tier: the u64 fast path, the exact-decimal bignum, the
// specials, and the extremes where the integer part alone is 309 digits
static f64
pick_value(prng &rng, usize i) noexcept
{
  switch ( i % 8 ) {
  case 0:
    return static_cast<f64>(static_cast<i64>(rng.next() % 2000001ull) - 1000000);
  case 1:
    return f64_from_bits(rng.next());      // anything, including nan/inf/subnormal
  case 2:
    return 1e308;
  case 3:
    return -1e308;
  case 4:
    return 1.0 / 3.0;
  case 5:
    return f64_from_bits(rng.next() & 0x000FFFFFFFFFFFFFull);      // subnormal
  case 6:
    return static_cast<f64>(rng.next()) * 1e-17;
  default:
    return 0.0;
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// a sink that captures, so the echof path can be compared byte for byte instead of eyeballed

struct capture_sink {
  micron::hstring<schar> out;

  max_t
  put(const char *p, usize n)
  {
    out.append(p, n);
    return static_cast<max_t>(n);
  }

  max_t
  put(char c)
  {
    out += c;
    return 1;
  }

  max_t
  flush(void)
  {
    return 0;
  }
};

int
main()
{
  prng rng(0x5eed10bfc0ffee11ULL);

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("precision that overflows a u32 size guard is refused, not written");
  {
    // the exact reproducer. `1u + prec` wrapped to 0, `need` became 1, the guard passed, and the
    // emitter wrote ~4 GB past a 1400-byte stack array. this must be an empty field.
    require_true(micron::format::format("{:.4294967295f}", 1.0).size() == 0);
    require_true(micron::format::format("{:.4294967295e}", 1.0).size() == 0);
    require_true(micron::format::format("{:.4294967295a}", 1.0).size() == 0);
    require_true(micron::format::format("{:.4294967295g}", 1.0).size() == 0);

    // and directly, with a canary: nothing is written and 0 is reported
    canary_buf cb;
    for ( u32 p : { 0xFFFFFFFFu, 0xFFFFFFFEu, 0x80000000u, 0x7FFFFFFFu, 1000000u } ) {
      char *b = cb.arm();
      require_true(ry::d2f_buffered(1.0, b, 64, p) == 0);
      require_true(cb.intact_from(0));
      b = cb.arm();
      require_true(ry::d2e_buffered(1.0, b, 64, p) == 0);
      require_true(cb.intact_from(0));
      b = cb.arm();
      require_true(ry::d2a_buffered(1.0, b, 64, p, true, false) == 0);
      require_true(cb.intact_from(0));
      // the size queries must not wrap either -- a caller sizing a buffer from one of these
      // would otherwise allocate ~0 bytes and then be told the write fits
      require_true(ry::d2f_size(1.0, p) > p);
      require_true(ry::d2e_size(1.0, p) > p);
    }

    // parse_spec saturates rather than wrapping, so an absurd digit run cannot land on a value
    // that happens to overflow downstream
    require_true(micron::format::format("{:.99999999999999f}", 1.0).size() == 0);
    require_true(micron::format::format("{:.000000000000006f}", 1.5).size() == 8);      // leading zeros still parse
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("fuzz: writers never exceed their capacity, and 0 means 'did not fit'");
  {
    canary_buf cb;
    char ref[BIG];
    for ( usize i = 0; i < N_FUZZ; ++i ) {
      const f64 v = pick_value(rng, i);
      const u32 prec = static_cast<u32>(rng.next() % 250ull);

      // what the writer produces when it certainly has room -- the oracle for this round
      const usize want_f = ry::d2f_buffered(v, ref, BIG, prec);
      require_true(want_f != 0);
      require_true(want_f <= ry::d2f_size(v, prec));

      // the boundary capacities plus a random interior one: below want_f it must refuse and write
      // nothing, at or above it must produce exactly the oracle bytes
      const usize caps[] = { 0u, 1u, want_f / 2u, want_f - 1u, want_f, want_f + 1u, BIG - 1u,
                             static_cast<usize>(rng.next() % (want_f + 2u)) };
      for ( usize cap : caps ) {
        if ( cap >= BIG ) continue;
        char *b = cb.arm();
        const usize n = ry::d2f_buffered(v, b, cap, prec);
        require_true(cb.intact_from(cap));      // never past the capacity, whatever it answered
        if ( cap < want_f ) {
          require_true(n == 0);
        } else {
          require_true(n == want_f);
          for ( usize k = 0; k < n; ++k ) require_true(b[k] == ref[k]);
        }
      }
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("fuzz: d2e / d2g size queries agree with the writers");
  {
    canary_buf cb;
    char ref[BIG];
    for ( usize i = 0; i < N_FUZZ; ++i ) {
      const f64 v = pick_value(rng, i);
      const u32 prec = static_cast<u32>(rng.next() % 200ull);

      const usize want_e = ry::d2e_buffered(v, ref, BIG, prec);
      require_true(want_e != 0);
      require_true(want_e <= ry::d2e_size(v, prec));

      char *b = cb.arm();
      require_true(ry::d2e_buffered(v, b, want_e - 1, prec) == 0);
      require_true(cb.intact_from(want_e - 1));
      b = cb.arm();
      require_true(ry::d2e_buffered(v, b, want_e, prec) == want_e);
      require_true(cb.intact_from(want_e));

      // %g: the answer is the TRIMMED text, so the capacity test has to be against that. this is
      // the whole of finding 10 -- d2g_buffered sized on the pre-trim width and refused buffers
      // that could hold the result, and there was no d2g_size to ask instead.
      for ( bool alt : { false, true } ) {
        const usize want_g = ry::d2g_buffered(v, ref, BIG, prec, alt, false);
        require_true(want_g == ry::d2g_size(v, prec, alt));
        require_true(want_g != 0);
        b = cb.arm();
        require_true(ry::d2g_buffered(v, b, want_g, prec, alt, false) == want_g);
        require_true(cb.intact_from(want_g));
        for ( usize k = 0; k < want_g; ++k ) require_true(b[k] == ref[k]);
        if ( want_g > 0 ) {
          b = cb.arm();
          require_true(ry::d2g_buffered(v, b, want_g - 1, prec, alt, false) == 0);
          require_true(cb.intact_from(want_g - 1));
        }
      }
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("d2g accepts any buffer that fits the trimmed answer");
  {
    // the exact reproducer: "1.500000000" is 11 bytes before the trim, "1.5" is 3 after it. every
    // capacity from 3 up must work; only 0..2 may refuse.
    char b[64];
    for ( usize cap = 0; cap <= 16; ++cap ) {
      const usize n = micron::to_chars(b, cap, 1.5, micron::float_format::general, 10);
      if ( cap < 3 )
        require_true(n == 0);
      else {
        require_true(n == 3);
        require_true(b[0] == '1' && b[1] == '.' && b[2] == '5');
      }
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("the hstring porcelain is total -- these used to abort");
  {
    // each of these needs more than the stale 350 the wrappers used to advertise, so each threw
    // micron::except::library_error out of hstring(buf, buf+0) and killed the process
    require_true(micron::to_fixed(1e308, 45).size() == 355);
    require_true(micron::to_fixed(1.0 / 3.0, 349).size() == 351);
    require_true(micron::to_fixed(1.0 / 3.0, 348).size() == 350);      // the last one that worked before
    require_true(micron::to_scientific(1e308, 500).size() == 507);
    require_true(micron::double_to_string(1e308, 45).size() == 355);
    require_true(micron::float_to_string(1e38f, 60).size() == 99);
    require_true(micron::to_fixed_trim(1.5, 400).size() == 3);

    // past even the 1100-byte scratch: a legal request (glibc prints it), so it takes the heap
    // path rather than losing the value
    require_true(micron::to_fixed(1.0, 2000).size() == 2002);
    require_true(micron::to_fixed(1.0, 5000).size() == 5002);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("fuzz: hstring porcelain == the writer's own answer");
  {
    char ref[BIG];
    for ( usize i = 0; i < N_FUZZ / 2; ++i ) {
      const f64 v = pick_value(rng, i);
      const u32 prec = static_cast<u32>(rng.next() % 900ull);

      const usize want = ry::d2f_buffered(v, ref, BIG, prec);
      micron::hstring<char> s = micron::to_fixed(v, prec);
      require_true(s.size() == want);
      for ( usize k = 0; k < want; ++k ) require_true(s[k] == ref[k]);

      const usize wante = ry::d2e_buffered(v, ref, BIG, prec);
      micron::hstring<char> se = micron::to_scientific(v, prec);
      require_true(se.size() == wante);
      for ( usize k = 0; k < wante; ++k ) require_true(se[k] == ref[k]);
    }
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("fuzz: sstring porcelain truncates, never empties");
  {
    char ref[BIG];
    for ( usize i = 0; i < N_FUZZ / 4; ++i ) {
      const f64 v = pick_value(rng, i);
      const u32 prec = static_cast<u32>(rng.next() % 120ull);

      const usize want = ry::d2f_buffered(v, ref, BIG, prec);
      const usize cap = 64;
      micron::sstring<cap, char> s = micron::to_fixed_stack<cap, char>(v, prec);
      const usize expect = want > cap - 1 ? cap - 1 : want;
      require_true(s.len() == expect);
      for ( usize k = 0; k < expect; ++k ) require_true(s[k] == ref[k]);
    }

    // the named reproducers: 63 characters of the value, not an empty string
    require_true(micron::to_fixed_stack(1e100, 6u).len() == 63);
    require_true(micron::to_scientific_stack(1e100, 90u).len() == 79);
    require_true(micron::to_string_stack(1e300, 6u).len() == 63);
    require_true(micron::to_string_stack(1e38f, 10u).len() == 47);
    require_true(micron::double_to_string_stack(1e300, 6u).len() == 63);
    require_true(micron::to_fixed_trim_stack(1.5, 400u).len() == 3);
    require_true(micron::to_general_stack(1e100, 6u).len() > 0);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("echof's sink path carries the per-formatter scratch");
  {
    // format() and the echo sink are two independent copies of the same frame; they must agree.
    // the sink used to declare a flat 72-byte buffer, so every float field wider than that
    // vanished from stdout while format() rendered it correctly.
    const char *specs[] = { "{:.6f}", "{:.100f}", "{:.20e}", "{:.6g}", "{:a}" };
    const f64 vals[] = { 1e300, 1.0, -1e-300, 1.0 / 3.0, 1e308 };
    for ( const char *sp : specs ) {
      for ( f64 v : vals ) {
        capture_sink cs;
        micron::io::__echo_impl::format_to_sink(cs, sp, v);
        micron::hstring<schar> want = micron::format::format(sp, v);
        require_true(cs.out.size() == want.size());
        for ( usize k = 0; k < want.size(); ++k ) require_true(cs.out[k] == want[k]);
        require_true(want.size() > 0);
      }
    }
    // the headline case: 308 characters, not an empty field
    capture_sink cs;
    micron::io::__echo_impl::format_to_sink(cs, "{:.6f}", 1e300);
    require_true(cs.out.size() == 308);
  }
  end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("container elements carry the per-formatter scratch");
  {
    // fmt_element is the third copy of the frame. on the flat 72 a vector of large doubles under
    // {:.6f} printed "{  }" -- the element gone entirely.
    micron::vector<double> v;
    v.push_back(1e300);
    micron::hstring<schar> got = micron::format::format("{:.6f}", v);
    micron::hstring<schar> scalar = micron::format::format("{:.6f}", 1e300);
    require_true(got.size() > scalar.size());      // the element plus the braces

    micron::vector<double> two;
    two.push_back(1e300);
    two.push_back(-1e300);
    require_true(micron::format::format("{:.6f}", two).size() > 2 * scalar.size());

    micron::pair<double, double> pr{ 1e300, 1.0 };
    require_true(micron::format::format("{:.6f}", pr).size() > scalar.size());
  }
  end_test_case();

  print("=== CONVERSIONS BUFSZ RIGOR SUITE PASSED ===");
  return 1;
}
