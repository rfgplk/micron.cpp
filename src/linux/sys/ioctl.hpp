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
  requires((micron::is_same_v<Args, unsigned long> && ...))
int
ioctl(int fd, Args... ops)
{
  return micron::syscall(SYS_ioctl, fd, ops...);
}

#define _ioc_nrbits 8
#define _ioc_typebits 8

/*
 * Let any architecture override either of the following before
 * including this file.
 */

#ifndef _ioc_sizebits
#define _ioc_sizebits 14
#endif

#ifndef _ioc_dirbits
#define _ioc_dirbits 2
#endif

#define _ioc_nrmask ((1 << _ioc_nrbits) - 1)
#define _ioc_typemask ((1 << _ioc_typebits) - 1)
#define _ioc_sizemask ((1 << _ioc_sizebits) - 1)
#define _ioc_dirmask ((1 << _ioc_dirbits) - 1)

#define _ioc_nrshift 0
#define _ioc_typeshift (_ioc_nrshift + _ioc_nrbits)
#define _ioc_sizeshift (_ioc_typeshift + _ioc_typebits)
#define _ioc_dirshift (_ioc_sizeshift + _ioc_sizebits)

#ifndef _ioc_none
#define _ioc_none 0u
#endif

#ifndef _ioc_write
#define _ioc_write 1u
#endif

#ifndef _ioc_read
#define _ioc_read 2u
#endif

#define _ioc(dir, type, nr, size)                                                                                       \
  (((dir) << _ioc_dirshift) | ((type) << _ioc_typeshift) | ((nr) << _ioc_nrshift) | ((size) << _ioc_sizeshift))

#define _ioc_typecheck(t) (sizeof(t))

#define _io(type, nr) _ioc(_ioc_none, (type), (nr), 0)
#define _ior(type, nr, argtype) _ioc(_ioc_read, (type), (nr), (_ioc_typecheck(argtype)))
#define _iow(type, nr, argtype) _ioc(_ioc_write, (type), (nr), (_ioc_typecheck(argtype)))
#define _iowr(type, nr, argtype) _ioc(_ioc_read | _ioc_write, (type), (nr), (_ioc_typecheck(argtype)))
#define _ior_bad(type, nr, argtype) _ioc(_ioc_read, (type), (nr), sizeof(argtype))
#define _iow_bad(type, nr, argtype) _ioc(_ioc_write, (type), (nr), sizeof(argtype))
#define _iowr_bad(type, nr, argtype) _ioc(_ioc_read | _ioc_write, (type), (nr), sizeof(argtype))

/* used to decode ioctl numbers.. */
#define _ioc_dir(nr) (((nr) >> _ioc_dirshift) & _ioc_dirmask)
#define _ioc_type(nr) (((nr) >> _ioc_typeshift) & _ioc_typemask)
#define _ioc_nr(nr) (((nr) >> _ioc_nrshift) & _ioc_nrmask)
#define _ioc_size(nr) (((nr) >> _ioc_sizeshift) & _ioc_sizemask)

/* ...and for the drivers/sound files... */

#define ioc_in (_ioc_write << _ioc_dirshift)
#define ioc_out (_ioc_read << _ioc_dirshift)
#define ioc_inout ((_ioc_write | _ioc_read) << _ioc_dirshift)
#define iocsize_mask (_ioc_sizemask << _ioc_sizeshift)
#define iocsize_shift (_ioc_sizeshift)

};
