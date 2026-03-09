//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "types.hpp"

thread_local i32 __micron_errno = 0;

i32 *
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
set_errno(i32 val)
{
  errno = val;
}

template <typename T>
  requires(micron::is_arithmetic_v<T> and micron::is_convertible_v<T, i32>)
inline __attribute__((always_inline)) void
set_errno(const T val)
{
  errno = static_cast<i32>(val);
}

namespace error
{

// NOTE: we're doing it like this because it's simpler
constexpr static const i32 permissions = 1;
constexpr static const i32 no_entry = 2;
constexpr static const i32 no_process = 3;
constexpr static const i32 interrupted = 4;
constexpr static const i32 io_error = 5;
constexpr static const i32 no_such_device = 6;
constexpr static const i32 arguments_too_big = 7;
constexpr static const i32 exec_error = 8;
constexpr static const i32 bad_file_number = 9;
constexpr static const i32 no_child_process = 10;
constexpr static const i32 try_again = 11;
constexpr static const i32 out_of_memory = 12;
constexpr static const i32 permission_denied = 13;
constexpr static const i32 bad_address = 14;
constexpr static const i32 block_device_req = 15;
constexpr static const i32 device_busy = 16;
constexpr static const i32 busy = 16;
constexpr static const i32 file_exists = 17;
constexpr static const i32 exdev = 18;
constexpr static const i32 no_device = 19;
constexpr static const i32 not_a_dir = 20;
constexpr static const i32 is_a_dir = 21;
constexpr static const i32 invalid_arg = 22;
constexpr static const i32 file_table_ovflw = 23;
constexpr static const i32 too_many_files = 24;
constexpr static const i32 not_a_tty = 25;
constexpr static const i32 text_busy = 26;
constexpr static const i32 file_too_big = 27;
constexpr static const i32 no_space = 28;
constexpr static const i32 illegal_seek = 29;
constexpr static const i32 read_only_fs = 30;
constexpr static const i32 too_many_links = 31;
constexpr static const i32 broken_pipe = 32;
constexpr static const i32 out_of_domain = 33;
constexpr static const i32 not_representable = 34;
constexpr static const i32 deadlock = 35;
constexpr static const i32 name_too_long = 36;
constexpr static const i32 no_record_locks = 37;
constexpr static const i32 bad_syscall = 38;
constexpr static const i32 overflow = 75;
constexpr static const i32 bad_fd = 77;

// for networking (libjkr)
constexpr static const i32 restarted = 85;
constexpr static const i32 estrpipe = 86;
constexpr static const i32 eusers = 87;
constexpr static const i32 not_a_socket = 88;
constexpr static const i32 edestaddrreq = 89;
constexpr static const i32 emsgsize = 90;
constexpr static const i32 wrong_protocol = 91;
constexpr static const i32 enoprotoopt = 92;
constexpr static const i32 eprotonosupport = 93;
constexpr static const i32 socket_not_supported = 94;
constexpr static const i32 eopnotsupp = 95;
constexpr static const i32 epfnosupport = 96;
constexpr static const i32 eafnosupport = 97;
constexpr static const i32 addr_in_use = 98;
constexpr static const i32 addr_not_available = 99;
constexpr static const i32 enetdown = 100;
constexpr static const i32 enetunreach = 101;
constexpr static const i32 network_reset = 102;
constexpr static const i32 econnaborted = 103;
constexpr static const i32 econnreset = 104;
constexpr static const i32 enobufs = 105;
constexpr static const i32 eisconn = 106;
constexpr static const i32 enotconn = 107;
constexpr static const i32 eshutdown = 108;
constexpr static const i32 etoomanyrefs = 109;
constexpr static const i32 timed_out = 110;
constexpr static const i32 econnrefused = 111;
constexpr static const i32 ehostdown = 112;
constexpr static const i32 ehostunreach = 113;
constexpr static const i32 ealready = 114;
constexpr static const i32 einprogress = 115;
constexpr static const i32 estale = 116;
constexpr static const i32 euclean = 117;
constexpr static const i32 enotnam = 118;
constexpr static const i32 enavail = 119;
constexpr static const i32 eisnam = 120;
constexpr static const i32 eremoteio = 121;
constexpr static const i32 equot = 122;

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

constexpr const char *restarted_msg = "Interrupted system call should be restarted *";
constexpr const char *estrpipe_msg = "Streams pipe error *";
constexpr const char *eusers_msg = "Too many users *";
constexpr const char *not_a_socket_msg = "Socket operation on non-socket *";
constexpr const char *edestaddrreq_msg = "Destination address required *";
constexpr const char *emsgsize_msg = "Message too long *";
constexpr const char *wrong_protocol_msg = "Protocol wrong type for socket *";
constexpr const char *enoprotoopt_msg = "Protocol not available *";
constexpr const char *eprotonosupport_msg = "Protocol not supported *";
constexpr const char *socket_not_supported_msg = "Socket type not supported *";
constexpr const char *eopnotsupp_msg = "Operation not supported on transport endpoint *";
constexpr const char *epfnosupport_msg = "Protocol family not supported *";
constexpr const char *eafnosupport_msg = "Address family not supported by protocol *";
constexpr const char *eaddrinuse_msg = "Address already in use *";
constexpr const char *addr_not_available_msg = "Cannot assign requested address *";
constexpr const char *enetdown_msg = "Network is down *";
constexpr const char *enetunreach_msg = "Network is unreachable *";
constexpr const char *network_reset_msg = "Network dropped connection because of reset *";
constexpr const char *econnaborted_msg = "Software caused connection abort *";
constexpr const char *econnreset_msg = "Connection reset by peer *";
constexpr const char *enobufs_msg = "No buffer space available *";
constexpr const char *eisconn_msg = "Transport endpoint is already connected *";
constexpr const char *enotconn_msg = "Transport endpoint is not connected *";
constexpr const char *eshutdown_msg = "Cannot send after transport endpoint shutdown *";
constexpr const char *etoomanyrefs_msg = "Too many references: cannot splice *";
constexpr const char *timed_out_msg = "Connection timed out *";
constexpr const char *econnrefused_msg = "Connection refused *";
constexpr const char *ehostdown_msg = "Host is down *";
constexpr const char *ehostunreach_msg = "No route to host *";
constexpr const char *ealready_msg = "Operation already in progress *";
constexpr const char *einprogress_msg = "Operation now in progress *";
constexpr const char *estale_msg = "Stale file handle *";
constexpr const char *euclean_msg = "Structure needs cleaning *";
constexpr const char *enotnam_msg = "Not a XENIX named type file *";
constexpr const char *enavail_msg = "No XENIX semaphores available *";
constexpr const char *eisnam_msg = "Is a named type file *";
constexpr const char *eremoteio_msg = "Remote I/O error *";
constexpr const char *equot_msg = "Quota exceeded *";

inline auto
get_errno(const int e)
{
  switch ( e ) {
  case permissions :
    return permissions_msg;
  case no_entry :
    return no_entry_msg;
  case no_process :
    return no_process_msg;
  case interrupted :
    return interrupted_msg;
  case io_error :
    return io_error_msg;
  case no_such_device :
    return no_such_device_msg;
  case arguments_too_big :
    return arguments_too_big_msg;
  case exec_error :
    return exec_error_msg;
  case bad_file_number :
    return bad_file_number_msg;
  case no_child_process :
    return no_child_process_msg;
  case try_again :
    return try_again_msg;
  case out_of_memory :
    return out_of_memory_msg;
  case permission_denied :
    return permission_denied_msg;
  case bad_address :
    return bad_address_msg;
  case block_device_req :
    return block_device_req_msg;
  case device_busy :
    return device_busy_msg;
  case file_exists :
    return file_exists_msg;
  case exdev :
    return exdev_msg;
  case no_device :
    return no_device_msg;
  case not_a_dir :
    return not_a_dir_msg;
  case is_a_dir :
    return is_a_dir_msg;
  case invalid_arg :
    return invalid_arg_msg;
  case file_table_ovflw :
    return file_table_ovflw_msg;
  case too_many_files :
    return too_many_files_msg;
  case not_a_tty :
    return not_a_tty_msg;
  case text_busy :
    return text_busy_msg;
  case file_too_big :
    return file_too_big_msg;
  case no_space :
    return no_space_msg;
  case illegal_seek :
    return illegal_seek_msg;
  case read_only_fs :
    return read_only_fs_msg;
  case too_many_links :
    return too_many_links_msg;
  case broken_pipe :
    return broken_pipe_msg;
  case out_of_domain :
    return out_of_domain_msg;
  case not_representable :
    return not_representable_msg;
  case deadlock :
    return deadlock_msg;
  case name_too_long :
    return name_too_long_msg;
  case no_record_locks :
    return no_record_locks_msg;
  case bad_syscall :
    return bad_syscall_msg;

  case restarted :
    return restarted_msg;
  case estrpipe :
    return estrpipe_msg;
  case eusers :
    return eusers_msg;
  case not_a_socket :
    return not_a_socket_msg;
  case edestaddrreq :
    return edestaddrreq_msg;
  case emsgsize :
    return emsgsize_msg;
  case wrong_protocol :
    return wrong_protocol_msg;
  case enoprotoopt :
    return enoprotoopt_msg;
  case eprotonosupport :
    return eprotonosupport_msg;
  case socket_not_supported :
    return socket_not_supported_msg;
  case eopnotsupp :
    return eopnotsupp_msg;
  case epfnosupport :
    return epfnosupport_msg;
  case eafnosupport :
    return eafnosupport_msg;
  case addr_in_use :
    return eaddrinuse_msg;
  case addr_not_available :
    return addr_not_available_msg;
  case enetdown :
    return enetdown_msg;
  case enetunreach :
    return enetunreach_msg;
  case network_reset :
    return network_reset_msg;
  case econnaborted :
    return econnaborted_msg;
  case econnreset :
    return econnreset_msg;
  case enobufs :
    return enobufs_msg;
  case eisconn :
    return eisconn_msg;
  case enotconn :
    return enotconn_msg;
  case eshutdown :
    return eshutdown_msg;
  case etoomanyrefs :
    return etoomanyrefs_msg;
  case timed_out :
    return timed_out_msg;
  case econnrefused :
    return econnrefused_msg;
  case ehostdown :
    return ehostdown_msg;
  case ehostunreach :
    return ehostunreach_msg;
  case ealready :
    return ealready_msg;
  case einprogress :
    return einprogress_msg;
  case estale :
    return estale_msg;
  case euclean :
    return euclean_msg;
  case enotnam :
    return enotnam_msg;
  case enavail :
    return enavail_msg;
  case eisnam :
    return eisnam_msg;
  case eremoteio :
    return eremoteio_msg;
  case equot :
    return equot_msg;

  default :
    return no_msg;
  }
}

template <int e>
inline constexpr auto
const_get_errno()
{
  if constexpr ( e == permissions )
    return permissions_msg;
  if constexpr ( e == no_entry )
    return no_entry_msg;
  if constexpr ( e == no_process )
    return no_process_msg;
  if constexpr ( e == interrupted )
    return interrupted_msg;
  if constexpr ( e == io_error )
    return io_error_msg;
  if constexpr ( e == no_such_device )
    return no_such_device_msg;
  if constexpr ( e == arguments_too_big )
    return arguments_too_big_msg;
  if constexpr ( e == exec_error )
    return exec_error_msg;
  if constexpr ( e == bad_file_number )
    return bad_file_number_msg;
  if constexpr ( e == no_child_process )
    return no_child_process_msg;
  if constexpr ( e == try_again )
    return try_again_msg;
  if constexpr ( e == out_of_memory )
    return out_of_memory_msg;
  if constexpr ( e == permission_denied )
    return permission_denied_msg;
  if constexpr ( e == bad_address )
    return bad_address_msg;
  if constexpr ( e == block_device_req )
    return block_device_req_msg;
  if constexpr ( e == device_busy )
    return device_busy_msg;
  if constexpr ( e == file_exists )
    return file_exists_msg;
  if constexpr ( e == exdev )
    return exdev_msg;
  if constexpr ( e == no_device )
    return no_device_msg;
  if constexpr ( e == not_a_dir )
    return not_a_dir_msg;
  if constexpr ( e == is_a_dir )
    return is_a_dir_msg;
  if constexpr ( e == invalid_arg )
    return invalid_arg_msg;
  if constexpr ( e == file_table_ovflw )
    return file_table_ovflw_msg;
  if constexpr ( e == too_many_files )
    return too_many_files_msg;
  if constexpr ( e == not_a_tty )
    return not_a_tty_msg;
  if constexpr ( e == text_busy )
    return text_busy_msg;
  if constexpr ( e == file_too_big )
    return file_too_big_msg;
  if constexpr ( e == no_space )
    return no_space_msg;
  if constexpr ( e == illegal_seek )
    return illegal_seek_msg;
  if constexpr ( e == read_only_fs )
    return read_only_fs_msg;
  if constexpr ( e == too_many_links )
    return too_many_links_msg;
  if constexpr ( e == broken_pipe )
    return broken_pipe_msg;
  if constexpr ( e == out_of_domain )
    return out_of_domain_msg;
  if constexpr ( e == not_representable )
    return not_representable_msg;
  if constexpr ( e == deadlock )
    return deadlock_msg;
  if constexpr ( e == name_too_long )
    return name_too_long_msg;
  if constexpr ( e == no_record_locks )
    return no_record_locks_msg;
  if constexpr ( e == bad_syscall )
    return bad_syscall_msg;

  if constexpr ( e == restarted )
    return restarted_msg;
  if constexpr ( e == estrpipe )
    return estrpipe_msg;
  if constexpr ( e == eusers )
    return eusers_msg;
  if constexpr ( e == not_a_socket )
    return not_a_socket_msg;
  if constexpr ( e == edestaddrreq )
    return edestaddrreq_msg;
  if constexpr ( e == emsgsize )
    return emsgsize_msg;
  if constexpr ( e == wrong_protocol )
    return wrong_protocol_msg;
  if constexpr ( e == enoprotoopt )
    return enoprotoopt_msg;
  if constexpr ( e == eprotonosupport )
    return eprotonosupport_msg;
  if constexpr ( e == socket_not_supported )
    return socket_not_supported_msg;
  if constexpr ( e == eopnotsupp )
    return eopnotsupp_msg;
  if constexpr ( e == epfnosupport )
    return epfnosupport_msg;
  if constexpr ( e == eafnosupport )
    return eafnosupport_msg;
  if constexpr ( e == addr_in_use )
    return eaddrinuse_msg;
  if constexpr ( e == addr_not_available )
    return addr_not_available_msg;
  if constexpr ( e == enetdown )
    return enetdown_msg;
  if constexpr ( e == enetunreach )
    return enetunreach_msg;
  if constexpr ( e == network_reset )
    return network_reset_msg;
  if constexpr ( e == econnaborted )
    return econnaborted_msg;
  if constexpr ( e == econnreset )
    return econnreset_msg;
  if constexpr ( e == enobufs )
    return enobufs_msg;
  if constexpr ( e == eisconn )
    return eisconn_msg;
  if constexpr ( e == enotconn )
    return enotconn_msg;
  if constexpr ( e == eshutdown )
    return eshutdown_msg;
  if constexpr ( e == etoomanyrefs )
    return etoomanyrefs_msg;
  if constexpr ( e == timed_out )
    return timed_out_msg;
  if constexpr ( e == econnrefused )
    return econnrefused_msg;
  if constexpr ( e == ehostdown )
    return ehostdown_msg;
  if constexpr ( e == ehostunreach )
    return ehostunreach_msg;
  if constexpr ( e == ealready )
    return ealready_msg;
  if constexpr ( e == einprogress )
    return einprogress_msg;
  if constexpr ( e == estale )
    return estale_msg;
  if constexpr ( e == euclean )
    return euclean_msg;
  if constexpr ( e == enotnam )
    return enotnam_msg;
  if constexpr ( e == enavail )
    return enavail_msg;
  if constexpr ( e == eisnam )
    return eisnam_msg;
  if constexpr ( e == eremoteio )
    return eremoteio_msg;
  if constexpr ( e == equot )
    return equot_msg;

  return no_msg;
}

inline auto
what_errno()
{
  const int e = errno;
  return get_errno(e);
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
};     // namespace error
};     // namespace micron
