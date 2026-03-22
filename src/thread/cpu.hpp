//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "../io/console.hpp"
#include "../io/filesystem.hpp"
#include "../linux/process/system.hpp"
#include "../linux/sys/cpu.hpp"
#include "../linux/sys/limits.hpp"
#include "../linux/sys/resource.hpp"
#include "../linux/sys/sysfs.hpp"
#include "../slice.hpp"
#include "../string/strings.hpp"
#include "../string/unistring.hpp"
#include "../types.hpp"
#include "scheduling.hpp"

#if defined(__micron_arch_x86_any)
#include "../../external/x86/cpuid.h"
#include "../../external/x86/x86.h"
#elif defined(__micron_arch_arm_any)
#include "../../external/arm/arm.h"
#include "../../external/arm/sysreg.h"
#endif

namespace micron
{

inline unsigned
cpu_count(void)
{
#if defined(__micron_arch_x86_any)
  return (unsigned)maximum_leaf() + 1;
#elif defined(__micron_arch_arm_any)
  return posix::sysfs::cpu::present_count();
#endif
}

inline unsigned
which_cpu(void)
{
  return posix::getcpu();
}

inline void
set_priority(int prio, int pid = posix::getpid())
{
  posix::set_priority(posix::prio_process, pid, prio);
}

inline unsigned
park_cpu(unsigned c = 0xFFFFFFFF)
{
  if ( c == 0xFFFFFFFF ) {
    c = posix::getcpu();
  }
  posix::cpu_set_t set(c);
  posix::sched_setaffinity(0, sizeof(set), set);
  return c;
}

inline unsigned
park_cpu_pid(pid_t pid, unsigned c = 0xFFFFFFFF)
{
  if ( c == 0xFFFFFFFF ) {
    c = posix::getcpu();
  }
  posix::cpu_set_t set(c);
  posix::sched_setaffinity(pid, sizeof(set), set);
  return c;
}

inline void
park_cpu(pid_t pid, posix::cpu_set_t &set)
{
  posix::sched_setaffinity(pid, sizeof(set), set);
}

inline void
enable_all_cores(pid_t pid = 0)
{
  posix::cpu_set_t set;
  auto count = cpu_count();
  for ( usize i = 0; i <= count; i++ )
    set.cpu_set(i);
  posix::sched_setaffinity(pid, sizeof(set), set);
}

struct cpu_freq_t {
  u64 cur_hz;
  u64 min_hz;
  u64 max_hz;
  micron::sstring<32, char> governor;
};

inline cpu_freq_t
cpu_freq(unsigned cpu_id = 0)
{
  cpu_freq_t f;
  f.cur_hz = posix::sysfs::cpu::cur_freq_hz(cpu_id);
  f.min_hz = posix::sysfs::cpu::min_freq_hz(cpu_id);
  f.max_hz = posix::sysfs::cpu::max_freq_hz(cpu_id);
  f.governor = posix::sysfs::cpu::scaling_governor(cpu_id);
  return f;
}

inline bool
cpu_set_freq_max(unsigned cpu_id, u64 hz)
{
  return posix::sysfs::cpu::set_max_freq_hz(cpu_id, hz);
}

inline bool
cpu_set_freq_min(unsigned cpu_id, u64 hz)
{
  return posix::sysfs::cpu::set_min_freq_hz(cpu_id, hz);
}

inline bool
cpu_set_governor(unsigned cpu_id, const char *gov)
{
  return posix::sysfs::cpu::set_scaling_governor(cpu_id, gov);
}

inline bool
cpu_set_governor_all(const char *gov)
{
  return posix::sysfs::cpu::set_governor_all(gov, cpu_count());
}

inline bool
cpu_set_performance_all()
{
  return posix::sysfs::cpu::set_performance_all(cpu_count());
}

template <bool D = false> class cpu_t : public scheduler_t
{
  pid_t pid;
  posix::cpu_set_t procs;
  unsigned count;

public:
  cpu_t(void) : scheduler_t(posix::getpid()), pid(posix::getpid()), procs(), count(cpu_count())
  {
    if constexpr ( D ) {
      for ( usize i = 0; i <= count; i++ )
        procs.cpu_set(i);
    }     // default to all threads
  }

  bool
  operator[](const usize n) const
  {
    if ( n >= count )
      exc<except::library_error>("micron cpu_t::operator[] out of range");
    return procs.cpu_isset(n);
  }

  void
  clear(void)
  {
    procs.cpu_zero();
  }

  auto &
  get()
  {
    return procs;
  }

  const auto &
  get() const
  {
    return procs;
  }

  bool
  at(const usize n) const
  {
    if ( n >= count )
      exc<except::library_error>("micron cpu_t::at out of range");
    return procs.cpu_isset(n);
  }

  void
  set_core(const usize n)
  {
    if ( n >= count )
      exc<except::library_error>("micron cpu_t::set_core out of range");
    procs.cpu_set(n);
  }

  void
  update_cores(void)
  {
    posix::set_affinity(pid, procs);
  }

  void
  update(void)
  {
    enable_all_cores(pid);
    load_scheduler(pid);
    posix::set_affinity(pid, procs);
  }

  unsigned
  core_count() const
  {
    return count;
  }

  void
  clear_core(const usize n)
  {
    if ( n >= count )
      exc<except::library_error>("micron cpu_t::clear_core out of range");
    procs.cpu_clr(n);
  }

  void
  set_all()
  {
    for ( usize i = 0; i <= count; i++ )
      procs.cpu_set(i);
  }

  usize
  active_count() const
  {
    return procs.cpu_count();
  }

  unsigned
  pin(unsigned c = 0xFFFFFFFF)
  {
    if ( c == 0xFFFFFFFF )
      c = posix::getcpu();
    procs.cpu_zero();
    procs.cpu_set(c);
    posix::sched_setaffinity(pid, sizeof(procs), procs);
    return c;
  }

  cpu_freq_t
  freq(unsigned cpu_id = 0) const
  {
    return cpu_freq(cpu_id);
  }

  bool
  set_freq_max(unsigned cpu_id, u64 hz)
  {
    return cpu_set_freq_max(cpu_id, hz);
  }

  bool
  set_freq_min(unsigned cpu_id, u64 hz)
  {
    return cpu_set_freq_min(cpu_id, hz);
  }

  bool
  set_governor(const char *gov, unsigned cpu_id)
  {
    return micron::cpu_set_governor(cpu_id, gov);
  }

  bool
  set_governor_all(const char *gov)
  {
    return micron::cpu_set_governor_all(gov);
  }

  bool
  performance_mode()
  {
    return micron::cpu_set_performance_all();
  }

  bool
  is_online(unsigned cpu_id) const
  {
    return posix::sysfs::cpu::is_online(cpu_id);
  }

  bool
  set_online(unsigned cpu_id, bool on)
  {
    return posix::sysfs::cpu::set_online(cpu_id, on);
  }

  void
  set_priority(int prio)
  {
    micron::set_priority(prio, pid);
  }

  u32
  cache_line_size(unsigned core_id = 0) const
  {
    return posix::sysfs::cpu::cache::l1d_line_size(core_id);
  }
};

enum class cpu_vendor : u8 {
  unknown = 0,
  intel,
  amd,
  arm_ltd,
  broadcom,
  cavium,
  fujitsu,
  hisilicon,
  nvidia,
  qualcomm,
  samsung,
  apple,
  marvell,
  ampere,
  microsoft,
  phytium,
  centaur,
  cyrix,
  transmeta,
  via,
};

struct cache_info_t {
  u16 type;
  u16 level;
  u32 line_size;
  u32 associativity;
  u32 sets;
  u64 size;
};

struct core_info_t {
  u8 type;
  u8 cluster;
  u64 freq;
  cache_info_t caches[5];
};

// unified cpu features across x86 arm
// NOTE: if we add support for more uarches, consider refactoring this out
struct cpu_features_t {
  char fp;
  char simd;
  char aes;
  char sha1;
  char sha2;
  char sha3;
  char sha512;
  char sm3;
  char sm4;
  char crc32;
  char popcnt;
  char fma;
  char f16c;
  char bf16;
  char i8mm;
  char rdrand;
  char atomics;

  // 8086
  char sse;
  char sse2;
  char sse3;
  char ssse3;
  char sse4_1;
  char sse4_2;
  char avx;
  char avx2;
  char avx512f;
  char avx512cd;
  char avx512bw;
  char avx512dq;
  char avx512vl;
  char avx512vnni;
  char avx512bf16;
  char avx512fp16;
  char avx512ifma;
  char avx512vbmi;
  char avx512vbmi2;
  char avx512bitalg;
  char avx512vpopcntdq;
  char avx_vnni;
  char avx_ifma;
  char avx10;
  char amx_tile;
  char amx_int8;
  char amx_bf16;
  char amx_fp16;
  char gfni;
  char vaes;
  char vpclmulqdq;
  char bmi1;
  char bmi2;
  char adx;
  char movbe;
  char serialize;
  char hle;
  char rtm;
  char sgx;
  char mpx;
  char spec_ctrl;
  char erms;
  char fsgsbase;
  char rdseed;
  char clflushopt;
  char clwb;
  char pclmulqdq;
  char xsave;
  char osxsave;
  char hypervisor;
  char tsc;
  char mmx;
  char cmov;
  char htt;
  char fpu;
  char nx;
  char syscall_x86;
  char amd_3dnow;
  char amd_3dnowext;
  char fma4;
  char xop;
  char tbm;
  char sse4a;
  char apx_f;

  // arm
  char asimd;
  char pmull;
  char sve;
  char sve2;
  char sve2p1;
  char sve_aes;
  char sve_bitperm;
  char sve_bf16;
  char sve_i8mm;
  char sve_f32mm;
  char sve_f64mm;
  char sme;
  char sme2;
  char sme2p1;
  char lse128;
  char dotprod;
  char fphp;
  char asimdhp;
  char asimdrdm;
  char jscvt;
  char fcma;
  char flagm;
  char flagm2;
  char lrcpc;
  char ilrcpc;
  char lrcpc3;
  char dcpop;
  char dcpodp;
  char dit;
  char mte;
  char mte3;
  char bti;
  char paca;
  char pacg;
  char ssbs;
  char sb;
  char mops;
  char hbc;
  char cssc;
  char frint;
  char wfxt;
  char rng;
  char ecv;
  char fpmr;
  char lut;
  char faminmax;
};

inline constexpr unsigned
simd_width()
{
#if defined(__micron_arch_x86_any)
  return __micron_x86_simd_width;
#elif defined(__micron_arch_arm_any)
#if defined(__micron_arm_sve2)
  return 0; /* SVE is VL-agnostic */
#elif defined(__micron_arm_sve)
  return 0;
#elif defined(__micron_arm_neon)
  return 128;
#else
  return 0;
#endif
#else
  return 0;
#endif
}

namespace __impl
{

#if defined(__micron_arch_x86_any)

inline bool
__have_cpuid()
{
  return have_cpuid() != 0;
}

inline int
__max_leaf()
{
  return maximum_leaf();
}

inline void
__fill_features(const processor_t &raw, cpu_features_t &f)
{
  micron::memset(&f, 0, sizeof(cpu_features_t));
  const auto &r = raw.features;

  f.fp = r.fpu;
  f.simd = r.sse2;
  f.aes = r.aes;
  f.sha1 = 0;
  f.sha2 = r.sha;
  f.sha3 = 0;
  f.sha512 = 0;
  f.sm3 = r.sm3;
  f.sm4 = r.sm4;
  f.crc32 = 0;
  f.popcnt = r.popcnt;
  f.fma = r.fma;
  f.f16c = r.f16c;
  f.bf16 = r.avx512_bf16;
  f.i8mm = 0;
  f.rdrand = r.rdrand;
  f.atomics = 1;

  f.sse = r.sse;
  f.sse2 = r.sse2;
  f.sse3 = r.sse3;
  f.ssse3 = r.ssse3;
  f.sse4_1 = r.sse4_1;
  f.sse4_2 = r.sse4_2;
  f.avx = r.avx;
  f.avx2 = r.avx2;
  f.avx512f = r.avx512f;
  f.avx512cd = r.avx512cd;
  f.avx512bw = r.avx512bw;
  f.avx512dq = r.avx512dq;
  f.avx512vl = r.avx512vl;
  f.avx512vnni = r.avx512vnni;
  f.avx512bf16 = r.avx512_bf16;
  f.avx512fp16 = r.avx512_fp16;
  f.avx512ifma = r.avx512ifma;
  f.avx512vbmi = r.avx512vbmi;
  f.avx512vbmi2 = r.avx512vbmi2;
  f.avx512bitalg = r.avx512bitalg;
  f.avx512vpopcntdq = r.avx512vpopcntdq;
  f.avx_vnni = r.avx_vnni;
  f.avx_ifma = r.avx_ifma;
  f.avx10 = r.avx10;
  f.amx_tile = r.amx_tile;
  f.amx_int8 = r.amx_int8;
  f.amx_bf16 = r.amx_bf16;
  f.amx_fp16 = r.amx_fp16;
  f.gfni = r.gfni;
  f.vaes = r.vaes;
  f.vpclmulqdq = r.vpclmulqdq;
  f.bmi1 = r.bmi1;
  f.bmi2 = r.bmi2;
  f.adx = r.adx;
  f.movbe = r.movbe;
  f.serialize = r.serialize;
  f.hle = r.hle;
  f.rtm = r.rtm;
  f.sgx = r.sgx;
  f.mpx = r.mpx;
  f.spec_ctrl = r.spec_ctrl;
  f.erms = r.erms;
  f.fsgsbase = r.fsgsbase;
  f.rdseed = r.rdseed;
  f.clflushopt = r.clflushopt;
  f.clwb = r.clwb;
  f.pclmulqdq = r.pclmulqdq;
  f.xsave = r.xsave;
  f.osxsave = r.osxsave;
  f.hypervisor = r.hypervisor;
  f.tsc = r.tsc;
  f.mmx = r.mmx;
  f.cmov = r.cmov;
  f.htt = r.htt;
  f.fpu = r.fpu;
  f.nx = r.nx;
  f.syscall_x86 = r.syscall;
  f.amd_3dnow = r.amd_3dnow;
  f.amd_3dnowext = r.amd_3dnowext;
  f.fma4 = r.fma4;
  f.xop = r.xop;
  f.tbm = r.tbm;
  f.sse4a = r.sse4a;
  f.apx_f = r.apx_f;
}

inline cpu_vendor
__map_vendor(char v)
{
  switch ( v ) {
  case GenuineIntel :
    return cpu_vendor::intel;
  case AuthenticAMD :
    return cpu_vendor::amd;
  case CentaurHauls :
    return cpu_vendor::centaur;
  case CyrixInstead :
    return cpu_vendor::cyrix;
  case TransmetaCPU :
  case GenuineTMx86 :
    return cpu_vendor::transmeta;
  case VIA_VIA_VIA_ :
    return cpu_vendor::via;
  default :
    return cpu_vendor::unknown;
  }
}

inline void
__fill_caches(const processor_t &raw, core_info_t *cores, int n)
{
  for ( int j = 0; j < n; j++ ) {
    cores[j].type = raw.core[j].type;
    cores[j].cluster = 0;
    cores[j].freq = raw.core[j].freq;
    for ( int i = 0; i < 5; i++ ) {
      const auto &c = raw.core[j].caches[i];
      cores[j].caches[i].type = c.type;
      cores[j].caches[i].level = c.level;
      cores[j].caches[i].line_size = c.coherency;
      cores[j].caches[i].associativity = c.associativity;
      cores[j].caches[i].sets = c.sets;
      cores[j].caches[i].size = c.size;
    }
  }
}

#elif defined(__micron_arch_arm_any)

inline bool
__have_cpuid()
{
  return have_idreg() != 0;
}

inline int
__max_leaf()
{
  return maximum_arch();
}

inline void
__fill_features(const processor_t &raw, cpu_features_t &f)
{
  micron::memset(&f, 0, sizeof(cpu_features_t));
  const auto &r = raw.features;

  f.fp = r.fp;
  f.simd = r.asimd;
  f.aes = r.aes;
  f.sha1 = r.sha1;
  f.sha2 = r.sha2;
  f.sha3 = r.sha3;
  f.sha512 = r.sha512;
  f.sm3 = r.sm3;
  f.sm4 = r.sm4;
  f.crc32 = r.crc32;
  f.popcnt = r.asimd;
  f.fma = r.fp;
  f.f16c = r.f16c;
  f.bf16 = r.bf16;
  f.i8mm = r.i8mm;
  f.rdrand = r.rng;
  f.atomics = r.atomics;

  f.asimd = r.asimd;
  f.pmull = r.pmull;
  f.sve = r.sve;
  f.sve2 = r.sve2;
  f.sve2p1 = r.sve2p1;
  f.sve_aes = r.sve_aes;
  f.sve_bitperm = r.sve_bitperm;
  f.sve_bf16 = r.sve_bf16;
  f.sve_i8mm = r.sve_i8mm;
  f.sve_f32mm = r.sve_f32mm;
  f.sve_f64mm = r.sve_f64mm;
  f.sme = r.sme;
  f.sme2 = r.sme2;
  f.sme2p1 = r.sme2p1;
  f.lse128 = r.lse128;
  f.dotprod = r.asimddp;
  f.fphp = r.fphp;
  f.asimdhp = r.asimdhp;
  f.asimdrdm = r.asimdrdm;
  f.jscvt = r.jscvt;
  f.fcma = r.fcma;
  f.flagm = r.flagm;
  f.flagm2 = r.flagm2;
  f.lrcpc = r.lrcpc;
  f.ilrcpc = r.ilrcpc;
  f.lrcpc3 = r.lrcpc3;
  f.dcpop = r.dcpop;
  f.dcpodp = r.dcpodp;
  f.dit = r.dit;
  f.mte = r.mte;
  f.mte3 = r.mte3;
  f.bti = r.bti;
  f.paca = r.paca;
  f.pacg = r.pacg;
  f.ssbs = r.ssbs;
  f.sb = r.sb;
  f.mops = r.mops;
  f.hbc = r.hbc;
  f.cssc = r.cssc;
  f.frint = r.frint;
  f.wfxt = r.wfxt;
  f.rng = r.rng;
  f.ecv = r.ecv;
  f.fpmr = r.fpmr;
  f.lut = r.lut;
  f.faminmax = r.faminmax;
}

inline cpu_vendor
__map_vendor(char v)
{
  switch ( (unsigned char)v ) {
  case IMPL_ARM :
    return cpu_vendor::arm_ltd;
  case IMPL_BROADCOM :
    return cpu_vendor::broadcom;
  case IMPL_CAVIUM :
    return cpu_vendor::cavium;
  case IMPL_FUJITSU :
    return cpu_vendor::fujitsu;
  case IMPL_HISILICON :
    return cpu_vendor::hisilicon;
  case IMPL_NVIDIA :
    return cpu_vendor::nvidia;
  case IMPL_QUALCOMM :
    return cpu_vendor::qualcomm;
  case IMPL_SAMSUNG :
    return cpu_vendor::samsung;
  case IMPL_APPLE :
    return cpu_vendor::apple;
  case IMPL_MARVELL :
    return cpu_vendor::marvell;
  case IMPL_AMPERE :
    return cpu_vendor::ampere;
  case IMPL_MICROSOFT :
    return cpu_vendor::microsoft;
  case IMPL_PHYTIUM :
    return cpu_vendor::phytium;
  default :
    return cpu_vendor::unknown;
  }
}

inline void
__fill_caches(const processor_t &raw, core_info_t *cores, int n)
{
  for ( int j = 0; j < n; j++ ) {
    cores[j].type = CORE_TYPE_CORE;
    cores[j].cluster = raw.core[j].cluster;
    cores[j].freq = raw.core[j].timer_freq;
    for ( int i = 0; i < 4; i++ ) {
      const auto &c = raw.core[j].caches[i];
      cores[j].caches[i].type = c.type;
      cores[j].caches[i].level = c.level;
      cores[j].caches[i].line_size = c.line_size;
      cores[j].caches[i].associativity = c.associativity;
      cores[j].caches[i].sets = c.sets;
      cores[j].caches[i].size = c.size;
    }
    cores[j].caches[4] = {};
  }
}

#endif

};     // namespace __impl

// NOTE: this class is huge (70KB roughly)
// use with caution
class proc_t
{
  struct __data {
    processor_t raw;
    core_info_t cores[256];
  };

  __data *data_;
  cpu_features_t features_;
  cpu_vendor vendor_;
  unsigned count_;
  int arch_level_;
  bool probed_;
  bool probed_full_;

public:
  ~proc_t()
  {
    if ( data_ ) {
      micron::syscall(SYS_munmap, data_, sizeof(__data));
      data_ = nullptr;
    }
  }

  proc_t() : data_(nullptr), vendor_(cpu_vendor::unknown), count_(cpu_count()), arch_level_(0), probed_(false), probed_full_(false)
  {
    micron::memset(&features_, 0, sizeof(cpu_features_t));
  }

  proc_t(const proc_t &) = delete;
  proc_t &operator=(const proc_t &) = delete;

  proc_t(proc_t &&o) noexcept
      : data_(o.data_), features_(o.features_), vendor_(o.vendor_), count_(o.count_), arch_level_(o.arch_level_), probed_(o.probed_),
        probed_full_(o.probed_full_)
  {
    o.data_ = nullptr;
    o.probed_ = false;
    o.probed_full_ = false;
  }

  proc_t &
  operator=(proc_t &&o) noexcept
  {
    if ( this != &o ) {
      if ( data_ )
        micron::syscall(SYS_munmap, data_, sizeof(__data));
      data_ = o.data_;
      features_ = o.features_;
      vendor_ = o.vendor_;
      count_ = o.count_;
      arch_level_ = o.arch_level_;
      probed_ = o.probed_;
      probed_full_ = o.probed_full_;
      o.data_ = nullptr;
      o.probed_ = false;
      o.probed_full_ = false;
    }
    return *this;
  }

  static proc_t &
  instance()
  {
    static proc_t inst;
    return inst;
  }

  void
  __ensure_data()
  {
    if ( !data_ ) {
      // don't include mman due to spaghetti
      void *p = reinterpret_cast<void *>(micron::syscall(SYS_mmap, nullptr, sizeof(__data), 0x3, 0x22, -1, 0));
      if ( reinterpret_cast<max_t>(p) < 0 )
        return;
      data_ = static_cast<__data *>(p);
      micron::memset(data_, 0, sizeof(__data));
    }
  }

  void
  probe()
  {
    if ( probed_ )
      return;

    if ( !__impl::__have_cpuid() )
      return;

    __ensure_data();
    info(&data_->raw);

    arch_level_ = __impl::__max_leaf();
    vendor_ = __impl::__map_vendor(data_->raw.vendor);
    __impl::__fill_features(data_->raw, features_);

    probed_ = true;
  }

  void
  probe_full()
  {
    if ( !probed_ )
      probe();
    if ( probed_full_ || !data_ )
      return;

    spec(&data_->raw);

    int n = static_cast<int>(count_);
    if ( n > 256 )
      n = 256;
    __impl::__fill_caches(data_->raw, data_->cores, n);

    probed_full_ = true;
  }

  bool
  is_probed() const
  {
    return probed_;
  }

  bool
  is_probed_full() const
  {
    return probed_full_;
  }

  unsigned
  core_count() const
  {
    return count_;
  }

  cpu_vendor
  vendor() const
  {
    return vendor_;
  }

  int
  arch_level() const
  {
    return arch_level_;
  }

  const cpu_features_t &
  features() const
  {
    return features_;
  }

  const core_info_t &
  core(unsigned n) const
  {
    return data_->cores[n];
  }

  bool
  has_avx() const
  {
    return features_.avx;
  }

  bool
  has_avx2() const
  {
    return features_.avx2;
  }

  bool
  has_avx512() const
  {
    return features_.avx512f;
  }

  bool
  has_sse42() const
  {
    return features_.sse4_2;
  }

  bool
  has_neon() const
  {
    return features_.asimd;
  }

  bool
  has_sve() const
  {
    return features_.sve;
  }

  bool
  has_sve2() const
  {
    return features_.sve2;
  }

  bool
  has_sme() const
  {
    return features_.sme;
  }

  bool
  has_aes() const
  {
    return features_.aes;
  }

  bool
  has_sha() const
  {
    return features_.sha2;
  }

  bool
  has_crc32() const
  {
    return features_.crc32;
  }

  bool
  has_bf16() const
  {
    return features_.bf16;
  }

  bool
  has_fma() const
  {
    return features_.fma;
  }

  bool
  has_bmi2() const
  {
    return features_.bmi2;
  }

  bool
  has_popcnt() const
  {
    return features_.popcnt;
  }

  bool
  has_atomics() const
  {
    return features_.atomics;
  }

  bool
  has_dotprod() const
  {
    return features_.dotprod;
  }

  bool
  has(const char cpu_features_t::*field) const
  {
    return features_.*field != 0;
  }

#if defined(__micron_arch_x86_any)
  u8
  family() const
  {
    return data_ ? data_->raw.family : 0;
  }

  u8
  model() const
  {
    return data_ ? data_->raw.model : 0;
  }

  u8
  stepping() const
  {
    return data_ ? data_->raw.stepping : 0;
  }

  u8
  efamily() const
  {
    return data_ ? data_->raw.efamily : 0;
  }

  u8
  emodel() const
  {
    return data_ ? data_->raw.emodel : 0;
  }
#elif defined(__micron_arch_arm_any)
  u8
  implementer() const
  {
    return data_ ? data_->raw.implementer : 0;
  }

  u16
  part() const
  {
    return data_ ? data_->raw.part : 0;
  }

  u8
  variant() const
  {
    return data_ ? data_->raw.variant : 0;
  }

  u8
  revision() const
  {
    return data_ ? data_->raw.revision : 0;
  }
#endif

  const cache_info_t *
  l1d(unsigned core_id = 0) const
  {
    if ( !probed_full_ || !data_ )
      return nullptr;
    for ( int i = 0; i < 5; i++ )
      if ( data_->cores[core_id].caches[i].level == 1 && data_->cores[core_id].caches[i].type == DATA_CACHE )
        return &data_->cores[core_id].caches[i];
    return nullptr;
  }

  const cache_info_t *
  l1i(unsigned core_id = 0) const
  {
    if ( !probed_full_ || !data_ )
      return nullptr;
    for ( int i = 0; i < 5; i++ )
      if ( data_->cores[core_id].caches[i].level == 1 && data_->cores[core_id].caches[i].type == INSTRUCTION_CACHE )
        return &data_->cores[core_id].caches[i];
    return nullptr;
  }

  const cache_info_t *
  l2(unsigned core_id = 0) const
  {
    if ( !probed_full_ || !data_ )
      return nullptr;
    for ( int i = 0; i < 5; i++ )
      if ( data_->cores[core_id].caches[i].level == 2 )
        return &data_->cores[core_id].caches[i];
    return nullptr;
  }

  const cache_info_t *
  l3(unsigned core_id = 0) const
  {
    if ( !probed_full_ || !data_ )
      return nullptr;
    for ( int i = 0; i < 5; i++ )
      if ( data_->cores[core_id].caches[i].level == 3 )
        return &data_->cores[core_id].caches[i];
    return nullptr;
  }

  u32
  cache_line_size(unsigned core_id = 0) const
  {
    auto *d = l1d(core_id);
    return d ? d->line_size : 64;
  }

  unsigned
  sve_vector_bytes() const
  {
#if defined(__micron_arch_arm64) && defined(__micron_arm_sve)
    u64 vl;
    __asm__ volatile("rdvl %0, #1" : "=r"(vl));
    return (unsigned)vl;
#else
    return 0;
#endif
  }

  unsigned
  sve_vector_bits() const
  {
    return sve_vector_bytes() * 8;
  }

  cpu_freq_t
  freq(unsigned cpu_id = 0) const
  {
    return cpu_freq(cpu_id);
  }

  const processor_t *
  raw() const
  {
    return data_ ? &data_->raw : nullptr;
  }

  processor_t *
  raw()
  {
    __ensure_data();
    return &data_->raw;
  }
};

};     // namespace micron
