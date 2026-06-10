// nanoflagsarm (for C99 and onwards)
// https://github.com/rfgplk/nanoflagsarm
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2024 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

// libc-free

#include "features.h"
#include "macros.h"
#include "sysreg.h"

// NOTE: unlike on 8086, with arm you don't have a main cpuid like inst

BEGIN_C_NS

__inline__ int
__nf_strlen(const char *__restrict__ s)
{
  int n = 0;
  while ( s[n] ) n++;
  return n;
}

__inline__ unsigned long
__nf_strtoul(const char *__restrict__ s)
{
  unsigned long v = 0;
  while ( *s == ' ' || *s == '\t' ) s++;
  while ( *s >= '0' && *s <= '9' ) v = v * 10UL + (unsigned long)(*s++ - '0');
  return v;
}

__inline__ int
__nf_scat(char *__restrict__ dst, int pos, const char *__restrict__ s)
{
  while ( *s ) dst[pos++] = *s++;
  dst[pos] = '\0';
  return pos;
}

__inline__ int
__nf_ucat(char *__restrict__ dst, int pos, unsigned long v)
{
  char tmp[20];
  int n = 0;
  do {
    tmp[n++] = (char)('0' + (v % 10UL));
    v /= 10UL;
  } while ( v );
  while ( n ) dst[pos++] = tmp[--n];
  dst[pos] = '\0';
  return pos;
}

// "/sys/devices/system/cpu/cpu%d%s"
__inline__ void
__nf_cpu_path(char *__restrict__ dst, int cpu, const char *__restrict__ leaf)
{
  int p = __nf_scat(dst, 0, "/sys/devices/system/cpu/cpu");
  p = __nf_ucat(dst, p, (unsigned long)cpu);
  __nf_scat(dst, p, leaf);
}

// "/sys/devices/system/cpu/cpu%d/cache/index%d/%s"
__inline__ void
__nf_cache_path(char *__restrict__ dst, int cpu, int index, const char *__restrict__ leaf)
{
  int p = __nf_scat(dst, 0, "/sys/devices/system/cpu/cpu");
  p = __nf_ucat(dst, p, (unsigned long)cpu);
  p = __nf_scat(dst, p, "/cache/index");
  p = __nf_ucat(dst, p, (unsigned long)index);
  p = __nf_scat(dst, p, "/");
  __nf_scat(dst, p, leaf);
}

#ifdef armFEATURES_CPUSET

#ifdef ARCH_AARCH64
#define __NF_NR_getpid 172
#define __NF_NR_sched_setaffinity 122
#define __NF_NR_sched_getaffinity 123
#else
#define __NF_NR_getpid 20
#define __NF_NR_sched_setaffinity 241
#define __NF_NR_sched_getaffinity 242
#endif

#define __NF_CPU_SETSIZE 1024
#define __NF_NCPUBITS (8 * sizeof(unsigned long))

typedef struct {
  unsigned long __bits[__NF_CPU_SETSIZE / __NF_NCPUBITS];
} __nf_cpu_set_t;

// __nf_syscall0 / __nf_syscall3 live in sysreg.h

__inline__ int
topology_set_cpu(const unsigned long n)
{
  long pid = __nf_syscall0(__NF_NR_getpid);
  __nf_cpu_set_t cpu;

  if ( __nf_syscall3(__NF_NR_sched_getaffinity, pid, (long)sizeof(cpu), (long)&cpu) < 0 ) return 0;

  for ( unsigned i = 0; i < sizeof(cpu.__bits) / sizeof(cpu.__bits[0]); i++ ) cpu.__bits[i] = 0;

  cpu.__bits[n / __NF_NCPUBITS] |= (1UL << (n % __NF_NCPUBITS));

  if ( __nf_syscall3(__NF_NR_sched_setaffinity, pid, (long)sizeof(cpu), (long)&cpu) < 0 ) return 0;

  return 1;
}

#else

__inline__ int
topology_set_cpu(const unsigned long n)
{
  (void)n;
  return 0;
}

#endif

__inline__ void
cache_line_sizes(unsigned int *__restrict__ dcache_line, unsigned int *__restrict__ icache_line)
{
  unsigned long ctr;
  read_ctr(&ctr);
  *icache_line = 4U << rfield(ctr, 3, 0);
  *dcache_line = 4U << rfield(ctr, 19, 16);
}

__inline__ unsigned long
__read_sysfs_ulong(const char *__restrict__ path)
{
  char buf[64];
  if ( __nf_read_file(path, buf, (long)sizeof(buf)) > 0 ) return __nf_strtoul(buf);
  return 0;
}

__inline__ void
__read_sysfs_str(const char *__restrict__ path, char *__restrict__ buf, int len)
{
  buf[0] = '\0';
  long n = __nf_read_file(path, buf, (long)len);
  // sysfs leaves are single-line: keep the first line only
  for ( long i = 0; i < n; i++ ) {
    if ( buf[i] == '\n' ) {
      buf[i] = '\0';
      break;
    }
  }
}

__inline__ unsigned long
__parse_size_str(const char *__restrict__ s)
{
  unsigned long val = __nf_strtoul(s);
  int n = __nf_strlen(s);
  if ( n > 0 ) {
    char suffix = s[n - 1];
    if ( suffix == 'K' || suffix == 'k' )
      val *= 1024UL;
    else if ( suffix == 'M' || suffix == 'm' )
      val *= 1024UL * 1024UL;
  }
  return val;
}

__inline__ void
cache_from_sysfs(const int cpu, const int index, cache_t *__restrict__ r)
{
  char path[128];
  char tbuf[32];

  r->type = NO_CACHE;
  r->level = 0;
  r->line_size = 0;
  r->associativity = 0;
  r->sets = 0;
  r->size = 0;

  __nf_cache_path(path, cpu, index, "type");
  __read_sysfs_str(path, tbuf, sizeof(tbuf));

  if ( tbuf[0] == '\0' ) return;

  if ( tbuf[0] == 'D' )
    r->type = DATA_CACHE;
  else if ( tbuf[0] == 'I' )
    r->type = INSTRUCTION_CACHE;
  else if ( tbuf[0] == 'U' )
    r->type = UNIFIED_CACHE;
  else
    return;

  __nf_cache_path(path, cpu, index, "level");
  r->level = (unsigned short)__read_sysfs_ulong(path);

  __nf_cache_path(path, cpu, index, "coherency_line_size");
  r->line_size = (unsigned int)__read_sysfs_ulong(path);

  __nf_cache_path(path, cpu, index, "ways_of_associativity");
  r->associativity = (unsigned int)__read_sysfs_ulong(path);

  __nf_cache_path(path, cpu, index, "number_of_sets");
  r->sets = (unsigned int)__read_sysfs_ulong(path);

  __nf_cache_path(path, cpu, index, "size");
  __read_sysfs_str(path, tbuf, sizeof(tbuf));
  if ( tbuf[0] )
    r->size = __parse_size_str(tbuf);
  else
    r->size = (unsigned long)r->line_size * r->associativity * r->sets;
}

#ifdef ARCH_AARCH64
#define __NF_NR_nanosleep 101
#else
#define __NF_NR_nanosleep 162
#endif

__inline__ void
frequency_user(core_t (*ptr)[256])
{
  unsigned long long start, end;
  start = read_cntvct();
  {
    struct {
      long sec;
      long nsec;
    } ts = { 1, 0 };

    __nf_syscall3(__NF_NR_nanosleep, (long)&ts, 0, 0);
  }
  end = read_cntvct();
  unsigned long long freq = end - start;
  for ( int i = 0; i < 256; i++ ) (*ptr)[i].timer_freq = freq;
}

__inline__ long
__nf_nprocessors(void)
{
  char buf[64];
  if ( __nf_read_file("/sys/devices/system/cpu/present", buf, (long)sizeof(buf)) <= 0 ) return 1;
  const char *last = buf;
  for ( const char *q = buf; *q; q++ )
    if ( *q == '-' || *q == ',' ) last = q + 1;
  return (long)__nf_strtoul(last) + 1;
}

__inline__ void
cores_all(char *__restrict__ total, core_t (*ptr)[256])
{
  long ncpu = __nf_nprocessors();
  if ( ncpu <= 0 ) ncpu = 1;
  if ( ncpu > 256 ) ncpu = 256;
  *total = (char)ncpu;

  for ( int i = 0; i < ncpu; i++ ) {
    (*ptr)[i].type = CORE_TYPE_CORE;
    (*ptr)[i].cluster = 0;

    char path[128];
    char buf[32];
    __nf_cpu_path(path, i, "/topology/physical_package_id");
    if ( __nf_read_file(path, buf, (long)sizeof(buf)) > 0 && buf[0] != '-' ) (*ptr)[i].cluster = (unsigned char)__nf_strtoul(buf);
  }

  for ( int i = (int)ncpu; i < 256; i++ ) (*ptr)[i].type = CORE_TYPE_NONE;
}

END_C_NS
