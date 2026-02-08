//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "types.hpp"

thread_local u32 __micron_errno = 0;

u32 *
__micron_errno_location(void)
{
  return &__micron_errno;
}
#ifndef errno
#define errno (*__micron_errno_location())
#endif

namespace micron
{

template <int Val>
inline __attribute__((always_inline)) void
set_errno(void)
{
  errno = Val;
}

inline __attribute__((always_inline)) void
set_errno(int val)
{
  errno = val;
}

namespace error
{

// NOTE: we're doing it like this because it's simpler
constexpr static const u32 permissions = 1;
constexpr static const u32 no_entry = 2;
constexpr static const u32 no_process = 3;
constexpr static const u32 interrupted = 4;
constexpr static const u32 io_error = 5;
constexpr static const u32 no_such_device = 6;
constexpr static const u32 arguments_too_big = 7;
constexpr static const u32 exec_error = 8;
constexpr static const u32 bad_file_number = 9;
constexpr static const u32 no_child_process = 10;
constexpr static const u32 try_again = 11;
constexpr static const u32 out_of_memory = 12;
constexpr static const u32 permission_denied = 13;
constexpr static const u32 bad_address = 14;
constexpr static const u32 block_device_req = 15;
constexpr static const u32 device_busy = 16;
constexpr static const u32 busy = 16;
constexpr static const u32 file_exists = 17;
constexpr static const u32 exdev = 18;
constexpr static const u32 no_device = 19;
constexpr static const u32 not_a_dir = 20;
constexpr static const u32 is_a_dir = 21;
constexpr static const u32 invalid_arg = 22;
constexpr static const u32 file_table_ovflw = 23;
constexpr static const u32 too_many_files = 24;
constexpr static const u32 not_a_tty = 25;
constexpr static const u32 text_busy = 26;
constexpr static const u32 file_too_big = 27;
constexpr static const u32 no_space = 28;
constexpr static const u32 illegal_seek = 29;
constexpr static const u32 read_only_fs = 30;
constexpr static const u32 too_many_links = 31;
constexpr static const u32 broken_pipe = 32;
constexpr static const u32 out_of_domain = 33;
constexpr static const u32 not_representable = 34;
constexpr static const u32 deadlock = 35;
constexpr static const u32 name_too_long = 36;
constexpr static const u32 no_record_locks = 37;
constexpr static const u32 bad_syscall = 38;
constexpr static const u32 overflow = 75;

constexpr const char *permissions_msg = "Operation not permitted *";
constexpr const char *no_entry_msg = "No such file or directory *";
constexpr const char *no_process_msg = "No such process *";
constexpr const char *interrupted_msg = "Interrupted system call *";
constexpr const char *io_error_msg = "I/O error *";
constexpr const char *no_such_device_msg = "No such device or address *";
constexpr const char *arguments_too_big_msg = "Argument list too long *";
constexpr const char *exec_error_msg = "Bad file number *";
constexpr const char *bad_file_number_msg = "Bad file number *";
constexpr const char *no_child_process_msg = "No child processes *";
constexpr const char *try_again_msg = "Try again *";
constexpr const char *out_of_memory_msg = "Out of memory *";
constexpr const char *permission_denied_msg = "Permission denied *";
constexpr const char *bad_address_msg = "Bad address *";
constexpr const char *block_device_req_msg = "Block device required *";
constexpr const char *device_busy_msg = "Device or resource busy *";
constexpr const char *file_exists_msg = "File exists *";
constexpr const char *exdev_msg = "Cross-device link *";
constexpr const char *no_device_msg = "No such device *";
constexpr const char *not_a_dir_msg = "Not a directory *";
constexpr const char *is_a_dir_msg = "Is a directory *";
constexpr const char *invalid_arg_msg = "Invalid argument *";
constexpr const char *file_table_ovflw_msg = "File table overflow *";
constexpr const char *too_many_files_msg = "Too many open files *";
constexpr const char *not_a_tty_msg = "Not a typewriter *";
constexpr const char *text_busy_msg = "Text file busy *";
constexpr const char *file_too_big_msg = "File too large *";
constexpr const char *no_space_msg = "No space left on device *";
constexpr const char *illegal_seek_msg = "Illegal seek *";
constexpr const char *read_only_fs_msg = "Read-only file system *";
constexpr const char *too_many_links_msg = "Too many links *";
constexpr const char *broken_pipe_msg = "Broken pipe *";
constexpr const char *out_of_domain_msg = "Math argument out of domain of func *";
constexpr const char *not_representable_msg = "Math result not representable *";
constexpr const char *deadlock_msg = "Resource deadlock would occur *";
constexpr const char *name_too_long_msg = "File name too long *";
constexpr const char *no_record_locks_msg = "No record locks available *";
constexpr const char *bad_syscall_msg = "Invalid system call number *";
constexpr const char *no_msg = "No errno *";

inline auto
what_errno()
{
  const int e = errno;
  if ( permissions == e ) {
    return permissions_msg;
  }
  if ( no_entry == e ) {
    return no_entry_msg;
  }
  if ( no_process == e ) {
    return no_process_msg;
  }
  if ( interrupted == e ) {
    return interrupted_msg;
  }
  if ( io_error == e ) {

    return io_error_msg;
  }
  if ( no_such_device == e ) {

    return no_such_device_msg;
  }
  if ( arguments_too_big == e ) {

    return arguments_too_big_msg;
  }
  if ( exec_error == e ) {

    return exec_error_msg;
  }
  if ( bad_file_number == e ) {

    return bad_file_number_msg;
  }
  if ( no_child_process == e ) {
    return no_child_process_msg;
  }
  if ( try_again == e ) {

    return try_again_msg;
  }
  if ( out_of_memory == e ) {

    return out_of_memory_msg;
  }
  if ( permission_denied == e ) {
    return permission_denied_msg;
  }
  if ( bad_address == e ) {
    return bad_address_msg;
  }
  if ( block_device_req == e ) {
    return block_device_req_msg;
  }
  if ( device_busy == e ) {
    return device_busy_msg;
  }
  if ( file_exists == e ) {
    return file_exists_msg;
  }
  if ( exdev == e ) {
    return exdev_msg;
  }
  if ( no_device == e ) {
    return no_device_msg;
  }
  if ( not_a_dir == e ) {
    return not_a_dir_msg;
  }
  if ( is_a_dir == e ) {
    return is_a_dir_msg;
  }
  if ( invalid_arg == e ) {
    return invalid_arg_msg;
  }
  if ( file_table_ovflw == e ) {
    return file_table_ovflw_msg;
  }
  if ( too_many_files == e ) {
    return too_many_files_msg;
  }
  if ( not_a_tty == e ) {
    return not_a_tty_msg;
  }
  if ( text_busy == e ) {
    return text_busy_msg;
  }
  if ( file_too_big == e ) {
    return file_too_big_msg;
  }
  if ( no_space == e ) {
    return no_space_msg;
  }
  if ( illegal_seek == e ) {
    return illegal_seek_msg;
  }
  if ( read_only_fs == e ) {
    return read_only_fs_msg;
  }
  if ( too_many_links == e ) {
    return too_many_links_msg;
  }
  if ( broken_pipe == e ) {
    return broken_pipe_msg;
  }
  if ( out_of_domain == e ) {
    return out_of_domain_msg;
  }
  if ( not_representable == e ) {
    return not_representable_msg;
  }
  if ( deadlock == e ) {
    return deadlock_msg;
  }
  if ( name_too_long == e ) {
    return name_too_long_msg;
  }
  if ( no_record_locks == e ) {
    return no_record_locks_msg;
  }
  if ( bad_syscall == e ) {
    return bad_syscall_msg;
  }
  return no_msg;
}

inline auto
get_errno(const int e)
{
  if ( permissions == e ) {
    return permissions_msg;
  }
  if ( no_entry == e ) {
    return no_entry_msg;
  }
  if ( no_process == e ) {
    return no_process_msg;
  }
  if ( interrupted == e ) {
    return interrupted_msg;
  }
  if ( io_error == e ) {

    return io_error_msg;
  }
  if ( no_such_device == e ) {

    return no_such_device_msg;
  }
  if ( arguments_too_big == e ) {

    return arguments_too_big_msg;
  }
  if ( exec_error == e ) {

    return exec_error_msg;
  }
  if ( bad_file_number == e ) {

    return bad_file_number_msg;
  }
  if ( no_child_process == e ) {
    return no_child_process_msg;
  }
  if ( try_again == e ) {

    return try_again_msg;
  }
  if ( out_of_memory == e ) {

    return out_of_memory_msg;
  }
  if ( permission_denied == e ) {
    return permission_denied_msg;
  }
  if ( bad_address == e ) {
    return bad_address_msg;
  }
  if ( block_device_req == e ) {
    return block_device_req_msg;
  }
  if ( device_busy == e ) {
    return device_busy_msg;
  }
  if ( file_exists == e ) {
    return file_exists_msg;
  }
  if ( exdev == e ) {
    return exdev_msg;
  }
  if ( no_device == e ) {
    return no_device_msg;
  }
  if ( not_a_dir == e ) {
    return not_a_dir_msg;
  }
  if ( is_a_dir == e ) {
    return is_a_dir_msg;
  }
  if ( invalid_arg == e ) {
    return invalid_arg_msg;
  }
  if ( file_table_ovflw == e ) {
    return file_table_ovflw_msg;
  }
  if ( too_many_files == e ) {
    return too_many_files_msg;
  }
  if ( not_a_tty == e ) {
    return not_a_tty_msg;
  }
  if ( text_busy == e ) {
    return text_busy_msg;
  }
  if ( file_too_big == e ) {
    return file_too_big_msg;
  }
  if ( no_space == e ) {
    return no_space_msg;
  }
  if ( illegal_seek == e ) {
    return illegal_seek_msg;
  }
  if ( read_only_fs == e ) {
    return read_only_fs_msg;
  }
  if ( too_many_links == e ) {
    return too_many_links_msg;
  }
  if ( broken_pipe == e ) {
    return broken_pipe_msg;
  }
  if ( out_of_domain == e ) {
    return out_of_domain_msg;
  }
  if ( not_representable == e ) {
    return not_representable_msg;
  }
  if ( deadlock == e ) {
    return deadlock_msg;
  }
  if ( name_too_long == e ) {
    return name_too_long_msg;
  }
  if ( no_record_locks == e ) {
    return no_record_locks_msg;
  }
  if ( bad_syscall == e ) {
    return bad_syscall_msg;
  }
  return no_msg;
}

template <int e>
inline constexpr auto
const_get_errno(void)
{
  if constexpr ( permissions == e ) {
    return permissions_msg;
  }
  if constexpr ( no_entry == e ) {
    return no_entry_msg;
  }
  if constexpr ( no_process == e ) {
    return no_process_msg;
  }
  if constexpr ( interrupted == e ) {
    return interrupted_msg;
  }
  if constexpr ( io_error == e ) {

    return io_error_msg;
  }
  if constexpr ( no_such_device == e ) {

    return no_such_device_msg;
  }
  if constexpr ( arguments_too_big == e ) {

    return arguments_too_big_msg;
  }
  if constexpr ( exec_error == e ) {

    return exec_error_msg;
  }
  if constexpr ( bad_file_number == e ) {

    return bad_file_number_msg;
  }
  if constexpr ( no_child_process == e ) {
    return no_child_process_msg;
  }
  if constexpr ( try_again == e ) {

    return try_again_msg;
  }
  if constexpr ( out_of_memory == e ) {

    return out_of_memory_msg;
  }
  if constexpr ( permission_denied == e ) {
    return permission_denied_msg;
  }
  if constexpr ( bad_address == e ) {
    return bad_address_msg;
  }
  if constexpr ( block_device_req == e ) {
    return block_device_req_msg;
  }
  if constexpr ( device_busy == e ) {
    return device_busy_msg;
  }
  if constexpr ( file_exists == e ) {
    return file_exists_msg;
  }
  if constexpr ( exdev == e ) {
    return exdev_msg;
  }
  if constexpr ( no_device == e ) {
    return no_device_msg;
  }
  if constexpr ( not_a_dir == e ) {
    return not_a_dir_msg;
  }
  if constexpr ( is_a_dir == e ) {
    return is_a_dir_msg;
  }
  if constexpr ( invalid_arg == e ) {
    return invalid_arg_msg;
  }
  if constexpr ( file_table_ovflw == e ) {
    return file_table_ovflw_msg;
  }
  if constexpr ( too_many_files == e ) {
    return too_many_files_msg;
  }
  if constexpr ( not_a_tty == e ) {
    return not_a_tty_msg;
  }
  if constexpr ( text_busy == e ) {
    return text_busy_msg;
  }
  if constexpr ( file_too_big == e ) {
    return file_too_big_msg;
  }
  if constexpr ( no_space == e ) {
    return no_space_msg;
  }
  if constexpr ( illegal_seek == e ) {
    return illegal_seek_msg;
  }
  if constexpr ( read_only_fs == e ) {
    return read_only_fs_msg;
  }
  if constexpr ( too_many_links == e ) {
    return too_many_links_msg;
  }
  if constexpr ( broken_pipe == e ) {
    return broken_pipe_msg;
  }
  if constexpr ( out_of_domain == e ) {
    return out_of_domain_msg;
  }
  if constexpr ( not_representable == e ) {
    return not_representable_msg;
  }
  if constexpr ( deadlock == e ) {
    return deadlock_msg;
  }
  if constexpr ( name_too_long == e ) {
    return name_too_long_msg;
  }
  if constexpr ( no_record_locks == e ) {
    return no_record_locks_msg;
  }
  if constexpr ( bad_syscall == e ) {
    return bad_syscall_msg;
  }
  return no_msg;
}

// NOTE: for backwards compatibility, we're keeping the raw list of defines

#ifndef _ASM_GENERIC_ERRNO_H
#define _ASM_GENERIC_ERRNO_H
#define EPERM 1         /* Operation not permitted */
#define ENOENT 2        /* No such file or directory */
#define ESRCH 3         /* No such process */
#define EINTR 4         /* Interrupted system call */
#define EIO 5           /* I/O error */
#define ENXIO 6         /* No such device or address */
#define E2BIG 7         /* Argument list too long */
#define ENOEXEC 8       /* Exec format error */
#define EBADF 9         /* Bad file number */
#define ECHILD 10       /* No child processes */
#define EAGAIN 11       /* Try again */
#define ENOMEM 12       /* Out of memory */
#define EACCES 13       /* Permission denied */
#define EFAULT 14       /* Bad address */
#define ENOTBLK 15      /* Block device required */
#define EBUSY 16        /* Device or resource busy */
#define EEXIST 17       /* File exists */
#define EXDEV 18        /* Cross-device link */
#define ENODEV 19       /* No such device */
#define ENOTDIR 20      /* Not a directory */
#define EISDIR 21       /* Is a directory */
#define EINVAL 22       /* Invalid argument */
#define ENFILE 23       /* File table overflow */
#define EMFILE 24       /* Too many open files */
#define ENOTTY 25       /* Not a typewriter */
#define ETXTBSY 26      /* Text file busy */
#define EFBIG 27        /* File too large */
#define ENOSPC 28       /* No space left on device */
#define ESPIPE 29       /* Illegal seek */
#define EROFS 30        /* Read-only file system */
#define EMLINK 31       /* Too many links */
#define EPIPE 32        /* Broken pipe */
#define EDOM 33         /* Math argument out of domain of func */
#define ERANGE 34       /* Math result not representable */
#define EDEADLK 35      /* Resource deadlock would occur */
#define ENAMETOOLONG 36 /* File name too long */
#define ENOLCK 37       /* No record locks available */

#define ENOSYS 38 /* Invalid system call number */

#define ENOTEMPTY 39       /* Directory not empty */
#define ELOOP 40           /* Too many symbolic links encountered */
#define EWOULDBLOCK EAGAIN /* Operation would block */
#define ENOMSG 42          /* No message of desired type */
#define EIDRM 43           /* Identifier removed */
#define ECHRNG 44          /* Channel number out of range */
#define EL2NSYNC 45        /* Level 2 not synchronized */
#define EL3HLT 46          /* Level 3 halted */
#define EL3RST 47          /* Level 3 reset */
#define ELNRNG 48          /* Link number out of range */
#define EUNATCH 49         /* Protocol driver not attached */
#define ENOCSI 50          /* No CSI structure available */
#define EL2HLT 51          /* Level 2 halted */
#define EBADE 52           /* Invalid exchange */
#define EBADR 53           /* Invalid request descriptor */
#define EXFULL 54          /* Exchange full */
#define ENOANO 55          /* No anode */
#define EBADRQC 56         /* Invalid request code */
#define EBADSLT 57         /* Invalid slot */

#define EDEADLOCK EDEADLK

#define EBFONT 59          /* Bad font file format */
#define ENOSTR 60          /* Device not a stream */
#define ENODATA 61         /* No data available */
#define ETIME 62           /* Timer expired */
#define ENOSR 63           /* Out of streams resources */
#define ENONET 64          /* Machine is not on the network */
#define ENOPKG 65          /* Package not installed */
#define EREMOTE 66         /* Object is remote */
#define ENOLINK 67         /* Link has been severed */
#define EADV 68            /* Advertise error */
#define ESRMNT 69          /* Srmount error */
#define ECOMM 70           /* Communication error on send */
#define EPROTO 71          /* Protocol error */
#define EMULTIHOP 72       /* Multihop attempted */
#define EDOTDOT 73         /* RFS specific error */
#define EBADMSG 74         /* Not a data message */
#define EOVERFLOW 75       /* Value too large for defined data type */
#define ENOTUNIQ 76        /* Name not unique on network */
#define EBADFD 77          /* File descriptor in bad state */
#define EREMCHG 78         /* Remote address changed */
#define ELIBACC 79         /* Can not access a needed shared library */
#define ELIBBAD 80         /* Accessing a corrupted shared library */
#define ELIBSCN 81         /* .lib section in a.out corrupted */
#define ELIBMAX 82         /* Attempting to link in too many shared libraries */
#define ELIBEXEC 83        /* Cannot exec a shared library directly */
#define EILSEQ 84          /* Illegal byte sequence */
#define ERESTART 85        /* Interrupted system call should be restarted */
#define ESTRPIPE 86        /* Streams pipe error */
#define EUSERS 87          /* Too many users */
#define ENOTSOCK 88        /* Socket operation on non-socket */
#define EDESTADDRREQ 89    /* Destination address required */
#define EMSGSIZE 90        /* Message too long */
#define EPROTOTYPE 91      /* Protocol wrong type for socket */
#define ENOPROTOOPT 92     /* Protocol not available */
#define EPROTONOSUPPORT 93 /* Protocol not supported */
#define ESOCKTNOSUPPORT 94 /* Socket type not supported */
#define EOPNOTSUPP 95      /* Operation not supported on transport endpoint */
#define EPFNOSUPPORT 96    /* Protocol family not supported */
#define EAFNOSUPPORT 97    /* Address family not supported by protocol */
#define EADDRINUSE 98      /* Address already in use */
#define EADDRNOTAVAIL 99   /* Cannot assign requested address */
#define ENETDOWN 100       /* Network is down */
#define ENETUNREACH 101    /* Network is unreachable */
#define ENETRESET 102      /* Network dropped connection because of reset */
#define ECONNABORTED 103   /* Software caused connection abort */
#define ECONNRESET 104     /* Connection reset by peer */
#define ENOBUFS 105        /* No buffer space available */
#define EISCONN 106        /* Transport endpoint is already connected */
#define ENOTCONN 107       /* Transport endpoint is not connected */
#define ESHUTDOWN 108      /* Cannot send after transport endpoint shutdown */
#define ETOOMANYREFS 109   /* Too many references: cannot splice */
#define ETIMEDOUT 110      /* Connection timed out */
#define ECONNREFUSED 111   /* Connection refused */
#define EHOSTDOWN 112      /* Host is down */
#define EHOSTUNREACH 113   /* No route to host */
#define EALREADY 114       /* Operation already in progress */
#define EINPROGRESS 115    /* Operation now in progress */
#define ESTALE 116         /* Stale file handle */
#define EUCLEAN 117        /* Structure needs cleaning */
#define ENOTNAM 118        /* Not a XENIX named type file */
#define ENAVAIL 119        /* No XENIX semaphores available */
#define EISNAM 120         /* Is a named type file */
#define EREMOTEIO 121      /* Remote I/O error */
#define EDQUOT 122         /* Quota exceeded */

#define ENOMEDIUM 123    /* No medium found */
#define EMEDIUMTYPE 124  /* Wrong medium type */
#define ECANCELED 125    /* Operation Canceled */
#define ENOKEY 126       /* Required key not available */
#define EKEYEXPIRED 127  /* Key has expired */
#define EKEYREVOKED 128  /* Key has been revoked */
#define EKEYREJECTED 129 /* Key was rejected by service */

/* for robust mutexes */
#define EOWNERDEAD 130      /* Owner died */
#define ENOTRECOVERABLE 131 /* State not recoverable */

#define ERFKILL 132 /* Operation not possible due to RF-kill */

#define EHWPOISON 133 /* Memory page has hardware error */

#endif
};
};
