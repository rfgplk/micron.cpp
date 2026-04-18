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
constexpr static const i32 would_block = 11;     // alias try_again
constexpr static const i32 out_of_memory = 12;
constexpr static const i32 permission_denied = 13;
constexpr static const i32 bad_address = 14;
constexpr static const i32 block_device_req = 15;
constexpr static const i32 device_busy = 16;
constexpr static const i32 busy = 16;     // alias
constexpr static const i32 file_exists = 17;
constexpr static const i32 cross_device_link = 18;
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
constexpr static const i32 dir_not_empty = 39;
constexpr static const i32 too_many_symlinks = 40;
constexpr static const i32 no_message = 42;
constexpr static const i32 identifier_removed = 43;
constexpr static const i32 channel_out_of_range = 44;
constexpr static const i32 level2_not_synced = 45;
constexpr static const i32 level3_halted = 46;
constexpr static const i32 level3_reset = 47;
constexpr static const i32 link_num_out_of_range = 48;
constexpr static const i32 proto_driver_not_attached = 49;
constexpr static const i32 no_csi_structure = 50;
constexpr static const i32 level2_halted = 51;
constexpr static const i32 invalid_exchange = 52;
constexpr static const i32 invalid_request_desc = 53;
constexpr static const i32 exchange_full = 54;
constexpr static const i32 no_anode = 55;
constexpr static const i32 invalid_request_code = 56;
constexpr static const i32 invalid_slot = 57;
constexpr static const i32 bad_font_file = 59;
constexpr static const i32 not_a_stream = 60;
constexpr static const i32 no_data_available = 61;
constexpr static const i32 timer_expired = 62;
constexpr static const i32 no_stream_resources = 63;
constexpr static const i32 not_on_network = 64;
constexpr static const i32 package_not_installed = 65;
constexpr static const i32 object_is_remote = 66;
constexpr static const i32 link_severed = 67;
constexpr static const i32 advertise_error = 68;
constexpr static const i32 srmount_error = 69;
constexpr static const i32 comm_error_on_send = 70;
constexpr static const i32 proto_error = 71;
constexpr static const i32 multihop_attempted = 72;
constexpr static const i32 rfs_specific_error = 73;
constexpr static const i32 not_a_data_message = 74;
constexpr static const i32 overflow = 75;
constexpr static const i32 name_not_unique = 76;
constexpr static const i32 bad_fd = 77;
constexpr static const i32 remote_addr_changed = 78;
constexpr static const i32 shared_lib_inaccessible = 79;
constexpr static const i32 shared_lib_corrupted = 80;
constexpr static const i32 shared_lib_section_bad = 81;
constexpr static const i32 too_many_shared_libs = 82;
constexpr static const i32 cannot_exec_shared_lib = 83;
constexpr static const i32 illegal_byte_sequence = 84;

// for networking (libjkr)
constexpr static const i32 restarted = 85;
constexpr static const i32 streams_pipe_error = 86;
constexpr static const i32 too_many_users = 87;
constexpr static const i32 not_a_socket = 88;
constexpr static const i32 dest_addr_required = 89;
constexpr static const i32 message_too_long = 90;
constexpr static const i32 wrong_protocol = 91;
constexpr static const i32 proto_not_available = 92;
constexpr static const i32 proto_not_supported = 93;
constexpr static const i32 socket_not_supported = 94;
constexpr static const i32 op_not_supported = 95;
constexpr static const i32 proto_family_not_supported = 96;
constexpr static const i32 addr_family_not_supported = 97;
constexpr static const i32 addr_in_use = 98;
constexpr static const i32 addr_not_available = 99;
constexpr static const i32 network_down = 100;
constexpr static const i32 network_unreachable = 101;
constexpr static const i32 network_reset = 102;
constexpr static const i32 connection_aborted = 103;
constexpr static const i32 connection_reset = 104;
constexpr static const i32 no_buffer_space = 105;
constexpr static const i32 is_connected = 106;
constexpr static const i32 not_connected = 107;
constexpr static const i32 endpoint_shutdown = 108;
constexpr static const i32 too_many_refs = 109;
constexpr static const i32 timed_out = 110;
constexpr static const i32 connection_refused = 111;
constexpr static const i32 host_down = 112;
constexpr static const i32 host_unreachable = 113;
constexpr static const i32 already_in_progress = 114;
constexpr static const i32 in_progress = 115;
constexpr static const i32 stale_file_handle = 116;
constexpr static const i32 needs_cleaning = 117;
constexpr static const i32 not_xenix_named = 118;
constexpr static const i32 no_xenix_semaphores = 119;
constexpr static const i32 is_named_type = 120;
constexpr static const i32 remote_io_error = 121;
constexpr static const i32 quota_exceeded = 122;
constexpr static const i32 no_medium = 123;
constexpr static const i32 wrong_medium_type = 124;
constexpr static const i32 operation_canceled = 125;
constexpr static const i32 key_not_available = 126;
constexpr static const i32 key_expired = 127;
constexpr static const i32 key_revoked = 128;
constexpr static const i32 key_rejected = 129;
constexpr static const i32 owner_died = 130;
constexpr static const i32 state_not_recoverable = 131;
constexpr static const i32 rf_kill = 132;
constexpr static const i32 hardware_memory_error = 133;

constexpr const char *permissions_msg = "Operation not permitted *";
constexpr const char *no_entry_msg = "No such file or directory *";
constexpr const char *no_process_msg = "No such process *";
constexpr const char *interrupted_msg = "Interrupted system call *";
constexpr const char *io_error_msg = "I/O error *";
constexpr const char *no_such_device_msg = "No such device or address *";
constexpr const char *arguments_too_big_msg = "Argument list too long *";
constexpr const char *exec_error_msg = "Exec format error *";
constexpr const char *bad_file_number_msg = "Bad file number *";
constexpr const char *no_child_process_msg = "No child processes *";
constexpr const char *try_again_msg = "Try again *";
constexpr const char *out_of_memory_msg = "Out of memory *";
constexpr const char *permission_denied_msg = "Permission denied *";
constexpr const char *bad_address_msg = "Bad address *";
constexpr const char *block_device_req_msg = "Block device required *";
constexpr const char *device_busy_msg = "Device or resource busy *";
constexpr const char *file_exists_msg = "File exists *";
constexpr const char *cross_device_link_msg = "Cross-device link *";
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
constexpr const char *dir_not_empty_msg = "Directory not empty *";
constexpr const char *too_many_symlinks_msg = "Too many symbolic links encountered *";
constexpr const char *no_message_msg = "No message of desired type *";
constexpr const char *identifier_removed_msg = "Identifier removed *";
constexpr const char *channel_out_of_range_msg = "Channel number out of range *";
constexpr const char *level2_not_synced_msg = "Level 2 not synchronized *";
constexpr const char *level3_halted_msg = "Level 3 halted *";
constexpr const char *level3_reset_msg = "Level 3 reset *";
constexpr const char *link_num_out_of_range_msg = "Link number out of range *";
constexpr const char *proto_driver_not_attached_msg = "Protocol driver not attached *";
constexpr const char *no_csi_structure_msg = "No CSI structure available *";
constexpr const char *level2_halted_msg = "Level 2 halted *";
constexpr const char *invalid_exchange_msg = "Invalid exchange *";
constexpr const char *invalid_request_desc_msg = "Invalid request descriptor *";
constexpr const char *exchange_full_msg = "Exchange full *";
constexpr const char *no_anode_msg = "No anode *";
constexpr const char *invalid_request_code_msg = "Invalid request code *";
constexpr const char *invalid_slot_msg = "Invalid slot *";
constexpr const char *bad_font_file_msg = "Bad font file format *";
constexpr const char *not_a_stream_msg = "Device not a stream *";
constexpr const char *no_data_available_msg = "No data available *";
constexpr const char *timer_expired_msg = "Timer expired *";
constexpr const char *no_stream_resources_msg = "Out of streams resources *";
constexpr const char *not_on_network_msg = "Machine is not on the network *";
constexpr const char *package_not_installed_msg = "Package not installed *";
constexpr const char *object_is_remote_msg = "Object is remote *";
constexpr const char *link_severed_msg = "Link has been severed *";
constexpr const char *advertise_error_msg = "Advertise error *";
constexpr const char *srmount_error_msg = "Srmount error *";
constexpr const char *comm_error_on_send_msg = "Communication error on send *";
constexpr const char *proto_error_msg = "Protocol error *";
constexpr const char *multihop_attempted_msg = "Multihop attempted *";
constexpr const char *rfs_specific_error_msg = "RFS specific error *";
constexpr const char *not_a_data_message_msg = "Not a data message *";
constexpr const char *overflow_msg = "Value too large for defined data type *";
constexpr const char *name_not_unique_msg = "Name not unique on network *";
constexpr const char *bad_fd_msg = "File descriptor in bad state *";
constexpr const char *remote_addr_changed_msg = "Remote address changed *";
constexpr const char *shared_lib_inaccessible_msg = "Cannot access a needed shared library *";
constexpr const char *shared_lib_corrupted_msg = "Accessing a corrupted shared library *";
constexpr const char *shared_lib_section_bad_msg = ".lib section in a.out corrupted *";
constexpr const char *too_many_shared_libs_msg = "Attempting to link in too many shared libraries *";
constexpr const char *cannot_exec_shared_lib_msg = "Cannot exec a shared library directly *";
constexpr const char *illegal_byte_sequence_msg = "Illegal byte sequence *";
constexpr const char *restarted_msg = "Interrupted system call should be restarted *";
constexpr const char *streams_pipe_error_msg = "Streams pipe error *";
constexpr const char *too_many_users_msg = "Too many users *";
constexpr const char *not_a_socket_msg = "Socket operation on non-socket *";
constexpr const char *dest_addr_required_msg = "Destination address required *";
constexpr const char *message_too_long_msg = "Message too long *";
constexpr const char *wrong_protocol_msg = "Protocol wrong type for socket *";
constexpr const char *proto_not_available_msg = "Protocol not available *";
constexpr const char *proto_not_supported_msg = "Protocol not supported *";
constexpr const char *socket_not_supported_msg = "Socket type not supported *";
constexpr const char *op_not_supported_msg = "Operation not supported on transport endpoint *";
constexpr const char *proto_family_not_supported_msg = "Protocol family not supported *";
constexpr const char *addr_family_not_supported_msg = "Address family not supported by protocol *";
constexpr const char *addr_in_use_msg = "Address already in use *";
constexpr const char *addr_not_available_msg = "Cannot assign requested address *";
constexpr const char *network_down_msg = "Network is down *";
constexpr const char *network_unreachable_msg = "Network is unreachable *";
constexpr const char *network_reset_msg = "Network dropped connection because of reset *";
constexpr const char *connection_aborted_msg = "Software caused connection abort *";
constexpr const char *connection_reset_msg = "Connection reset by peer *";
constexpr const char *no_buffer_space_msg = "No buffer space available *";
constexpr const char *is_connected_msg = "Transport endpoint is already connected *";
constexpr const char *not_connected_msg = "Transport endpoint is not connected *";
constexpr const char *endpoint_shutdown_msg = "Cannot send after transport endpoint shutdown *";
constexpr const char *too_many_refs_msg = "Too many references: cannot splice *";
constexpr const char *timed_out_msg = "Connection timed out *";
constexpr const char *connection_refused_msg = "Connection refused *";
constexpr const char *host_down_msg = "Host is down *";
constexpr const char *host_unreachable_msg = "No route to host *";
constexpr const char *already_in_progress_msg = "Operation already in progress *";
constexpr const char *in_progress_msg = "Operation now in progress *";
constexpr const char *stale_file_handle_msg = "Stale file handle *";
constexpr const char *needs_cleaning_msg = "Structure needs cleaning *";
constexpr const char *not_xenix_named_msg = "Not a XENIX named type file *";
constexpr const char *no_xenix_semaphores_msg = "No XENIX semaphores available *";
constexpr const char *is_named_type_msg = "Is a named type file *";
constexpr const char *remote_io_error_msg = "Remote I/O error *";
constexpr const char *quota_exceeded_msg = "Quota exceeded *";
constexpr const char *no_medium_msg = "No medium found *";
constexpr const char *wrong_medium_type_msg = "Wrong medium type *";
constexpr const char *operation_canceled_msg = "Operation canceled *";
constexpr const char *key_not_available_msg = "Required key not available *";
constexpr const char *key_expired_msg = "Key has expired *";
constexpr const char *key_revoked_msg = "Key has been revoked *";
constexpr const char *key_rejected_msg = "Key was rejected by service *";
constexpr const char *owner_died_msg = "Owner died *";
constexpr const char *state_not_recoverable_msg = "State not recoverable *";
constexpr const char *rf_kill_msg = "Operation not possible due to RF-kill *";
constexpr const char *hardware_memory_error_msg = "Memory page has hardware error *";
constexpr const char *no_msg = "No errno *";

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
  case try_again :     // also covers would_block
    return try_again_msg;
  case out_of_memory :
    return out_of_memory_msg;
  case permission_denied :
    return permission_denied_msg;
  case bad_address :
    return bad_address_msg;
  case block_device_req :
    return block_device_req_msg;
  case device_busy :     // also covers busy
    return device_busy_msg;
  case file_exists :
    return file_exists_msg;
  case cross_device_link :
    return cross_device_link_msg;
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
  case dir_not_empty :
    return dir_not_empty_msg;
  case too_many_symlinks :
    return too_many_symlinks_msg;
  case no_message :
    return no_message_msg;
  case identifier_removed :
    return identifier_removed_msg;
  case channel_out_of_range :
    return channel_out_of_range_msg;
  case level2_not_synced :
    return level2_not_synced_msg;
  case level3_halted :
    return level3_halted_msg;
  case level3_reset :
    return level3_reset_msg;
  case link_num_out_of_range :
    return link_num_out_of_range_msg;
  case proto_driver_not_attached :
    return proto_driver_not_attached_msg;
  case no_csi_structure :
    return no_csi_structure_msg;
  case level2_halted :
    return level2_halted_msg;
  case invalid_exchange :
    return invalid_exchange_msg;
  case invalid_request_desc :
    return invalid_request_desc_msg;
  case exchange_full :
    return exchange_full_msg;
  case no_anode :
    return no_anode_msg;
  case invalid_request_code :
    return invalid_request_code_msg;
  case invalid_slot :
    return invalid_slot_msg;
  case bad_font_file :
    return bad_font_file_msg;
  case not_a_stream :
    return not_a_stream_msg;
  case no_data_available :
    return no_data_available_msg;
  case timer_expired :
    return timer_expired_msg;
  case no_stream_resources :
    return no_stream_resources_msg;
  case not_on_network :
    return not_on_network_msg;
  case package_not_installed :
    return package_not_installed_msg;
  case object_is_remote :
    return object_is_remote_msg;
  case link_severed :
    return link_severed_msg;
  case advertise_error :
    return advertise_error_msg;
  case srmount_error :
    return srmount_error_msg;
  case comm_error_on_send :
    return comm_error_on_send_msg;
  case proto_error :
    return proto_error_msg;
  case multihop_attempted :
    return multihop_attempted_msg;
  case rfs_specific_error :
    return rfs_specific_error_msg;
  case not_a_data_message :
    return not_a_data_message_msg;
  case overflow :
    return overflow_msg;
  case name_not_unique :
    return name_not_unique_msg;
  case bad_fd :
    return bad_fd_msg;
  case remote_addr_changed :
    return remote_addr_changed_msg;
  case shared_lib_inaccessible :
    return shared_lib_inaccessible_msg;
  case shared_lib_corrupted :
    return shared_lib_corrupted_msg;
  case shared_lib_section_bad :
    return shared_lib_section_bad_msg;
  case too_many_shared_libs :
    return too_many_shared_libs_msg;
  case cannot_exec_shared_lib :
    return cannot_exec_shared_lib_msg;
  case illegal_byte_sequence :
    return illegal_byte_sequence_msg;
  case restarted :
    return restarted_msg;
  case streams_pipe_error :
    return streams_pipe_error_msg;
  case too_many_users :
    return too_many_users_msg;
  case not_a_socket :
    return not_a_socket_msg;
  case dest_addr_required :
    return dest_addr_required_msg;
  case message_too_long :
    return message_too_long_msg;
  case wrong_protocol :
    return wrong_protocol_msg;
  case proto_not_available :
    return proto_not_available_msg;
  case proto_not_supported :
    return proto_not_supported_msg;
  case socket_not_supported :
    return socket_not_supported_msg;
  case op_not_supported :
    return op_not_supported_msg;
  case proto_family_not_supported :
    return proto_family_not_supported_msg;
  case addr_family_not_supported :
    return addr_family_not_supported_msg;
  case addr_in_use :
    return addr_in_use_msg;
  case addr_not_available :
    return addr_not_available_msg;
  case network_down :
    return network_down_msg;
  case network_unreachable :
    return network_unreachable_msg;
  case network_reset :
    return network_reset_msg;
  case connection_aborted :
    return connection_aborted_msg;
  case connection_reset :
    return connection_reset_msg;
  case no_buffer_space :
    return no_buffer_space_msg;
  case is_connected :
    return is_connected_msg;
  case not_connected :
    return not_connected_msg;
  case endpoint_shutdown :
    return endpoint_shutdown_msg;
  case too_many_refs :
    return too_many_refs_msg;
  case timed_out :
    return timed_out_msg;
  case connection_refused :
    return connection_refused_msg;
  case host_down :
    return host_down_msg;
  case host_unreachable :
    return host_unreachable_msg;
  case already_in_progress :
    return already_in_progress_msg;
  case in_progress :
    return in_progress_msg;
  case stale_file_handle :
    return stale_file_handle_msg;
  case needs_cleaning :
    return needs_cleaning_msg;
  case not_xenix_named :
    return not_xenix_named_msg;
  case no_xenix_semaphores :
    return no_xenix_semaphores_msg;
  case is_named_type :
    return is_named_type_msg;
  case remote_io_error :
    return remote_io_error_msg;
  case quota_exceeded :
    return quota_exceeded_msg;
  case no_medium :
    return no_medium_msg;
  case wrong_medium_type :
    return wrong_medium_type_msg;
  case operation_canceled :
    return operation_canceled_msg;
  case key_not_available :
    return key_not_available_msg;
  case key_expired :
    return key_expired_msg;
  case key_revoked :
    return key_revoked_msg;
  case key_rejected :
    return key_rejected_msg;
  case owner_died :
    return owner_died_msg;
  case state_not_recoverable :
    return state_not_recoverable_msg;
  case rf_kill :
    return rf_kill_msg;
  case hardware_memory_error :
    return hardware_memory_error_msg;

  default :
    return no_msg;
  }
}

template <int e>
inline constexpr auto
const_get_errno()
{
  if constexpr ( e == permissions ) return permissions_msg;
  if constexpr ( e == no_entry ) return no_entry_msg;
  if constexpr ( e == no_process ) return no_process_msg;
  if constexpr ( e == interrupted ) return interrupted_msg;
  if constexpr ( e == io_error ) return io_error_msg;
  if constexpr ( e == no_such_device ) return no_such_device_msg;
  if constexpr ( e == arguments_too_big ) return arguments_too_big_msg;
  if constexpr ( e == exec_error ) return exec_error_msg;
  if constexpr ( e == bad_file_number ) return bad_file_number_msg;
  if constexpr ( e == no_child_process ) return no_child_process_msg;
  if constexpr ( e == try_again )     // also covers would_block
    return try_again_msg;
  if constexpr ( e == out_of_memory ) return out_of_memory_msg;
  if constexpr ( e == permission_denied ) return permission_denied_msg;
  if constexpr ( e == bad_address ) return bad_address_msg;
  if constexpr ( e == block_device_req ) return block_device_req_msg;
  if constexpr ( e == device_busy )     // also covers busy
    return device_busy_msg;
  if constexpr ( e == file_exists ) return file_exists_msg;
  if constexpr ( e == cross_device_link ) return cross_device_link_msg;
  if constexpr ( e == no_device ) return no_device_msg;
  if constexpr ( e == not_a_dir ) return not_a_dir_msg;
  if constexpr ( e == is_a_dir ) return is_a_dir_msg;
  if constexpr ( e == invalid_arg ) return invalid_arg_msg;
  if constexpr ( e == file_table_ovflw ) return file_table_ovflw_msg;
  if constexpr ( e == too_many_files ) return too_many_files_msg;
  if constexpr ( e == not_a_tty ) return not_a_tty_msg;
  if constexpr ( e == text_busy ) return text_busy_msg;
  if constexpr ( e == file_too_big ) return file_too_big_msg;
  if constexpr ( e == no_space ) return no_space_msg;
  if constexpr ( e == illegal_seek ) return illegal_seek_msg;
  if constexpr ( e == read_only_fs ) return read_only_fs_msg;
  if constexpr ( e == too_many_links ) return too_many_links_msg;
  if constexpr ( e == broken_pipe ) return broken_pipe_msg;
  if constexpr ( e == out_of_domain ) return out_of_domain_msg;
  if constexpr ( e == not_representable ) return not_representable_msg;
  if constexpr ( e == deadlock ) return deadlock_msg;
  if constexpr ( e == name_too_long ) return name_too_long_msg;
  if constexpr ( e == no_record_locks ) return no_record_locks_msg;
  if constexpr ( e == bad_syscall ) return bad_syscall_msg;
  if constexpr ( e == dir_not_empty ) return dir_not_empty_msg;
  if constexpr ( e == too_many_symlinks ) return too_many_symlinks_msg;
  if constexpr ( e == no_message ) return no_message_msg;
  if constexpr ( e == identifier_removed ) return identifier_removed_msg;
  if constexpr ( e == channel_out_of_range ) return channel_out_of_range_msg;
  if constexpr ( e == level2_not_synced ) return level2_not_synced_msg;
  if constexpr ( e == level3_halted ) return level3_halted_msg;
  if constexpr ( e == level3_reset ) return level3_reset_msg;
  if constexpr ( e == link_num_out_of_range ) return link_num_out_of_range_msg;
  if constexpr ( e == proto_driver_not_attached ) return proto_driver_not_attached_msg;
  if constexpr ( e == no_csi_structure ) return no_csi_structure_msg;
  if constexpr ( e == level2_halted ) return level2_halted_msg;
  if constexpr ( e == invalid_exchange ) return invalid_exchange_msg;
  if constexpr ( e == invalid_request_desc ) return invalid_request_desc_msg;
  if constexpr ( e == exchange_full ) return exchange_full_msg;
  if constexpr ( e == no_anode ) return no_anode_msg;
  if constexpr ( e == invalid_request_code ) return invalid_request_code_msg;
  if constexpr ( e == invalid_slot ) return invalid_slot_msg;
  if constexpr ( e == bad_font_file ) return bad_font_file_msg;
  if constexpr ( e == not_a_stream ) return not_a_stream_msg;
  if constexpr ( e == no_data_available ) return no_data_available_msg;
  if constexpr ( e == timer_expired ) return timer_expired_msg;
  if constexpr ( e == no_stream_resources ) return no_stream_resources_msg;
  if constexpr ( e == not_on_network ) return not_on_network_msg;
  if constexpr ( e == package_not_installed ) return package_not_installed_msg;
  if constexpr ( e == object_is_remote ) return object_is_remote_msg;
  if constexpr ( e == link_severed ) return link_severed_msg;
  if constexpr ( e == advertise_error ) return advertise_error_msg;
  if constexpr ( e == srmount_error ) return srmount_error_msg;
  if constexpr ( e == comm_error_on_send ) return comm_error_on_send_msg;
  if constexpr ( e == proto_error ) return proto_error_msg;
  if constexpr ( e == multihop_attempted ) return multihop_attempted_msg;
  if constexpr ( e == rfs_specific_error ) return rfs_specific_error_msg;
  if constexpr ( e == not_a_data_message ) return not_a_data_message_msg;
  if constexpr ( e == overflow ) return overflow_msg;
  if constexpr ( e == name_not_unique ) return name_not_unique_msg;
  if constexpr ( e == bad_fd ) return bad_fd_msg;
  if constexpr ( e == remote_addr_changed ) return remote_addr_changed_msg;
  if constexpr ( e == shared_lib_inaccessible ) return shared_lib_inaccessible_msg;
  if constexpr ( e == shared_lib_corrupted ) return shared_lib_corrupted_msg;
  if constexpr ( e == shared_lib_section_bad ) return shared_lib_section_bad_msg;
  if constexpr ( e == too_many_shared_libs ) return too_many_shared_libs_msg;
  if constexpr ( e == cannot_exec_shared_lib ) return cannot_exec_shared_lib_msg;
  if constexpr ( e == illegal_byte_sequence ) return illegal_byte_sequence_msg;
  if constexpr ( e == restarted ) return restarted_msg;
  if constexpr ( e == streams_pipe_error ) return streams_pipe_error_msg;
  if constexpr ( e == too_many_users ) return too_many_users_msg;
  if constexpr ( e == not_a_socket ) return not_a_socket_msg;
  if constexpr ( e == dest_addr_required ) return dest_addr_required_msg;
  if constexpr ( e == message_too_long ) return message_too_long_msg;
  if constexpr ( e == wrong_protocol ) return wrong_protocol_msg;
  if constexpr ( e == proto_not_available ) return proto_not_available_msg;
  if constexpr ( e == proto_not_supported ) return proto_not_supported_msg;
  if constexpr ( e == socket_not_supported ) return socket_not_supported_msg;
  if constexpr ( e == op_not_supported ) return op_not_supported_msg;
  if constexpr ( e == proto_family_not_supported ) return proto_family_not_supported_msg;
  if constexpr ( e == addr_family_not_supported ) return addr_family_not_supported_msg;
  if constexpr ( e == addr_in_use ) return addr_in_use_msg;
  if constexpr ( e == addr_not_available ) return addr_not_available_msg;
  if constexpr ( e == network_down ) return network_down_msg;
  if constexpr ( e == network_unreachable ) return network_unreachable_msg;
  if constexpr ( e == network_reset ) return network_reset_msg;
  if constexpr ( e == connection_aborted ) return connection_aborted_msg;
  if constexpr ( e == connection_reset ) return connection_reset_msg;
  if constexpr ( e == no_buffer_space ) return no_buffer_space_msg;
  if constexpr ( e == is_connected ) return is_connected_msg;
  if constexpr ( e == not_connected ) return not_connected_msg;
  if constexpr ( e == endpoint_shutdown ) return endpoint_shutdown_msg;
  if constexpr ( e == too_many_refs ) return too_many_refs_msg;
  if constexpr ( e == timed_out ) return timed_out_msg;
  if constexpr ( e == connection_refused ) return connection_refused_msg;
  if constexpr ( e == host_down ) return host_down_msg;
  if constexpr ( e == host_unreachable ) return host_unreachable_msg;
  if constexpr ( e == already_in_progress ) return already_in_progress_msg;
  if constexpr ( e == in_progress ) return in_progress_msg;
  if constexpr ( e == stale_file_handle ) return stale_file_handle_msg;
  if constexpr ( e == needs_cleaning ) return needs_cleaning_msg;
  if constexpr ( e == not_xenix_named ) return not_xenix_named_msg;
  if constexpr ( e == no_xenix_semaphores ) return no_xenix_semaphores_msg;
  if constexpr ( e == is_named_type ) return is_named_type_msg;
  if constexpr ( e == remote_io_error ) return remote_io_error_msg;
  if constexpr ( e == quota_exceeded ) return quota_exceeded_msg;
  if constexpr ( e == no_medium ) return no_medium_msg;
  if constexpr ( e == wrong_medium_type ) return wrong_medium_type_msg;
  if constexpr ( e == operation_canceled ) return operation_canceled_msg;
  if constexpr ( e == key_not_available ) return key_not_available_msg;
  if constexpr ( e == key_expired ) return key_expired_msg;
  if constexpr ( e == key_revoked ) return key_revoked_msg;
  if constexpr ( e == key_rejected ) return key_rejected_msg;
  if constexpr ( e == owner_died ) return owner_died_msg;
  if constexpr ( e == state_not_recoverable ) return state_not_recoverable_msg;
  if constexpr ( e == rf_kill ) return rf_kill_msg;
  if constexpr ( e == hardware_memory_error ) return hardware_memory_error_msg;

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
