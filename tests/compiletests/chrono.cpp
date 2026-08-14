//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Compile coverage for the whole chrono module: built across the arch x opt x freestanding matrix,
// never run. The rows that matter most are -k and -ke, because the never-throwing readers exist
// precisely so a freestanding build has something it can call -- exc<> there is an abort().
//
// Also instantiated here: every serialisation policy the target supports, so the inline asm is
// actually emitted on all four arches rather than merely parsed.

#include "../../src/chrono.hpp"

#include "../../src/chrono/calendar.hpp"
#include "../../src/chrono/calibrate.hpp"
#include "../../src/chrono/civil.hpp"
#include "../../src/chrono/clock.hpp"
#include "../../src/chrono/cycles.hpp"
#include "../../src/chrono/epochs.hpp"
#include "../../src/chrono/format.hpp"
#include "../../src/chrono/measure.hpp"
#include "../../src/chrono/parse.hpp"
#include "../../src/chrono/posix.hpp"
#include "../../src/chrono/units.hpp"
#include "../../src/chrono/vdso.hpp"

// tz.hpp is not in the umbrella (it does file io), so it gets its own instantiation
#include "../../src/chrono/tz.hpp"

// utime / utimes / futimesat / lutimes are FILE functions and live next to utimensat, not in chrono
#include "../../src/linux/io/sys.hpp"

namespace ch = micron::chrono;
namespace pp = micron::posix;

namespace
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the constexpr surface, checked at compile time on every cell of the matrix

static_assert(ch::days_from_civil(1970, 1, 1) == 0);
static_assert(ch::days_from_civil(1969, 12, 31) == -1);
static_assert(ch::civil_from_days(0).y == 1970);
static_assert(ch::civil_from_days(-1).d == 31);
static_assert(ch::civil_from_days(-1).y == 1969);
static_assert(ch::is_leap(2000) && !ch::is_leap(1900) && ch::is_leap(2024));
static_assert(ch::days_in_month(2024, 2) == 29 && ch::days_in_month(2026, 2) == 28);
static_assert(!ch::civil_valid(2026, 2, 30));
static_assert(ch::weekday_of(1970, 1, 1) == 4);
static_assert(ch::day_of_year(2026, 1, 9) == 9);
static_assert(ch::iso_week_date(2026, 1, 9).week == 2);
static_assert(ch::floor_div(-1, 86400) == -1);
static_assert(ch::floor_mod(-1, 86400) == 86399);

static_assert(ch::civil_secs(ch::civil_utc(1768000000L)) == 1768000000L);
static_assert(ch::to_civil(0, 3600).h == 1);
static_assert(ch::from_civil(ch::civil_utc(1768000000L), ch::tz_utc) == 1768000000L);
static_assert(ch::offset_at(ch::tz_fixed(7200), 0) == 7200);

static_assert(ch::dur_ms{ 1500 }.count() == 1500);
static_assert(ch::duration_cast<ch::dur_s>(ch::dur_ms{ 2500 }).count() == 2);
static_assert(ch::duration_cast<ch::dur_ns>(ch::dur_s{ 3 }).count() == 3000000000LL);
static_assert(ch::ns_of_ts(ch::ts_of_ns(-1)) == -1);
static_assert(ch::ns_per_week == 604800000000000ull);

static_assert(ch::epoch::ntp_from_unix(0) == 2208988800L);
static_assert(ch::epoch::filetime_from_unix(0) == 116444736000000000L);
static_assert(ch::epoch::mjd(0) == 40587L);
static_assert(ch::epoch::jdn(0) == 2440588L);
static_assert(ch::epoch::unix_from_gps(ch::epoch::gps_from_unix(1768000000L)) == 1768000000L);

static_assert(ch::parse_iso8601("2026-08-14T14:22:11Z", 20).ok());
static_assert(!ch::parse_iso8601("2026-02-30", 10).ok());
static_assert(!ch::parse_iso8601("2026-8-14", 9).ok());
static_assert(ch::to_unix(ch::parse_iso8601("2026-08-14T14:22:11Z", 20)) == 1786717331L);
static_assert(ch::parse_duration_ns("1h30m", 5).ns == 5400000000000ull);
static_assert(ch::parse_duration_ns("500ms", 5).ns == 500000000ull);
static_assert(!ch::parse_duration_ns("90x", 3).ok());

static_assert(ch::rfc3339_size(ch::civil_utc(1768000000L), 0) == 20);
static_assert(ch::compact_size(ch::civil_utc(1768000000L), true) == 16);
static_assert(ch::compact_size(ch::civil_utc(1768000000L), false) == 15);
// a year outside 0..9999 is wider than the name, and the size has to say so
static_assert(ch::compact_size(ch::civil{ .y = 12026 }, true) == 17);

static_assert(ch::cycles_to_ns(3000000000ull, 3000000000ull) == 1000000000ull);
static_assert(ch::ns_to_cycles(1000000000ull, 3000000000ull) == 3000000000ull);

// the policy the target picked has to be one it can emit
static_assert(ch::serial_supported<ch::default_serial>);
static_assert(ch::serial_supported<ch::serial::none>);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the runtime surface: instantiated, never called

[[maybe_unused]] volatile u64 g_sink_u = 0;
[[maybe_unused]] volatile i64 g_sink_i = 0;
[[maybe_unused]] volatile double g_sink_f = 0;

void
use_clock(void)
{
  g_sink_i = ch::mono_ns();
  g_sink_i = ch::real_ns();
  g_sink_i = ch::raw_ns();
  g_sink_i = ch::boot_ns();
  g_sink_i = ch::cpu_ns();
  g_sink_i = ch::thread_cpu_ns();
  g_sink_i = ch::now_ns();
  g_sink_i = ch::now_us();
  g_sink_i = ch::now_ms();
  g_sink_i = ch::now_s();
  g_sink_i = ch::clock_resolution_ns(micron::clock_monotonic);
  g_sink_i = ch::sleep_until(micron::clock_monotonic, 0);
  g_sink_i = ch::sleep_until_once(micron::clock_monotonic, 0);
  g_sink_i = ch::sleep_ns(0);
  g_sink_i = ch::remaining_ns(0, 0);
  g_sink_i = ch::remaining_ms(0, 0);
  g_sink_i = ch::deadline::in_ms(1).at_ns;

  micron::steady_clock c;
  c.start();
  c.stop();
  g_sink_f = c.read<micron::unit::seconds>();
  g_sink_f = c.read_ds();
  g_sink_f = c.read_ms();
  g_sink_f = c.read_us();
  g_sink_f = c.read_ns();
  g_sink_f = micron::steady_clock::read_ms(c.begin_point(), c.end_point());
  g_sink_f = micron::steady_clock::read_ns(c.begin_point(), c.end_point());
  g_sink_i = c.delta_ns();
  g_sink_i = micron::steady_clock::now_ns();
  g_sink_f = c.lap();
  c.reset();

  micron::raw_clock rc;
  micron::boot_clock bc;
  micron::cpu_clock cc;
  micron::thread_cpu_clock tc;
  g_sink_f = rc.read_ms() + bc.read_ms() + cc.read_ms() + tc.read_ms();

  micron::fduration_t out = 0;
  {
    micron::auto_timer<micron::system_clocks::monotonic> t(&out);
  }
  i64 iout = 0;
  {
    micron::scoped_timer t(&iout);
  }
  g_sink_f = out;
  g_sink_i = iout;
}

void
use_format(void)
{
  char buf[256];
  const ch::civil c = ch::civil_utc(1768000000L);
  g_sink_u = ch::write_iso8601(buf, sizeof(buf), c, 3, ch::date_sep::space, ch::offset_style::colon);
  g_sink_u = ch::write_rfc3339(buf, sizeof(buf), c, 9);
  g_sink_u = ch::write_iso_full(buf, sizeof(buf), c, 9);
  g_sink_u = ch::write_rfc2822(buf, sizeof(buf), c);
  g_sink_u = ch::write_http_date(buf, sizeof(buf), c);
  g_sink_u = ch::write_asctime(buf, sizeof(buf), c);
  g_sink_u = ch::write_compact(buf, sizeof(buf), c);
  g_sink_u = ch::write_log_stamp(buf, sizeof(buf), c);
  g_sink_u = ch::write_strftime(buf, sizeof(buf), "%F %T %a %A %b %B %j %V %u %w %I %p %z %Z %s %c %x %X %e %k %C %D %g %G %R %r", c);
  g_sink_u = ch::strftime_size("%F", c);
  g_sink_u = ch::write_duration_hms(buf, sizeof(buf), 12345);
  g_sink_u = ch::write_duration_clock(buf, sizeof(buf), 12345, 100);
  g_sink_u = ch::write_duration_units(buf, sizeof(buf), 12345);

  g_sink_u = ch::parse_iso8601(buf, 8).ok() ? 1u : 0u;
  g_sink_u = ch::parse_rfc3339(buf, 8).ok() ? 1u : 0u;
  g_sink_u = ch::parse_rfc2822(buf, 8).ok() ? 1u : 0u;
  g_sink_u = ch::parse_until(buf, 8).ok() ? 1u : 0u;
  g_sink_u = ch::parse_duration_ns(buf, 8).ns;
  g_sink_i = ch::until_epoch(ch::parse_until("09:00", 5), 0, ch::tz_utc);
}

// WARNING: this has to be a TEMPLATE. `if constexpr` only discards a branch from EVALUATION -- in a
// plain function the discarded statement is still fully instantiated, so guarding an unsupported
// tick<S>() that way still fires its static_assert. Inside a template the branch is not instantiated
template<ch::serial S>
void
try_tick(void)
{
  if constexpr ( ch::serial_supported<S> ) {
    g_sink_u = ch::tick<S>();
    g_sink_u = ch::tick_start<S>();
    g_sink_u = ch::tick_end<S>();
  }
}

void
use_cycles(void)
{
  g_sink_u = ch::tick<ch::serial::none>();
  g_sink_u = ch::tick_start<>();
  g_sink_u = ch::tick_end<>();
  try_tick<ch::serial::lfence>();
  try_tick<ch::serial::mfence_lfence>();
  try_tick<ch::serial::cpuid>();
  try_tick<ch::serial::rdtscp>();
  try_tick<ch::serial::serialize>();
  try_tick<ch::serial::isb>();

  ch::fence_compiler();
  ch::fence_load();
  ch::fence_mem();
  ch::fence_store();
  ch::fence_serialising();

#if defined(__micron_arch_x86_any)
  u32 aux = 0;
  g_sink_u = ch::tick_aux(aux);
  g_sink_u = ch::aux_cpu(aux) + ch::aux_node(aux);
  g_sink_u = ch::pmc_delta(100, 1, 48);
#endif

  g_sink_u = ch::tick_hz();
  g_sink_u = ch::core_hz();
  g_sink_u = ch::calibrate_tick_hz(1000);
  g_sink_u = ch::calibrate_core_hz();
  g_sink_u = ch::core_hz_spread().median_hz;
  g_sink_u = ch::counter_traits().nominal_hz;
  g_sink_u = ch::ticks_to_ns(1000);
  g_sink_u = ch::ns_to_ticks(1000);
  g_sink_u = ch::timer_overhead<>().min_ticks;
  g_sink_u = ch::timer_resolution<>().min_ticks;
  g_sink_u = ch::clock_cost_ns(micron::clock_monotonic, 1);
  g_sink_u = ch::report().tick_hz;
  ch::set_tick_hz(1);
  ch::set_core_hz(1);
  ch::reset_core_hz();

  u64 v = 1;
  f64 d = 1.0;
  f32 f = 1.0f;
  ch::sink(v);
  ch::sink(d);
  ch::sink(f);
  ch::modify(v);
  ch::modify(d);
  ch::modify(f);
  ch::overwrite(v);
  ch::sink_ptr(&v);
  ch::clobber_memory();

  g_sink_u = ch::current_cpu();
  g_sink_u = ch::first_available_cpu();
  g_sink_i = ch::pin_to_cpu(0);
  g_sink_i = ch::pin_here();
  ch::warmup_ns(0);
  g_sink_i = ch::prepare(0, 0);
  g_sink_i = ch::prepare_here(0);

  u64 ticks = 0;
  {
    ch::tick_timer<> t(&ticks);
  }
  g_sink_u = ticks;
}

void
use_posix(void)
{
  micron::time_t t = 0;
  pp::tm_t tm{};
  pp::gmtime_r(&t, &tm);
  pp::localtime_r(&t, &tm, ch::tz_utc);
  g_sink_i = pp::timegm(&tm);
  g_sink_i = pp::mktime(&tm, ch::tz_utc);
  char ab[pp::asctime_buf];
  pp::asctime_r(&tm, ab);
  pp::ctime_r(&t, ab, ch::tz_utc);
  char buf[128];
  g_sink_u = pp::strftime(buf, sizeof(buf), "%F %T", &tm);
  g_sink_u = pp::strptime("2026-01-01", "%Y-%m-%d", &tm) != nullptr ? 1u : 0u;
  g_sink_i = pp::usleep(0);
  g_sink_i = pp::nsleep(0);
  g_sink_i = pp::clk_tck();
  g_sink_i = pp::sysconf(pp::_sc_pagesize);

  g_sink_f = micron::difftime(1, 0);
  g_sink_i = micron::tai_offset();
  micron::timex_t tx{};
  g_sink_i = micron::adjtimex(tx);
  g_sink_i = micron::clock_adjtime(micron::clock_realtime, tx);
  micron::timespec_t ts{};
  g_sink_i = micron::sched_rr_get_interval(0, ts);
  micron::timeval_t tv{};
  g_sink_i = micron::settimeofday(tv);
  g_sink_u = micron::alarm(0);

  pp::utimbuf_t ub{ 0, 0 };
  g_sink_i = pp::utime("/nonexistent", &ub);
  micron::timeval_t tvs[2] = {};
  g_sink_i = pp::utimes("/nonexistent", tvs);
  g_sink_i = pp::futimesat(-100, "/nonexistent", tvs);
  g_sink_i = pp::lutimes("/nonexistent", tvs);

  g_sink_u = micron::getauxval(micron::at_pagesz);
  g_sink_u = micron::auxval_has(micron::at_clktck) ? 1u : 0u;
}

void
use_vdso_and_tz(void)
{
  g_sink_u = ch::vdso::available() ? 1u : 0u;
  g_sink_u = ch::vdso::supported ? 1u : 0u;
  micron::timespec_t ts{};
  g_sink_i = ch::vdso::clock_gettime(micron::clock_monotonic, ts);

  static u8 scratch[1024];
  static ch::tz::tz_storage<8> store;
  g_sink_u = ch::tz::load_named("UTC", scratch, sizeof(scratch), store) ? 1u : 0u;
  g_sink_u = ch::tz::load_localtime(scratch, sizeof(scratch), store) ? 1u : 0u;
  g_sink_u = ch::tz::load_local(nullptr, scratch, sizeof(scratch), store) ? 1u : 0u;
  g_sink_u = ch::tz::parse(scratch, sizeof(scratch), store) ? 1u : 0u;
  g_sink_i = ch::offset_at(store.view(), 0);
}

// the legacy f64 surface, so nothing above quietly removed it
void
use_legacy(void)
{
  g_sink_f = micron::days(1.0) + micron::hours(1.0) + micron::minutes(1.0) + micron::seconds(1.0) + micron::milliseconds(1.0)
             + micron::microseconds(1.0) + micron::nanoseconds(1.0);
  g_sink_f = micron::now();
  g_sink_i = static_cast<i64>(micron::unix_time());
  g_sink_i = static_cast<i64>(micron::today().to_unix());
  g_sink_f = micron::time_of_day_now().to_duration();
  g_sink_f = micron::elapsed<micron::unit::seconds>(micron::now_ts(), micron::now_ts());
  g_sink_f = micron::timediff(0, 1);
  micron::time_point<micron::system_clock<>, micron::fduration_t> tp = micron::time_point<micron::system_clock<>, micron::fduration_t>::now();
  g_sink_f = tp.as<micron::unit::seconds>();
  g_sink_f = (tp + 1.0).time_since_epoch();
  micron::year_month_day ymd = micron::year_month_day::from_unix(0);
  g_sink_u = ymd.ok() ? 1u : 0u;
  g_sink_u = ymd.weekday() + ymd.day_of_year();
  g_sink_i = ymd.to_days();
}

};      // namespace

int
main(void)
{
  // never actually run -- this file is built by verify_compile.duck and graded on compiling
  if ( g_sink_u == 0xDEADBEEFull ) {
    use_clock();
    use_format();
    use_cycles();
    use_posix();
    use_vdso_and_tz();
    use_legacy();
  }
  return 1;
}
