//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../errno.hpp"
#include "../linux/sys/fcntl.hpp"
#include "../linux/sys/system.hpp"

namespace micron
{
bool
is_root(void)
{
  // NOTE: only a surface level check, root may not have a UID of 0
  return (posix::getuid() == 0 or posix::geteuid() == 0);
}

namespace posix
{

struct passwd_t {
  char pw_name[64];
  char pw_passwd[32];
  uid_t pw_uid;
  gid_t pw_gid;
  char pw_gecos[128];
  char pw_dir[128];
  char pw_shell[64];
};

struct group_t {
  char gr_name[64];
  char gr_passwd[32];
  gid_t gr_gid;
  char gr_members[256];
};

namespace __impl
{
inline void
__field_copy(char *dst, usize cap, const char *s, const char *e)
{
  usize n = (e > s) ? static_cast<usize>(e - s) : 0;
  if ( n >= cap ) n = cap - 1;
  for ( usize i = 0; i < n; ++i ) dst[i] = s[i];
  dst[n] = '\0';
}

inline i64
__parse_long(const char *s, const char *e)
{
  if ( s >= e || *s < '0' || *s > '9' ) return -1;
  u64 v = 0;
  for ( ; s < e && *s >= '0' && *s <= '9'; ++s ) {
    v = v * 10 + static_cast<u64>(*s - '0');
    if ( v > 0xFFFFFFFFULL ) return -1;      // beyond the 32-bit uid/gid range
  }
  return static_cast<i64>(v);
}

template<int MaxF>
inline int
__split_fields(const char *p, const char *e, const char *(&fs)[MaxF], const char *(&fe)[MaxF])
{
  int n = 0;
  const char *s = p;
  for ( const char *c = p; c <= e; ++c ) {
    if ( c == e || *c == ':' ) {
      if ( n < MaxF ) {
        fs[n] = s;
        fe[n] = c;
        ++n;
      }
      s = c + 1;
    }
  }
  return n;
}

inline long
__slurp(const char *path, char *buf, usize cap)
{
  i32 fd = static_cast<i32>(micron::syscall(SYS_openat, at_fdcwd, path, o_rdonly, 0));
  if ( fd < 0 ) return fd;
  long total = 0;
  while ( static_cast<usize>(total) < cap - 1 ) {
    long r = micron::syscall(SYS_read, fd, buf + total, cap - 1 - static_cast<usize>(total));
    if ( r < 0 ) {
      if ( r == -static_cast<long>(error::interrupted) ) continue;      // retry EINTR (was treated as EOF)
      break;
    }
    if ( r == 0 ) break;      // EOF
    total += r;
  }
  micron::syscall(SYS_close, fd);
  buf[total] = '\0';
  return total;
}

inline bool
__key_matches_name(const char *fs, const char *fe, const char *name_key)
{
  usize kl = 0;
  while ( name_key[kl] ) ++kl;
  if ( static_cast<usize>(fe - fs) != kl ) return false;
  for ( usize i = 0; i < kl; ++i )
    if ( fs[i] != name_key[i] ) return false;
  return true;
}
};      // namespace __impl

inline bool
__lookup_passwd(bool by_uid, uid_t uid_key, const char *name_key, passwd_t &out)
{
  char buf[65536];      // /etc/passwd|group can exceed 16 KiB on large systems
  long len = __impl::__slurp("/etc/passwd", buf, sizeof(buf));
  if ( len <= 0 ) return false;
  const char *p = buf;
  const char *end = buf + len;
  while ( p < end ) {
    const char *le = p;
    while ( le < end && *le != '\n' ) ++le;
    const char *fs[7];
    const char *fe[7];
    if ( __impl::__split_fields<7>(p, le, fs, fe) >= 7 ) {
      bool hit = by_uid ? (static_cast<uid_t>(__impl::__parse_long(fs[2], fe[2])) == uid_key)
                        : __impl::__key_matches_name(fs[0], fe[0], name_key);
      if ( hit ) {
        __impl::__field_copy(out.pw_name, sizeof(out.pw_name), fs[0], fe[0]);
        __impl::__field_copy(out.pw_passwd, sizeof(out.pw_passwd), fs[1], fe[1]);
        out.pw_uid = static_cast<uid_t>(__impl::__parse_long(fs[2], fe[2]));
        out.pw_gid = static_cast<gid_t>(__impl::__parse_long(fs[3], fe[3]));
        __impl::__field_copy(out.pw_gecos, sizeof(out.pw_gecos), fs[4], fe[4]);
        __impl::__field_copy(out.pw_dir, sizeof(out.pw_dir), fs[5], fe[5]);
        __impl::__field_copy(out.pw_shell, sizeof(out.pw_shell), fs[6], fe[6]);
        return true;
      }
    }
    p = (le < end) ? le + 1 : end;
  }
  return false;
}

inline bool
getpwuid(uid_t uid, passwd_t &out)
{
  return __lookup_passwd(true, uid, nullptr, out);
}

inline bool
getpwnam(const char *name, passwd_t &out)
{
  return __lookup_passwd(false, 0, name, out);
}

inline bool
__lookup_group(bool by_gid, gid_t gid_key, const char *name_key, group_t &out)
{
  char buf[65536];      // /etc/passwd|group can exceed 16 KiB on large systems
  long len = __impl::__slurp("/etc/group", buf, sizeof(buf));
  if ( len <= 0 ) return false;
  const char *p = buf;
  const char *end = buf + len;
  while ( p < end ) {
    const char *le = p;
    while ( le < end && *le != '\n' ) ++le;
    const char *fs[4];
    const char *fe[4];
    int __nf = __impl::__split_fields<4>(p, le, fs, fe);
    if ( __nf >= 3 ) {
      bool hit = by_gid ? (static_cast<gid_t>(__impl::__parse_long(fs[2], fe[2])) == gid_key)
                        : __impl::__key_matches_name(fs[0], fe[0], name_key);
      if ( hit ) {
        __impl::__field_copy(out.gr_name, sizeof(out.gr_name), fs[0], fe[0]);
        __impl::__field_copy(out.gr_passwd, sizeof(out.gr_passwd), fs[1], fe[1]);
        out.gr_gid = static_cast<gid_t>(__impl::__parse_long(fs[2], fe[2]));
        if ( __nf >= 4 )
          __impl::__field_copy(out.gr_members, sizeof(out.gr_members), fs[3], fe[3]);
        else
          out.gr_members[0] = '\0';
        return true;
      }
    }
    p = (le < end) ? le + 1 : end;
  }
  return false;
}

inline bool
getgrgid(gid_t gid, group_t &out)
{
  return __lookup_group(true, gid, nullptr, out);
}

inline bool
getgrnam(const char *name, group_t &out)
{
  return __lookup_group(false, 0, name, out);
}

};      // namespace posix
};      // namespace micron
