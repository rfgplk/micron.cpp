//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../syscall.hpp"
#include "../../type_traits.hpp"

namespace micron
{

template <typename... Args>
// TODO: reintroduce this, and resolve type conflicts  requires((micron::is_same_v<Args, unsigned long> && ...))
int
ioctl(int fd, Args... ops)
{
  return static_cast<int>(micron::syscall(SYS_ioctl, fd, ops...));
}

constexpr static const u32 __ioc_nrbits = 8;
constexpr static const u32 __ioc_typebits = 8;
constexpr static const u32 __ioc_sizebits = 14;
constexpr static const u32 __ioc_dirbits = 2;

constexpr static const u64 __ioc_nrmask = ((1 << __ioc_nrbits) - 1);
constexpr static const u64 __ioc_typemask = ((1 << __ioc_typebits) - 1);
constexpr static const u64 __ioc_sizemask = ((1 << __ioc_sizebits) - 1);
constexpr static const u64 __ioc_dirmask = ((1 << __ioc_dirbits) - 1);

constexpr static const u32 __ioc_nrshift = 0;
constexpr static const u64 __ioc_typeshift = (__ioc_nrshift + __ioc_nrbits);
constexpr static const u64 __ioc_sizeshift = (__ioc_typeshift + __ioc_typebits);
constexpr static const u64 __ioc_dirshift = (__ioc_sizeshift + __ioc_sizebits);

constexpr static const u64 __ioc_none = 0;
constexpr static const u64 __ioc_write = 1;
constexpr static const u64 __ioc_read = 2;

template <typename T>
consteval size_t
__sizeof_type()
{
  return sizeof(T);
}

consteval u64
io_request(u64 dir, u64 type, u64 nr, u64 size)
{
  return (((dir) << __ioc_dirshift) | ((type) << __ioc_typeshift) | ((nr) << __ioc_nrshift) | ((size) << __ioc_sizeshift));
}

consteval u64
io_default_command(u64 type, u64 nr)
{
  return io_request(__ioc_none, type, nr, 0);
}

template <typename T>
consteval u64
io_read_command(u64 type, u64 nr)
{
  return io_request(__ioc_read, type, nr, __sizeof_type<T>());
}

template <typename T>
consteval u64
io_write_command(u64 type, u64 nr)
{
  return io_request(__ioc_write, type, nr, __sizeof_type<T>());
}

template <typename T>
consteval u64
io_readwrite_command(u64 type, u64 nr)
{
  return io_request(__ioc_write | __ioc_read, type, nr, __sizeof_type<T>());
}

template <typename T>
consteval u64
io_read_bad_command(u64 type, u64 nr)
{
  return io_request(__ioc_read, type, nr, __sizeof_type<T>());
}

template <typename T>
consteval u64
io_write_bad_command(u64 type, u64 nr)
{
  return io_request(__ioc_write, type, nr, __sizeof_type<T>());
}

template <typename T>
consteval u64
io_readwrite_bad_command(u64 type, u64 nr)
{
  return io_request(__ioc_read | __ioc_write, type, nr, __sizeof_type<T>());
}

consteval u64
io_get_dir(u64 nr)
{
  return (((nr) >> __ioc_dirshift) & __ioc_dirmask);
}

consteval u64
io_get_type(u64 nr)
{
  return (((nr) >> __ioc_typeshift) & __ioc_typemask);
}

consteval u64
io_get_nr(u64 nr)
{
  return (((nr) >> __ioc_nrshift) & __ioc_nrmask);
}

consteval u64
io_get_size(u64 nr)
{
  return (((nr) >> __ioc_sizeshift) & __ioc_sizemask);
}

#define ioc_in (__ioc_write << __ioc_dirshift)
#define ioc_out (__ioc_read << __ioc_dirshift)
#define ioc_inout ((__ioc_write | __ioc_read) << __ioc_dirshift)
#define iocsize_mask (__ioc_sizemask << __ioc_sizeshift)
#define iocsize_shift (__ioc_sizeshift)

// BEGIN CONSTANTS

constexpr static const u64 tcgets = 0x5401;
constexpr static const u64 tcsets = 0x5402;
constexpr static const u64 tcsetsw = 0x5403;
constexpr static const u64 tcsetsf = 0x5404;
constexpr static const u64 tcgeta = 0x5405;
constexpr static const u64 tcseta = 0x5406;
constexpr static const u64 tcsetaw = 0x5407;
constexpr static const u64 tcsetaf = 0x5408;
constexpr static const u64 tcsbrk = 0x5409;
constexpr static const u64 tcxonc = 0x540a;
constexpr static const u64 tcflsh = 0x540b;
constexpr static const u64 tiocexcl = 0x540c;
constexpr static const u64 tiocnxcl = 0x540d;
constexpr static const u64 tiocsctty = 0x540e;
constexpr static const u64 tiocgpgrp = 0x540f;
constexpr static const u64 tiocspgrp = 0x5410;
constexpr static const u64 tiocoutq = 0x5411;
constexpr static const u64 tiocsti = 0x5412;
constexpr static const u64 tiocgwinsz = 0x5413;
constexpr static const u64 tiocswinsz = 0x5414;
constexpr static const u64 tiocmget = 0x5415;
constexpr static const u64 tiocmbis = 0x5416;
constexpr static const u64 tiocmbic = 0x5417;
constexpr static const u64 tiocmset = 0x5418;
constexpr static const u64 tiocgsoftcar = 0x5419;
constexpr static const u64 tiocssoftcar = 0x541a;
constexpr static const u64 fionread = 0x541b;
constexpr static const u64 tiocinq = fionread;
constexpr static const u64 tioclinux = 0x541c;
constexpr static const u64 tioccons = 0x541d;
constexpr static const u64 tiocgserial = 0x541e;
constexpr static const u64 tiocsserial = 0x541f;
constexpr static const u64 tiocpkt = 0x5420;
constexpr static const u64 fionbio = 0x5421;
constexpr static const u64 tiocnotty = 0x5422;
constexpr static const u64 tiocsetd = 0x5423;
constexpr static const u64 tiocgetd = 0x5424;
constexpr static const u64 tcsbrkp = 0x5425;  /* needed for posix tcsendbreak() */
constexpr static const u64 tiocsbrk = 0x5427; /* bsd compatibility */
constexpr static const u64 tioccbrk = 0x5428; /* bsd compatibility */
constexpr static const u64 tiocgsid = 0x5429; /* return the session id of fd */
constexpr static const u64 tiocgrs485 = 0x542e;
constexpr static const u64 tiocsrs485 = 0x542f;
constexpr static const u64 tiocgptn = io_read_command<unsigned int>('T', 0x30); /* get pty number (of pty-mux device) */
constexpr static const u64 tiocsptlck = io_write_command<int>('T', 0x31);       /* lock/unlock pty */
constexpr static const u64 tiocgdev = io_read_command<unsigned int>('T', 0x32); /* get primary device node of /dev/console */
constexpr static const u64 tcgetx = 0x5432;                                     /* sys5 tcgetx compatibility */
constexpr static const u64 tcsetx = 0x5433;
constexpr static const u64 tcsetxf = 0x5434;
constexpr static const u64 tcsetxw = 0x5435;
constexpr static const u64 tiocsig = io_write_command<int>('T', 0x36); /* pty: generate signal */
constexpr static const u64 tiocvhangup = 0x5437;
constexpr static const u64 tiocgpkt = io_read_command<int>('T', 0x38);   /* get packet mode state */
constexpr static const u64 tiocgptlck = io_read_command<int>('T', 0x39); /* get pty lock state */
constexpr static const u64 tiocgexcl = io_read_command<int>('T', 0x40);  /* get exclusive mode state */

constexpr static const u64 fionclex = 0x5450;
constexpr static const u64 fioclex = 0x5451;
constexpr static const u64 fioasync = 0x5452;
constexpr static const u64 tiocserconfig = 0x5453;
constexpr static const u64 tiocsergwild = 0x5454;
constexpr static const u64 tiocserswild = 0x5455;
constexpr static const u64 tiocglcktrmios = 0x5456;
constexpr static const u64 tiocslcktrmios = 0x5457;
constexpr static const u64 tiocsergstruct = 0x5458;
constexpr static const u64 tiocsergetlsr = 0x5459;
constexpr static const u64 tiocsergetmulti = 0x545a;
constexpr static const u64 tiocsersetmulti = 0x545b;

constexpr static const u64 tiocmiwait = 0x545c;
constexpr static const u64 tiocgicount = 0x545d;
constexpr static const u64 tiocpkt_data = 0;
constexpr static const u64 tiocpkt_flushread = 1;
constexpr static const u64 tiocpkt_flushwrite = 2;
constexpr static const u64 tiocpkt_stop = 4;
constexpr static const u64 tiocpkt_start = 8;
constexpr static const u64 tiocpkt_nostop = 16;
constexpr static const u64 tiocpkt_dostop = 32;
constexpr static const u64 tiocpkt_ioctl = 64;
constexpr static const u64 tiocser_temt = 0x01;

}
