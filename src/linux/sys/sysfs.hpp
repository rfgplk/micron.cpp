//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../string/sstring.hpp"
#include "../../string/unistring.hpp"
#include "../../syscall.hpp"
#include "../../types.hpp"
#include "../io/sys.hpp"
#include "../sys/fcntl.hpp"
#include "../sys/types.hpp"

namespace micron
{
namespace posix
{

namespace sysfs
{

namespace __impl
{

constexpr static const usize __sysfs_buf_sz = 128;
constexpr static const usize __sysfs_path_sz = 192;

using path_t = micron::sstring<__sysfs_path_sz, char>;
using buf_t = micron::sstring<__sysfs_buf_sz, char>;

inline i32
__open_rd(const char *path)
{
  return posix::openat(at_fdcwd, path, o_rdonly | o_cloexec, 0);
}

inline i32
__open_wr(const char *path)
{
  return posix::openat(at_fdcwd, path, o_wronly | o_cloexec, 0);
}

inline void
__close(i32 fd)
{
  posix::close(fd);
}

inline usize
__read_node(const char *path, char *buf, usize cap)
{
  buf[0] = '\0';
  i32 fd = __open_rd(path);
  if ( fd < 0 )
    return 0;
  max_t n = posix::read(fd, buf, cap - 1);
  __close(fd);
  if ( n <= 0 ) {
    buf[0] = '\0';
    return 0;
  }
  buf[n] = '\0';
  // strip trailing newline
  if ( n > 0 && buf[n - 1] == '\n' ) {
    buf[n - 1] = '\0';
    --n;
  }
  return static_cast<usize>(n);
}

inline bool
__write_node(const char *path, const char *data, usize len)
{
  i32 fd = __open_wr(path);
  if ( fd < 0 )
    return false;
  posix::write(fd, const_cast<char *>(data), len);
  __close(fd);
  return true;
}

inline u64
__parse_u64(const char *buf)
{
  u64 val = 0;
  const char *p = buf;
  while ( *p == ' ' || *p == '\t' || *p == '\n' )
    ++p;
  while ( *p >= '0' && *p <= '9' )
    val = val * 10 + static_cast<u64>(*p++ - '0');
  return val;
}

inline u32
__parse_range_count(const char *buf)
{
  const char *p = buf;
  while ( *p == ' ' || *p == '\t' )
    ++p;
  const char *dash = p;
  while ( *dash && *dash != '-' )
    ++dash;
  if ( *dash == '-' ) {
    ++dash;
    u32 hi = 0;
    while ( *dash >= '0' && *dash <= '9' )
      hi = hi * 10 + static_cast<u32>(*dash++ - '0');
    return hi + 1;
  }
  u32 val = 0;
  while ( *p >= '0' && *p <= '9' )
    val = val * 10 + static_cast<u32>(*p++ - '0');
  return val + 1;
}

inline u64
__parse_size(const char *buf)
{
  u64 val = 0;
  const char *p = buf;
  while ( *p >= '0' && *p <= '9' )
    val = val * 10 + static_cast<u64>(*p++ - '0');
  if ( *p == 'K' || *p == 'k' )
    val *= 1024ULL;
  else if ( *p == 'M' || *p == 'm' )
    val *= 1024ULL * 1024ULL;
  else if ( *p == 'G' || *p == 'g' )
    val *= 1024ULL * 1024ULL * 1024ULL;
  return val;
}

inline path_t
__cpu_path(u32 cpu_id, const char *suffix)
{
  path_t p;
  p += "/sys/devices/system/cpu/cpu";
  p += micron::int_to_string_stack<u32, char, 12>(cpu_id);
  p += suffix;
  return p;
}

inline path_t
__cache_path(u32 cpu_id, u32 index, const char *attr)
{
  path_t p;
  p += "/sys/devices/system/cpu/cpu";
  p += micron::int_to_string_stack<u32, char, 12>(cpu_id);
  p += "/cache/index";
  p += micron::int_to_string_stack<u32, char, 12>(index);
  p += "/";
  p += attr;
  return p;
}

};     // namespace __impl

template <usize N = 128>
inline micron::sstring<N, char>
read_str(const char *path)
{
  micron::sstring<N, char> result;
  char buf[N];
  usize n = __impl::__read_node(path, buf, N);
  char *out = &result[0];
  for ( usize i = 0; i < n; i++ )
    out[i] = buf[i];
  out[n] = '\0';
  result._buf_set_length(n);
  return result;
}

inline u64
read_u64(const char *path)
{
  char buf[64];
  __impl::__read_node(path, buf, 64);
  return __impl::__parse_u64(buf);
}

inline i64
read_i64(const char *path)
{
  char buf[64];
  usize n = __impl::__read_node(path, buf, 64);
  if ( n == 0 )
    return 0;
  const char *p = buf;
  bool neg = false;
  if ( *p == '-' ) {
    neg = true;
    ++p;
  }
  u64 val = __impl::__parse_u64(p);
  return neg ? -static_cast<i64>(val) : static_cast<i64>(val);
}

inline bool
read_bool(const char *path)
{
  char buf[8];
  __impl::__read_node(path, buf, 8);
  return buf[0] == '1' || buf[0] == 'Y' || buf[0] == 'y';
}

inline u64
read_size(const char *path)
{
  char buf[32];
  __impl::__read_node(path, buf, 32);
  return __impl::__parse_size(buf);
}

inline bool
write_str(const char *path, const char *val)
{
  usize len = 0;
  while ( val[len] )
    ++len;
  return __impl::__write_node(path, val, len);
}

inline bool
write_u64(const char *path, u64 val)
{
  auto s = micron::int_to_string_stack<u64, char, 24>(val);
  return __impl::__write_node(path, &s[0], s.size());
}

inline bool
write_bool(const char *path, bool val)
{
  char c = val ? '1' : '0';
  return __impl::__write_node(path, &c, 1);
}

inline bool
exists(const char *path)
{
  i32 fd = __impl::__open_rd(path);
  if ( fd < 0 )
    return false;
  __impl::__close(fd);
  return true;
}

namespace cpu
{

inline u64
scaling_cur_freq(u32 cpu_id)
{
  auto p = __impl::__cpu_path(cpu_id, "/cpufreq/scaling_cur_freq");
  return read_u64(&p[0]);
}

inline u64
cpuinfo_cur_freq(u32 cpu_id)
{
  auto p = __impl::__cpu_path(cpu_id, "/cpufreq/cpuinfo_cur_freq");
  return read_u64(&p[0]);
}

inline u64
cpuinfo_min_freq(u32 cpu_id)
{
  auto p = __impl::__cpu_path(cpu_id, "/cpufreq/cpuinfo_min_freq");
  return read_u64(&p[0]);
}

inline u64
cpuinfo_max_freq(u32 cpu_id)
{
  auto p = __impl::__cpu_path(cpu_id, "/cpufreq/cpuinfo_max_freq");
  return read_u64(&p[0]);
}

inline u64
scaling_min_freq(u32 cpu_id)
{
  auto p = __impl::__cpu_path(cpu_id, "/cpufreq/scaling_min_freq");
  return read_u64(&p[0]);
}

inline u64
scaling_max_freq(u32 cpu_id)
{
  auto p = __impl::__cpu_path(cpu_id, "/cpufreq/scaling_max_freq");
  return read_u64(&p[0]);
}

inline bool
set_scaling_min_freq(u32 cpu_id, u64 khz)
{
  auto p = __impl::__cpu_path(cpu_id, "/cpufreq/scaling_min_freq");
  return write_u64(&p[0], khz);
}

inline bool
set_scaling_max_freq(u32 cpu_id, u64 khz)
{
  auto p = __impl::__cpu_path(cpu_id, "/cpufreq/scaling_max_freq");
  return write_u64(&p[0], khz);
}

inline u64
cur_freq_hz(u32 cpu_id)
{
  return scaling_cur_freq(cpu_id) * 1000ULL;
}

inline u64
min_freq_hz(u32 cpu_id)
{
  return cpuinfo_min_freq(cpu_id) * 1000ULL;
}

inline u64
max_freq_hz(u32 cpu_id)
{
  return cpuinfo_max_freq(cpu_id) * 1000ULL;
}

inline bool
set_min_freq_hz(u32 cpu_id, u64 hz)
{
  return set_scaling_min_freq(cpu_id, hz / 1000);
}

inline bool
set_max_freq_hz(u32 cpu_id, u64 hz)
{
  return set_scaling_max_freq(cpu_id, hz / 1000);
}

template <usize N = 32>
inline micron::sstring<N, char>
scaling_governor(u32 cpu_id)
{
  auto p = __impl::__cpu_path(cpu_id, "/cpufreq/scaling_governor");
  return read_str<N>(&p[0]);
}

inline bool
set_scaling_governor(u32 cpu_id, const char *gov)
{
  auto p = __impl::__cpu_path(cpu_id, "/cpufreq/scaling_governor");
  return write_str(&p[0], gov);
}

template <usize N = 256>
inline micron::sstring<N, char>
scaling_available_governors(u32 cpu_id)
{
  auto p = __impl::__cpu_path(cpu_id, "/cpufreq/scaling_available_governors");
  return read_str<N>(&p[0]);
}

template <usize N = 32>
inline micron::sstring<N, char>
scaling_driver(u32 cpu_id)
{
  auto p = __impl::__cpu_path(cpu_id, "/cpufreq/scaling_driver");
  return read_str<N>(&p[0]);
}

template <usize N = 32>
inline micron::sstring<N, char>
energy_performance_preference(u32 cpu_id)
{
  auto p = __impl::__cpu_path(cpu_id, "/cpufreq/energy_performance_preference");
  return read_str<N>(&p[0]);
}

inline bool
set_energy_performance_preference(u32 cpu_id, const char *pref)
{
  auto p = __impl::__cpu_path(cpu_id, "/cpufreq/energy_performance_preference");
  return write_str(&p[0], pref);
}

inline bool
set_governor_all(const char *gov, u32 ncpus)
{
  bool ok = true;
  for ( u32 i = 0; i < ncpus; i++ )
    ok &= set_scaling_governor(i, gov);
  return ok;
}

inline bool
set_performance_all(u32 ncpus)
{
  return set_governor_all("performance", ncpus);
}

inline bool
set_powersave_all(u32 ncpus)
{
  return set_governor_all("powersave", ncpus);
}

inline bool
is_online(u32 cpu_id)
{
  if ( cpu_id == 0 )
    return true;
  auto p = __impl::__cpu_path(cpu_id, "/online");
  return read_bool(&p[0]);
}

inline bool
set_online(u32 cpu_id, bool on)
{
  if ( cpu_id == 0 )
    return false;
  auto p = __impl::__cpu_path(cpu_id, "/online");
  return write_bool(&p[0], on);
}

inline u32
present_count()
{
  char buf[32];
  __impl::__read_node("/sys/devices/system/cpu/present", buf, 32);
  return __impl::__parse_range_count(buf);
}

inline u32
online_count()
{
  char buf[64];
  __impl::__read_node("/sys/devices/system/cpu/online", buf, 64);
  return __impl::__parse_range_count(buf);
}

inline u32
possible_count()
{
  char buf[32];
  __impl::__read_node("/sys/devices/system/cpu/possible", buf, 32);
  return __impl::__parse_range_count(buf);
}

inline u32
physical_package_id(u32 cpu_id)
{
  auto p = __impl::__cpu_path(cpu_id, "/topology/physical_package_id");
  return static_cast<u32>(read_u64(&p[0]));
}

inline u32
core_id(u32 cpu_id)
{
  auto p = __impl::__cpu_path(cpu_id, "/topology/core_id");
  return static_cast<u32>(read_u64(&p[0]));
}

template <usize N = 64>
inline micron::sstring<N, char>
thread_siblings_list(u32 cpu_id)
{
  auto p = __impl::__cpu_path(cpu_id, "/topology/thread_siblings_list");
  return read_str<N>(&p[0]);
}

template <usize N = 64>
inline micron::sstring<N, char>
core_siblings_list(u32 cpu_id)
{
  auto p = __impl::__cpu_path(cpu_id, "/topology/core_siblings_list");
  return read_str<N>(&p[0]);
}

namespace cache
{

constexpr static const u16 no_cache = 0;
constexpr static const u16 data_cache = 1;
constexpr static const u16 instruction_cache = 2;
constexpr static const u16 unified_cache = 3;

struct info_t {
  u16 type;
  u16 level;
  u32 line_size;
  u32 associativity;
  u32 sets;
  u64 size;
};

template <usize N = 32>
inline micron::sstring<N, char>
type_str(u32 cpu_id, u32 index)
{
  auto p = __impl::__cache_path(cpu_id, index, "type");
  return read_str<N>(&p[0]);
}

inline u32
level(u32 cpu_id, u32 index)
{
  auto p = __impl::__cache_path(cpu_id, index, "level");
  return static_cast<u32>(read_u64(&p[0]));
}

inline u32
coherency_line_size(u32 cpu_id, u32 index)
{
  auto p = __impl::__cache_path(cpu_id, index, "coherency_line_size");
  return static_cast<u32>(read_u64(&p[0]));
}

inline u32
ways_of_associativity(u32 cpu_id, u32 index)
{
  auto p = __impl::__cache_path(cpu_id, index, "ways_of_associativity");
  return static_cast<u32>(read_u64(&p[0]));
}

inline u32
number_of_sets(u32 cpu_id, u32 index)
{
  auto p = __impl::__cache_path(cpu_id, index, "number_of_sets");
  return static_cast<u32>(read_u64(&p[0]));
}

inline u64
size(u32 cpu_id, u32 index)
{
  auto p = __impl::__cache_path(cpu_id, index, "size");
  return read_size(&p[0]);
}

inline u16
__parse_type(const char *s)
{
  if ( s[0] == 'D' )
    return data_cache;
  if ( s[0] == 'I' )
    return instruction_cache;
  if ( s[0] == 'U' )
    return unified_cache;
  return no_cache;
}

inline info_t
read_info(u32 cpu_id, u32 index)
{
  info_t c;
  auto ts = type_str(cpu_id, index);
  c.type = __parse_type(&ts[0]);

  if ( c.type == no_cache ) {
    c.level = 0;
    c.line_size = 0;
    c.associativity = 0;
    c.sets = 0;
    c.size = 0;
    return c;
  }

  c.level = static_cast<u16>(level(cpu_id, index));
  c.line_size = coherency_line_size(cpu_id, index);
  c.associativity = ways_of_associativity(cpu_id, index);
  c.sets = number_of_sets(cpu_id, index);
  c.size = size(cpu_id, index);

  if ( c.size == 0 && c.line_size > 0 )
    c.size = static_cast<u64>(c.line_size) * c.associativity * c.sets;

  return c;
}

constexpr static const u32 max_indices = 8;

inline u32
read_all(u32 cpu_id, info_t *out, u32 max_n = max_indices)
{
  u32 count = 0;
  for ( u32 i = 0; i < max_n; i++ ) {
    auto p = __impl::__cache_path(cpu_id, i, "type");
    if ( !exists(&p[0]) )
      break;
    out[count++] = read_info(cpu_id, i);
  }
  return count;
}

inline u32
l1d_line_size(u32 cpu_id = 0)
{
  for ( u32 i = 0; i < max_indices; i++ ) {
    info_t c = read_info(cpu_id, i);
    if ( c.type == data_cache && c.level == 1 )
      return c.line_size;
    if ( c.type == no_cache )
      break;
  }
  return 64;
}

};     // namespace cache

template <usize N = 32>
inline micron::sstring<N, char>
cpuidle_governor(u32 cpu_id)
{
  auto p = __impl::__cpu_path(cpu_id, "/cpuidle/current_governor");
  return read_str<N>(&p[0]);
}

inline bool
set_cpuidle_governor(u32 cpu_id, const char *gov)
{
  auto p = __impl::__cpu_path(cpu_id, "/cpuidle/current_governor");
  return write_str(&p[0], gov);
}

namespace thermal
{

inline __impl::path_t
__thermal_path(u32 zone, const char *attr)
{
  __impl::path_t p;
  p += "/sys/class/thermal/thermal_zone";
  p += micron::int_to_string_stack<u32, char, 12>(zone);
  p += "/";
  p += attr;
  return p;
}

inline i64
temp(u32 zone = 0)
{
  auto p = __thermal_path(zone, "temp");
  return read_i64(&p[0]);
}

inline i32
temp_c(u32 zone = 0)
{
  return static_cast<i32>(temp(zone) / 1000);
}

template <usize N = 32>
inline micron::sstring<N, char>
type(u32 zone = 0)
{
  auto p = __thermal_path(zone, "type");
  return read_str<N>(&p[0]);
}

};     // namespace thermal

namespace power
{

inline __impl::path_t
__powercap_path(u32 zone, const char *attr)
{
  __impl::path_t p;
  p += "/sys/class/powercap/intel-rapl:";
  p += micron::int_to_string_stack<u32, char, 12>(zone);
  p += "/";
  p += attr;
  return p;
}

inline u64
energy_uj(u32 zone = 0)
{
  auto p = __powercap_path(zone, "energy_uj");
  return read_u64(&p[0]);
}

inline u64
max_energy_uj(u32 zone = 0)
{
  auto p = __powercap_path(zone, "max_energy_range_uj");
  return read_u64(&p[0]);
}

template <usize N = 64>
inline micron::sstring<N, char>
name(u32 zone = 0)
{
  auto p = __powercap_path(zone, "name");
  return read_str<N>(&p[0]);
}

};     // namespace power

template <usize N = 128>
inline micron::sstring<N, char>
cpu_vulnerability(const char *vuln)
{
  __impl::path_t p;
  p += "/sys/devices/system/cpu/vulnerabilities/";
  p += vuln;
  return read_str<N>(&p[0]);
}

inline u32
aslr_level()
{
  return static_cast<u32>(read_u64("/proc/sys/kernel/randomize_va_space"));
}

template <usize N = 64>
inline micron::sstring<N, char>
thp_enabled()
{
  return read_str<N>("/sys/kernel/mm/transparent_hugepage/enabled");
}

template <usize N = 32>
inline micron::sstring<N, char>
microcode_version(u32 cpu_id = 0)
{
  auto p = __impl::__cpu_path(cpu_id, "/microcode/version");
  return read_str<N>(&p[0]);
}

template <usize N = 64>
inline micron::sstring<N, char>
numa_node_cpulist(u32 node = 0)
{
  __impl::path_t p;
  p += "/sys/devices/system/node/node";
  p += micron::int_to_string_stack<u32, char, 12>(node);
  p += "/cpulist";
  return read_str<N>(&p[0]);
}

struct cpu_freq_info_t {
  u64 cur_khz;
  u64 min_khz;
  u64 max_khz;
  u64 scaling_min_khz;
  u64 scaling_max_khz;
  micron::sstring<32, char> governor;
  micron::sstring<32, char> driver;
};

inline cpu_freq_info_t
cpu_freq_info(u32 cpu_id)
{
  cpu_freq_info_t f;
  f.cur_khz = cpu::scaling_cur_freq(cpu_id);
  f.min_khz = cpu::cpuinfo_min_freq(cpu_id);
  f.max_khz = cpu::cpuinfo_max_freq(cpu_id);
  f.scaling_min_khz = cpu::scaling_min_freq(cpu_id);
  f.scaling_max_khz = cpu::scaling_max_freq(cpu_id);
  f.governor = cpu::scaling_governor(cpu_id);
  f.driver = cpu::scaling_driver(cpu_id);
  return f;
}

struct cpu_topo_t {
  u32 package_id;
  u32 core_id;
  bool online;
};

inline cpu_topo_t
cpu_topo(u32 cpu_id)
{
  cpu_topo_t t;
  t.package_id = cpu::physical_package_id(cpu_id);
  t.core_id = cpu::core_id(cpu_id);
  t.online = cpu::is_online(cpu_id);
  return t;
}

};     // namespace cpu

};     // namespace sysfs

};     // namespace posix
};     // namespace micron
