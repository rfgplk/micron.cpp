/* Generated at libc build time from syscall list.  */
/* The system call list corresponds to kernel 6.7.  */
#ifndef __GLIBC_LINUX_VERSION_CODE
#define __GLIBC_LINUX_VERSION_CODE 395008
#endif

#ifndef __X32_SYSCALL_BIT
#define __X32_SYSCALL_BIT 0x40000000
#endif

// glibc undef guards here so we don't throw errors all over the place
#undef __NR_read
#undef __NR_write
#undef __NR_open
#undef __NR_close
#undef __NR_stat
#undef __NR_fstat
#undef __NR_lstat
#undef __NR_poll
#undef __NR_lseek
#undef __NR_mmap
#undef __NR_mprotect
#undef __NR_munmap
#undef __NR_brk
#undef __NR_rt_sigprocmask
#undef __NR_pread64
#undef __NR_pwrite64
#undef __NR_access
#undef __NR_pipe
#undef __NR_select
#undef __NR_sched_yield
#undef __NR_mremap
#undef __NR_msync
#undef __NR_mincore
#undef __NR_madvise
#undef __NR_shmget
#undef __NR_shmat
#undef __NR_shmctl
#undef __NR_dup
#undef __NR_dup2
#undef __NR_pause
#undef __NR_nanosleep
#undef __NR_getitimer
#undef __NR_alarm
#undef __NR_setitimer
#undef __NR_getpid
#undef __NR_sendfile
#undef __NR_socket
#undef __NR_connect
#undef __NR_accept
#undef __NR_sendto
#undef __NR_shutdown
#undef __NR_bind
#undef __NR_listen
#undef __NR_getsockname
#undef __NR_getpeername
#undef __NR_socketpair
#undef __NR_clone
#undef __NR_fork
#undef __NR_vfork
#undef __NR_exit
#undef __NR_wait4
#undef __NR_kill
#undef __NR_uname
#undef __NR_semget
#undef __NR_semop
#undef __NR_semctl
#undef __NR_shmdt
#undef __NR_msgget
#undef __NR_msgsnd
#undef __NR_msgrcv
#undef __NR_msgctl
#undef __NR_fcntl
#undef __NR_flock
#undef __NR_fsync
#undef __NR_fdatasync
#undef __NR_truncate
#undef __NR_ftruncate
#undef __NR_getdents
#undef __NR_getcwd
#undef __NR_chdir
#undef __NR_fchdir
#undef __NR_rename
#undef __NR_mkdir
#undef __NR_rmdir
#undef __NR_creat
#undef __NR_link
#undef __NR_unlink
#undef __NR_symlink
#undef __NR_readlink
#undef __NR_chmod
#undef __NR_fchmod
#undef __NR_chown
#undef __NR_fchown
#undef __NR_lchown
#undef __NR_umask
#undef __NR_gettimeofday
#undef __NR_getrlimit
#undef __NR_getrusage
#undef __NR_sysinfo
#undef __NR_times
#undef __NR_getuid
#undef __NR_syslog
#undef __NR_getgid
#undef __NR_setuid
#undef __NR_setgid
#undef __NR_geteuid
#undef __NR_getegid
#undef __NR_setpgid
#undef __NR_getppid
#undef __NR_getpgrp
#undef __NR_setsid
#undef __NR_setreuid
#undef __NR_setregid
#undef __NR_getgroups
#undef __NR_setgroups
#undef __NR_setresuid
#undef __NR_getresuid
#undef __NR_setresgid
#undef __NR_getresgid
#undef __NR_getpgid
#undef __NR_setfsuid
#undef __NR_setfsgid
#undef __NR_getsid
#undef __NR_capget
#undef __NR_capset
#undef __NR_rt_sigsuspend
#undef __NR_utime
#undef __NR_mknod
#undef __NR_personality
#undef __NR_ustat
#undef __NR_statfs
#undef __NR_fstatfs
#undef __NR_sysfs
#undef __NR_getpriority
#undef __NR_setpriority
#undef __NR_sched_setparam
#undef __NR_sched_getparam
#undef __NR_sched_setscheduler
#undef __NR_sched_getscheduler
#undef __NR_sched_get_priority_max
#undef __NR_sched_get_priority_min
#undef __NR_sched_rr_get_interval
#undef __NR_mlock
#undef __NR_munlock
#undef __NR_mlockall
#undef __NR_munlockall
#undef __NR_vhangup
#undef __NR_modify_ldt
#undef __NR_pivot_root
#undef __NR_prctl
#undef __NR_arch_prctl
#undef __NR_adjtimex
#undef __NR_setrlimit
#undef __NR_chroot
#undef __NR_sync
#undef __NR_acct
#undef __NR_settimeofday
#undef __NR_mount
#undef __NR_umount2
#undef __NR_swapon
#undef __NR_swapoff
#undef __NR_reboot
#undef __NR_sethostname
#undef __NR_setdomainname
#undef __NR_iopl
#undef __NR_ioperm
#undef __NR_init_module
#undef __NR_delete_module
#undef __NR_quotactl
#undef __NR_getpmsg
#undef __NR_putpmsg
#undef __NR_afs_syscall
#undef __NR_tuxcall
#undef __NR_security
#undef __NR_gettid
#undef __NR_readahead
#undef __NR_setxattr
#undef __NR_lsetxattr
#undef __NR_fsetxattr
#undef __NR_getxattr
#undef __NR_lgetxattr
#undef __NR_fgetxattr
#undef __NR_listxattr
#undef __NR_llistxattr
#undef __NR_flistxattr
#undef __NR_removexattr
#undef __NR_lremovexattr
#undef __NR_fremovexattr
#undef __NR_tkill
#undef __NR_time
#undef __NR_futex
#undef __NR_sched_setaffinity
#undef __NR_sched_getaffinity
#undef __NR_io_destroy
#undef __NR_io_getevents
#undef __NR_io_cancel
#undef __NR_lookup_dcookie
#undef __NR_epoll_create
#undef __NR_remap_file_pages
#undef __NR_getdents64
#undef __NR_set_tid_address
#undef __NR_restart_syscall
#undef __NR_semtimedop
#undef __NR_fadvise64
#undef __NR_timer_settime
#undef __NR_timer_gettime
#undef __NR_timer_getoverrun
#undef __NR_timer_delete
#undef __NR_clock_settime
#undef __NR_clock_gettime
#undef __NR_clock_getres
#undef __NR_clock_nanosleep
#undef __NR_exit_group
#undef __NR_epoll_wait
#undef __NR_epoll_ctl
#undef __NR_tgkill
#undef __NR_utimes
#undef __NR_mbind
#undef __NR_set_mempolicy
#undef __NR_get_mempolicy
#undef __NR_mq_open
#undef __NR_mq_unlink
#undef __NR_mq_timedsend
#undef __NR_mq_timedreceive
#undef __NR_mq_getsetattr
#undef __NR_add_key
#undef __NR_request_key
#undef __NR_keyctl
#undef __NR_ioprio_set
#undef __NR_ioprio_get
#undef __NR_inotify_init
#undef __NR_inotify_add_watch
#undef __NR_inotify_rm_watch
#undef __NR_migrate_pages
#undef __NR_openat
#undef __NR_mkdirat
#undef __NR_mknodat
#undef __NR_fchownat
#undef __NR_futimesat
#undef __NR_newfstatat
#undef __NR_unlinkat
#undef __NR_renameat
#undef __NR_linkat
#undef __NR_symlinkat
#undef __NR_readlinkat
#undef __NR_fchmodat
#undef __NR_faccessat
#undef __NR_pselect6
#undef __NR_ppoll
#undef __NR_unshare
#undef __NR_splice
#undef __NR_tee
#undef __NR_sync_file_range
#undef __NR_utimensat
#undef __NR_epoll_pwait
#undef __NR_signalfd
#undef __NR_timerfd_create
#undef __NR_eventfd
#undef __NR_fallocate
#undef __NR_timerfd_settime
#undef __NR_timerfd_gettime
#undef __NR_accept4
#undef __NR_signalfd4
#undef __NR_eventfd2
#undef __NR_epoll_create1
#undef __NR_dup3
#undef __NR_pipe2
#undef __NR_inotify_init1
#undef __NR_perf_event_open
#undef __NR_fanotify_init
#undef __NR_fanotify_mark
#undef __NR_prlimit64
#undef __NR_name_to_handle_at
#undef __NR_open_by_handle_at
#undef __NR_clock_adjtime
#undef __NR_syncfs
#undef __NR_setns
#undef __NR_getcpu
#undef __NR_kcmp
#undef __NR_finit_module
#undef __NR_sched_setattr
#undef __NR_sched_getattr
#undef __NR_renameat2
#undef __NR_seccomp
#undef __NR_getrandom
#undef __NR_memfd_create
#undef __NR_kexec_file_load
#undef __NR_bpf
#undef __NR_userfaultfd
#undef __NR_membarrier
#undef __NR_mlock2
#undef __NR_copy_file_range
#undef __NR_pkey_mprotect
#undef __NR_pkey_alloc
#undef __NR_pkey_free
#undef __NR_statx
#undef __NR_io_pgetevents
#undef __NR_rseq
#undef __NR_pidfd_send_signal
#undef __NR_io_uring_setup
#undef __NR_io_uring_enter
#undef __NR_io_uring_register
#undef __NR_open_tree
#undef __NR_move_mount
#undef __NR_fsopen
#undef __NR_fsconfig
#undef __NR_fsmount
#undef __NR_fspick
#undef __NR_pidfd_open
#undef __NR_clone3
#undef __NR_close_range
#undef __NR_openat2
#undef __NR_pidfd_getfd
#undef __NR_faccessat2
#undef __NR_process_madvise
#undef __NR_epoll_pwait2
#undef __NR_mount_setattr
#undef __NR_quotactl_fd
#undef __NR_landlock_create_ruleset
#undef __NR_landlock_add_rule
#undef __NR_landlock_restrict_self
#undef __NR_memfd_secret
#undef __NR_process_mrelease
#undef __NR_futex_waitv
#undef __NR_set_mempolicy_home_node
#undef __NR_cachestat
#undef __NR_fchmodat2
#undef __NR_map_shadow_stack
#undef __NR_futex_wake
#undef __NR_futex_wait
#undef __NR_futex_requeue
#undef __NR_statmount
#undef __NR_listmount
#undef __NR_lsm_get_self_attr
#undef __NR_lsm_set_self_attr
#undef __NR_lsm_list_modules
#undef __NR_mseal
#undef __NR_rt_sigaction
#undef __NR_rt_sigreturn
#undef __NR_ioctl
#undef __NR_readv
#undef __NR_writev
#undef __NR_recvfrom
#undef __NR_sendmsg
#undef __NR_recvmsg
#undef __NR_execve
#undef __NR_ptrace
#undef __NR_rt_sigpending
#undef __NR_rt_sigtimedwait
#undef __NR_rt_sigqueueinfo
#undef __NR_sigaltstack
#undef __NR_timer_create
#undef __NR_mq_notify
#undef __NR_kexec_load
#undef __NR_waitid
#undef __NR_set_robust_list
#undef __NR_get_robust_list
#undef __NR_vmsplice
#undef __NR_move_pages
#undef __NR_preadv
#undef __NR_pwritev
#undef __NR_rt_tgsigqueueinfo
#undef __NR_recvmmsg
#undef __NR_sendmmsg
#undef __NR_process_vm_readv
#undef __NR_process_vm_writev
#undef __NR_setsockopt
#undef __NR_getsockopt
#undef __NR_io_setup
#undef __NR_io_submit
#undef __NR_execveat
#undef __NR_preadv2
#undef __NR_pwritev2

#if __x86_64__
#define __NR_read 0
#define __NR_write 1
#define __NR_open 2
#define __NR_close 3
#define __NR_stat 4
#define __NR_fstat 5
#define __NR_lstat 6
#define __NR_poll 7
#define __NR_lseek 8
#define __NR_mmap 9
#define __NR_mprotect 10
#define __NR_munmap 11
#define __NR_brk 12
#define __NR_rt_sigaction 13
#define __NR_rt_sigprocmask 14
#define __NR_rt_sigreturn 15
#define __NR_ioctl 16
#define __NR_pread64 17
#define __NR_pwrite64 18
#define __NR_readv 19
#define __NR_writev 20
#define __NR_access 21
#define __NR_pipe 22
#define __NR_select 23
#define __NR_sched_yield 24
#define __NR_mremap 25
#define __NR_msync 26
#define __NR_mincore 27
#define __NR_madvise 28
#define __NR_shmget 29
#define __NR_shmat 30
#define __NR_shmctl 31
#define __NR_dup 32
#define __NR_dup2 33
#define __NR_pause 34
#define __NR_nanosleep 35
#define __NR_getitimer 36
#define __NR_alarm 37
#define __NR_setitimer 38
#define __NR_getpid 39
#define __NR_sendfile 40
#define __NR_socket 41
#define __NR_connect 42
#define __NR_accept 43
#define __NR_sendto 44
#define __NR_recvfrom 45
#define __NR_sendmsg 46
#define __NR_recvmsg 47
#define __NR_shutdown 48
#define __NR_bind 49
#define __NR_listen 50
#define __NR_getsockname 51
#define __NR_getpeername 52
#define __NR_socketpair 53
#define __NR_setsockopt 54
#define __NR_getsockopt 55
#define __NR_clone 56
#define __NR_fork 57
#define __NR_vfork 58
#define __NR_execve 59
#define __NR_exit 60
#define __NR_wait4 61
#define __NR_kill 62
#define __NR_uname 63
#define __NR_semget 64
#define __NR_semop 65
#define __NR_semctl 66
#define __NR_shmdt 67
#define __NR_msgget 68
#define __NR_msgsnd 69
#define __NR_msgrcv 70
#define __NR_msgctl 71
#define __NR_fcntl 72
#define __NR_flock 73
#define __NR_fsync 74
#define __NR_fdatasync 75
#define __NR_truncate 76
#define __NR_ftruncate 77
#define __NR_getdents 78
#define __NR_getcwd 79
#define __NR_chdir 80
#define __NR_fchdir 81
#define __NR_rename 82
#define __NR_mkdir 83
#define __NR_rmdir 84
#define __NR_creat 85
#define __NR_link 86
#define __NR_unlink 87
#define __NR_symlink 88
#define __NR_readlink 89
#define __NR_chmod 90
#define __NR_fchmod 91
#define __NR_chown 92
#define __NR_fchown 93
#define __NR_lchown 94
#define __NR_umask 95
#define __NR_gettimeofday 96
#define __NR_getrlimit 97
#define __NR_getrusage 98
#define __NR_sysinfo 99
#define __NR_times 100
#define __NR_ptrace 101
#define __NR_getuid 102
#define __NR_syslog 103
#define __NR_getgid 104
#define __NR_setuid 105
#define __NR_setgid 106
#define __NR_geteuid 107
#define __NR_getegid 108
#define __NR_setpgid 109
#define __NR_getppid 110
#define __NR_getpgrp 111
#define __NR_setsid 112
#define __NR_setreuid 113
#define __NR_setregid 114
#define __NR_getgroups 115
#define __NR_setgroups 116
#define __NR_setresuid 117
#define __NR_getresuid 118
#define __NR_setresgid 119
#define __NR_getresgid 120
#define __NR_getpgid 121
#define __NR_setfsuid 122
#define __NR_setfsgid 123
#define __NR_getsid 124
#define __NR_capget 125
#define __NR_capset 126
#define __NR_rt_sigpending 127
#define __NR_rt_sigtimedwait 128
#define __NR_rt_sigqueueinfo 129
#define __NR_rt_sigsuspend 130
#define __NR_sigaltstack 131
#define __NR_utime 132
#define __NR_mknod 133
#define __NR_uselib 134
#define __NR_personality 135
#define __NR_ustat 136
#define __NR_statfs 137
#define __NR_fstatfs 138
#define __NR_sysfs 139
#define __NR_getpriority 140
#define __NR_setpriority 141
#define __NR_sched_setparam 142
#define __NR_sched_getparam 143
#define __NR_sched_setscheduler 144
#define __NR_sched_getscheduler 145
#define __NR_sched_get_priority_max 146
#define __NR_sched_get_priority_min 147
#define __NR_sched_rr_get_interval 148
#define __NR_mlock 149
#define __NR_munlock 150
#define __NR_mlockall 151
#define __NR_munlockall 152
#define __NR_vhangup 153
#define __NR_modify_ldt 154
#define __NR_pivot_root 155
#define __NR__sysctl 156
#define __NR_prctl 157
#define __NR_arch_prctl 158
#define __NR_adjtimex 159
#define __NR_setrlimit 160
#define __NR_chroot 161
#define __NR_sync 162
#define __NR_acct 163
#define __NR_settimeofday 164
#define __NR_mount 165
#define __NR_umount2 166
#define __NR_swapon 167
#define __NR_swapoff 168
#define __NR_reboot 169
#define __NR_sethostname 170
#define __NR_setdomainname 171
#define __NR_iopl 172
#define __NR_ioperm 173
#define __NR_create_module 174
#define __NR_init_module 175
#define __NR_delete_module 176
#define __NR_get_kernel_syms 177
#define __NR_query_module 178
#define __NR_quotactl 179
#define __NR_nfsservctl 180
#define __NR_getpmsg 181
#define __NR_putpmsg 182
#define __NR_afs_syscall 183
#define __NR_tuxcall 184
#define __NR_security 185
#define __NR_gettid 186
#define __NR_readahead 187
#define __NR_setxattr 188
#define __NR_lsetxattr 189
#define __NR_fsetxattr 190
#define __NR_getxattr 191
#define __NR_lgetxattr 192
#define __NR_fgetxattr 193
#define __NR_listxattr 194
#define __NR_llistxattr 195
#define __NR_flistxattr 196
#define __NR_removexattr 197
#define __NR_lremovexattr 198
#define __NR_fremovexattr 199
#define __NR_tkill 200
#define __NR_time 201
#define __NR_futex 202
#define __NR_sched_setaffinity 203
#define __NR_sched_getaffinity 204
#define __NR_set_thread_area 205
#define __NR_io_setup 206
#define __NR_io_destroy 207
#define __NR_io_getevents 208
#define __NR_io_submit 209
#define __NR_io_cancel 210
#define __NR_get_thread_area 211
#define __NR_lookup_dcookie 212
#define __NR_epoll_create 213
#define __NR_epoll_ctl_old 214
#define __NR_epoll_wait_old 215
#define __NR_remap_file_pages 216
#define __NR_getdents64 217
#define __NR_set_tid_address 218
#define __NR_restart_syscall 219
#define __NR_semtimedop 220
#define __NR_fadvise64 221
#define __NR_timer_create 222
#define __NR_timer_settime 223
#define __NR_timer_gettime 224
#define __NR_timer_getoverrun 225
#define __NR_timer_delete 226
#define __NR_clock_settime 227
#define __NR_clock_gettime 228
#define __NR_clock_getres 229
#define __NR_clock_nanosleep 230
#define __NR_exit_group 231
#define __NR_epoll_wait 232
#define __NR_epoll_ctl 233
#define __NR_tgkill 234
#define __NR_utimes 235
#define __NR_vserver 236
#define __NR_mbind 237
#define __NR_set_mempolicy 238
#define __NR_get_mempolicy 239
#define __NR_mq_open 240
#define __NR_mq_unlink 241
#define __NR_mq_timedsend 242
#define __NR_mq_timedreceive 243
#define __NR_mq_notify 244
#define __NR_mq_getsetattr 245
#define __NR_kexec_load 246
#define __NR_waitid 247
#define __NR_add_key 248
#define __NR_request_key 249
#define __NR_keyctl 250
#define __NR_ioprio_set 251
#define __NR_ioprio_get 252
#define __NR_inotify_init 253
#define __NR_inotify_add_watch 254
#define __NR_inotify_rm_watch 255
#define __NR_migrate_pages 256
#define __NR_openat 257
#define __NR_mkdirat 258
#define __NR_mknodat 259
#define __NR_fchownat 260
#define __NR_futimesat 261
#define __NR_newfstatat 262
#define __NR_unlinkat 263
#define __NR_renameat 264
#define __NR_linkat 265
#define __NR_symlinkat 266
#define __NR_readlinkat 267
#define __NR_fchmodat 268
#define __NR_faccessat 269
#define __NR_pselect6 270
#define __NR_ppoll 271
#define __NR_unshare 272
#define __NR_set_robust_list 273
#define __NR_get_robust_list 274
#define __NR_splice 275
#define __NR_tee 276
#define __NR_sync_file_range 277
#define __NR_vmsplice 278
#define __NR_move_pages 279
#define __NR_utimensat 280
#define __NR_epoll_pwait 281
#define __NR_signalfd 282
#define __NR_timerfd_create 283
#define __NR_eventfd 284
#define __NR_fallocate 285
#define __NR_timerfd_settime 286
#define __NR_timerfd_gettime 287
#define __NR_accept4 288
#define __NR_signalfd4 289
#define __NR_eventfd2 290
#define __NR_epoll_create1 291
#define __NR_dup3 292
#define __NR_pipe2 293
#define __NR_inotify_init1 294
#define __NR_preadv 295
#define __NR_pwritev 296
#define __NR_rt_tgsigqueueinfo 297
#define __NR_perf_event_open 298
#define __NR_recvmmsg 299
#define __NR_fanotify_init 300
#define __NR_fanotify_mark 301
#define __NR_prlimit64 302
#define __NR_name_to_handle_at 303
#define __NR_open_by_handle_at 304
#define __NR_clock_adjtime 305
#define __NR_syncfs 306
#define __NR_sendmmsg 307
#define __NR_setns 308
#define __NR_getcpu 309
#define __NR_process_vm_readv 310
#define __NR_process_vm_writev 311
#define __NR_kcmp 312
#define __NR_finit_module 313
#define __NR_sched_setattr 314
#define __NR_sched_getattr 315
#define __NR_renameat2 316
#define __NR_seccomp 317
#define __NR_getrandom 318
#define __NR_memfd_create 319
#define __NR_kexec_file_load 320
#define __NR_bpf 321
#define __NR_execveat 322
#define __NR_userfaultfd 323
#define __NR_membarrier 324
#define __NR_mlock2 325
#define __NR_copy_file_range 326
#define __NR_preadv2 327
#define __NR_pwritev2 328
#define __NR_pkey_mprotect 329
#define __NR_pkey_alloc 330
#define __NR_pkey_free 331
#define __NR_statx 332
#define __NR_io_pgetevents 333
#define __NR_rseq 334
#define __NR_uretprobe 335
#define __NR_pidfd_send_signal 424
#define __NR_io_uring_setup 425
#define __NR_io_uring_enter 426
#define __NR_io_uring_register 427
#define __NR_open_tree 428
#define __NR_move_mount 429
#define __NR_fsopen 430
#define __NR_fsconfig 431
#define __NR_fsmount 432
#define __NR_fspick 433
#define __NR_pidfd_open 434
#define __NR_clone3 435
#define __NR_close_range 436
#define __NR_openat2 437
#define __NR_pidfd_getfd 438
#define __NR_faccessat2 439
#define __NR_process_madvise 440
#define __NR_epoll_pwait2 441
#define __NR_mount_setattr 442
#define __NR_quotactl_fd 443
#define __NR_landlock_create_ruleset 444
#define __NR_landlock_add_rule 445
#define __NR_landlock_restrict_self 446
#define __NR_memfd_secret 447
#define __NR_process_mrelease 448
#define __NR_futex_waitv 449
#define __NR_set_mempolicy_home_node 450
#define __NR_cachestat 451
#define __NR_fchmodat2 452
#define __NR_map_shadow_stack 453
#define __NR_futex_wake 454
#define __NR_futex_wait 455
#define __NR_futex_requeue 456
#define __NR_statmount 457
#define __NR_listmount 458
#define __NR_lsm_get_self_attr 459
#define __NR_lsm_set_self_attr 460
#define __NR_lsm_list_modules 461
#define __NR_mseal 462
#elif defined(__x86__) || defined(__i386__)
#define __NR_read (__X32_SYSCALL_BIT + 0)
#define __NR_write (__X32_SYSCALL_BIT + 1)
#define __NR_open (__X32_SYSCALL_BIT + 2)
#define __NR_close (__X32_SYSCALL_BIT + 3)
#define __NR_stat (__X32_SYSCALL_BIT + 4)
#define __NR_fstat (__X32_SYSCALL_BIT + 5)
#define __NR_lstat (__X32_SYSCALL_BIT + 6)
#define __NR_poll (__X32_SYSCALL_BIT + 7)
#define __NR_lseek (__X32_SYSCALL_BIT + 8)
#define __NR_mmap (__X32_SYSCALL_BIT + 9)
#define __NR_mprotect (__X32_SYSCALL_BIT + 10)
#define __NR_munmap (__X32_SYSCALL_BIT + 11)
#define __NR_brk (__X32_SYSCALL_BIT + 12)
#define __NR_rt_sigprocmask (__X32_SYSCALL_BIT + 14)
#define __NR_pread64 (__X32_SYSCALL_BIT + 17)
#define __NR_pwrite64 (__X32_SYSCALL_BIT + 18)
#define __NR_access (__X32_SYSCALL_BIT + 21)
#define __NR_pipe (__X32_SYSCALL_BIT + 22)
#define __NR_select (__X32_SYSCALL_BIT + 23)
#define __NR_sched_yield (__X32_SYSCALL_BIT + 24)
#define __NR_mremap (__X32_SYSCALL_BIT + 25)
#define __NR_msync (__X32_SYSCALL_BIT + 26)
#define __NR_mincore (__X32_SYSCALL_BIT + 27)
#define __NR_madvise (__X32_SYSCALL_BIT + 28)
#define __NR_shmget (__X32_SYSCALL_BIT + 29)
#define __NR_shmat (__X32_SYSCALL_BIT + 30)
#define __NR_shmctl (__X32_SYSCALL_BIT + 31)
#define __NR_dup (__X32_SYSCALL_BIT + 32)
#define __NR_dup2 (__X32_SYSCALL_BIT + 33)
#define __NR_pause (__X32_SYSCALL_BIT + 34)
#define __NR_nanosleep (__X32_SYSCALL_BIT + 35)
#define __NR_getitimer (__X32_SYSCALL_BIT + 36)
#define __NR_alarm (__X32_SYSCALL_BIT + 37)
#define __NR_setitimer (__X32_SYSCALL_BIT + 38)
#define __NR_getpid (__X32_SYSCALL_BIT + 39)
#define __NR_sendfile (__X32_SYSCALL_BIT + 40)
#define __NR_socket (__X32_SYSCALL_BIT + 41)
#define __NR_connect (__X32_SYSCALL_BIT + 42)
#define __NR_accept (__X32_SYSCALL_BIT + 43)
#define __NR_sendto (__X32_SYSCALL_BIT + 44)
#define __NR_shutdown (__X32_SYSCALL_BIT + 48)
#define __NR_bind (__X32_SYSCALL_BIT + 49)
#define __NR_listen (__X32_SYSCALL_BIT + 50)
#define __NR_getsockname (__X32_SYSCALL_BIT + 51)
#define __NR_getpeername (__X32_SYSCALL_BIT + 52)
#define __NR_socketpair (__X32_SYSCALL_BIT + 53)
#define __NR_clone (__X32_SYSCALL_BIT + 56)
#define __NR_fork (__X32_SYSCALL_BIT + 57)
#define __NR_vfork (__X32_SYSCALL_BIT + 58)
#define __NR_exit (__X32_SYSCALL_BIT + 60)
#define __NR_wait4 (__X32_SYSCALL_BIT + 61)
#define __NR_kill (__X32_SYSCALL_BIT + 62)
#define __NR_uname (__X32_SYSCALL_BIT + 63)
#define __NR_semget (__X32_SYSCALL_BIT + 64)
#define __NR_semop (__X32_SYSCALL_BIT + 65)
#define __NR_semctl (__X32_SYSCALL_BIT + 66)
#define __NR_shmdt (__X32_SYSCALL_BIT + 67)
#define __NR_msgget (__X32_SYSCALL_BIT + 68)
#define __NR_msgsnd (__X32_SYSCALL_BIT + 69)
#define __NR_msgrcv (__X32_SYSCALL_BIT + 70)
#define __NR_msgctl (__X32_SYSCALL_BIT + 71)
#define __NR_fcntl (__X32_SYSCALL_BIT + 72)
#define __NR_flock (__X32_SYSCALL_BIT + 73)
#define __NR_fsync (__X32_SYSCALL_BIT + 74)
#define __NR_fdatasync (__X32_SYSCALL_BIT + 75)
#define __NR_truncate (__X32_SYSCALL_BIT + 76)
#define __NR_ftruncate (__X32_SYSCALL_BIT + 77)
#define __NR_getdents (__X32_SYSCALL_BIT + 78)
#define __NR_getcwd (__X32_SYSCALL_BIT + 79)
#define __NR_chdir (__X32_SYSCALL_BIT + 80)
#define __NR_fchdir (__X32_SYSCALL_BIT + 81)
#define __NR_rename (__X32_SYSCALL_BIT + 82)
#define __NR_mkdir (__X32_SYSCALL_BIT + 83)
#define __NR_rmdir (__X32_SYSCALL_BIT + 84)
#define __NR_creat (__X32_SYSCALL_BIT + 85)
#define __NR_link (__X32_SYSCALL_BIT + 86)
#define __NR_unlink (__X32_SYSCALL_BIT + 87)
#define __NR_symlink (__X32_SYSCALL_BIT + 88)
#define __NR_readlink (__X32_SYSCALL_BIT + 89)
#define __NR_chmod (__X32_SYSCALL_BIT + 90)
#define __NR_fchmod (__X32_SYSCALL_BIT + 91)
#define __NR_chown (__X32_SYSCALL_BIT + 92)
#define __NR_fchown (__X32_SYSCALL_BIT + 93)
#define __NR_lchown (__X32_SYSCALL_BIT + 94)
#define __NR_umask (__X32_SYSCALL_BIT + 95)
#define __NR_gettimeofday (__X32_SYSCALL_BIT + 96)
#define __NR_getrlimit (__X32_SYSCALL_BIT + 97)
#define __NR_getrusage (__X32_SYSCALL_BIT + 98)
#define __NR_sysinfo (__X32_SYSCALL_BIT + 99)
#define __NR_times (__X32_SYSCALL_BIT + 100)
#define __NR_getuid (__X32_SYSCALL_BIT + 102)
#define __NR_syslog (__X32_SYSCALL_BIT + 103)
#define __NR_getgid (__X32_SYSCALL_BIT + 104)
#define __NR_setuid (__X32_SYSCALL_BIT + 105)
#define __NR_setgid (__X32_SYSCALL_BIT + 106)
#define __NR_geteuid (__X32_SYSCALL_BIT + 107)
#define __NR_getegid (__X32_SYSCALL_BIT + 108)
#define __NR_setpgid (__X32_SYSCALL_BIT + 109)
#define __NR_getppid (__X32_SYSCALL_BIT + 110)
#define __NR_getpgrp (__X32_SYSCALL_BIT + 111)
#define __NR_setsid (__X32_SYSCALL_BIT + 112)
#define __NR_setreuid (__X32_SYSCALL_BIT + 113)
#define __NR_setregid (__X32_SYSCALL_BIT + 114)
#define __NR_getgroups (__X32_SYSCALL_BIT + 115)
#define __NR_setgroups (__X32_SYSCALL_BIT + 116)
#define __NR_setresuid (__X32_SYSCALL_BIT + 117)
#define __NR_getresuid (__X32_SYSCALL_BIT + 118)
#define __NR_setresgid (__X32_SYSCALL_BIT + 119)
#define __NR_getresgid (__X32_SYSCALL_BIT + 120)
#define __NR_getpgid (__X32_SYSCALL_BIT + 121)
#define __NR_setfsuid (__X32_SYSCALL_BIT + 122)
#define __NR_setfsgid (__X32_SYSCALL_BIT + 123)
#define __NR_getsid (__X32_SYSCALL_BIT + 124)
#define __NR_capget (__X32_SYSCALL_BIT + 125)
#define __NR_capset (__X32_SYSCALL_BIT + 126)
#define __NR_rt_sigsuspend (__X32_SYSCALL_BIT + 130)
#define __NR_utime (__X32_SYSCALL_BIT + 132)
#define __NR_mknod (__X32_SYSCALL_BIT + 133)
#define __NR_personality (__X32_SYSCALL_BIT + 135)
#define __NR_ustat (__X32_SYSCALL_BIT + 136)
#define __NR_statfs (__X32_SYSCALL_BIT + 137)
#define __NR_fstatfs (__X32_SYSCALL_BIT + 138)
#define __NR_sysfs (__X32_SYSCALL_BIT + 139)
#define __NR_getpriority (__X32_SYSCALL_BIT + 140)
#define __NR_setpriority (__X32_SYSCALL_BIT + 141)
#define __NR_sched_setparam (__X32_SYSCALL_BIT + 142)
#define __NR_sched_getparam (__X32_SYSCALL_BIT + 143)
#define __NR_sched_setscheduler (__X32_SYSCALL_BIT + 144)
#define __NR_sched_getscheduler (__X32_SYSCALL_BIT + 145)
#define __NR_sched_get_priority_max (__X32_SYSCALL_BIT + 146)
#define __NR_sched_get_priority_min (__X32_SYSCALL_BIT + 147)
#define __NR_sched_rr_get_interval (__X32_SYSCALL_BIT + 148)
#define __NR_mlock (__X32_SYSCALL_BIT + 149)
#define __NR_munlock (__X32_SYSCALL_BIT + 150)
#define __NR_mlockall (__X32_SYSCALL_BIT + 151)
#define __NR_munlockall (__X32_SYSCALL_BIT + 152)
#define __NR_vhangup (__X32_SYSCALL_BIT + 153)
#define __NR_modify_ldt (__X32_SYSCALL_BIT + 154)
#define __NR_pivot_root (__X32_SYSCALL_BIT + 155)
#define __NR_prctl (__X32_SYSCALL_BIT + 157)
#define __NR_arch_prctl (__X32_SYSCALL_BIT + 158)
#define __NR_adjtimex (__X32_SYSCALL_BIT + 159)
#define __NR_setrlimit (__X32_SYSCALL_BIT + 160)
#define __NR_chroot (__X32_SYSCALL_BIT + 161)
#define __NR_sync (__X32_SYSCALL_BIT + 162)
#define __NR_acct (__X32_SYSCALL_BIT + 163)
#define __NR_settimeofday (__X32_SYSCALL_BIT + 164)
#define __NR_mount (__X32_SYSCALL_BIT + 165)
#define __NR_umount2 (__X32_SYSCALL_BIT + 166)
#define __NR_swapon (__X32_SYSCALL_BIT + 167)
#define __NR_swapoff (__X32_SYSCALL_BIT + 168)
#define __NR_reboot (__X32_SYSCALL_BIT + 169)
#define __NR_sethostname (__X32_SYSCALL_BIT + 170)
#define __NR_setdomainname (__X32_SYSCALL_BIT + 171)
#define __NR_iopl (__X32_SYSCALL_BIT + 172)
#define __NR_ioperm (__X32_SYSCALL_BIT + 173)
#define __NR_init_module (__X32_SYSCALL_BIT + 175)
#define __NR_delete_module (__X32_SYSCALL_BIT + 176)
#define __NR_quotactl (__X32_SYSCALL_BIT + 179)
#define __NR_getpmsg (__X32_SYSCALL_BIT + 181)
#define __NR_putpmsg (__X32_SYSCALL_BIT + 182)
#define __NR_afs_syscall (__X32_SYSCALL_BIT + 183)
#define __NR_tuxcall (__X32_SYSCALL_BIT + 184)
#define __NR_security (__X32_SYSCALL_BIT + 185)
#define __NR_gettid (__X32_SYSCALL_BIT + 186)
#define __NR_readahead (__X32_SYSCALL_BIT + 187)
#define __NR_setxattr (__X32_SYSCALL_BIT + 188)
#define __NR_lsetxattr (__X32_SYSCALL_BIT + 189)
#define __NR_fsetxattr (__X32_SYSCALL_BIT + 190)
#define __NR_getxattr (__X32_SYSCALL_BIT + 191)
#define __NR_lgetxattr (__X32_SYSCALL_BIT + 192)
#define __NR_fgetxattr (__X32_SYSCALL_BIT + 193)
#define __NR_listxattr (__X32_SYSCALL_BIT + 194)
#define __NR_llistxattr (__X32_SYSCALL_BIT + 195)
#define __NR_flistxattr (__X32_SYSCALL_BIT + 196)
#define __NR_removexattr (__X32_SYSCALL_BIT + 197)
#define __NR_lremovexattr (__X32_SYSCALL_BIT + 198)
#define __NR_fremovexattr (__X32_SYSCALL_BIT + 199)
#define __NR_tkill (__X32_SYSCALL_BIT + 200)
#define __NR_time (__X32_SYSCALL_BIT + 201)
#define __NR_futex (__X32_SYSCALL_BIT + 202)
#define __NR_sched_setaffinity (__X32_SYSCALL_BIT + 203)
#define __NR_sched_getaffinity (__X32_SYSCALL_BIT + 204)
#define __NR_io_destroy (__X32_SYSCALL_BIT + 207)
#define __NR_io_getevents (__X32_SYSCALL_BIT + 208)
#define __NR_io_cancel (__X32_SYSCALL_BIT + 210)
#define __NR_lookup_dcookie (__X32_SYSCALL_BIT + 212)
#define __NR_epoll_create (__X32_SYSCALL_BIT + 213)
#define __NR_remap_file_pages (__X32_SYSCALL_BIT + 216)
#define __NR_getdents64 (__X32_SYSCALL_BIT + 217)
#define __NR_set_tid_address (__X32_SYSCALL_BIT + 218)
#define __NR_restart_syscall (__X32_SYSCALL_BIT + 219)
#define __NR_semtimedop (__X32_SYSCALL_BIT + 220)
#define __NR_fadvise64 (__X32_SYSCALL_BIT + 221)
#define __NR_timer_settime (__X32_SYSCALL_BIT + 223)
#define __NR_timer_gettime (__X32_SYSCALL_BIT + 224)
#define __NR_timer_getoverrun (__X32_SYSCALL_BIT + 225)
#define __NR_timer_delete (__X32_SYSCALL_BIT + 226)
#define __NR_clock_settime (__X32_SYSCALL_BIT + 227)
#define __NR_clock_gettime (__X32_SYSCALL_BIT + 228)
#define __NR_clock_getres (__X32_SYSCALL_BIT + 229)
#define __NR_clock_nanosleep (__X32_SYSCALL_BIT + 230)
#define __NR_exit_group (__X32_SYSCALL_BIT + 231)
#define __NR_epoll_wait (__X32_SYSCALL_BIT + 232)
#define __NR_epoll_ctl (__X32_SYSCALL_BIT + 233)
#define __NR_tgkill (__X32_SYSCALL_BIT + 234)
#define __NR_utimes (__X32_SYSCALL_BIT + 235)
#define __NR_mbind (__X32_SYSCALL_BIT + 237)
#define __NR_set_mempolicy (__X32_SYSCALL_BIT + 238)
#define __NR_get_mempolicy (__X32_SYSCALL_BIT + 239)
#define __NR_mq_open (__X32_SYSCALL_BIT + 240)
#define __NR_mq_unlink (__X32_SYSCALL_BIT + 241)
#define __NR_mq_timedsend (__X32_SYSCALL_BIT + 242)
#define __NR_mq_timedreceive (__X32_SYSCALL_BIT + 243)
#define __NR_mq_getsetattr (__X32_SYSCALL_BIT + 245)
#define __NR_add_key (__X32_SYSCALL_BIT + 248)
#define __NR_request_key (__X32_SYSCALL_BIT + 249)
#define __NR_keyctl (__X32_SYSCALL_BIT + 250)
#define __NR_ioprio_set (__X32_SYSCALL_BIT + 251)
#define __NR_ioprio_get (__X32_SYSCALL_BIT + 252)
#define __NR_inotify_init (__X32_SYSCALL_BIT + 253)
#define __NR_inotify_add_watch (__X32_SYSCALL_BIT + 254)
#define __NR_inotify_rm_watch (__X32_SYSCALL_BIT + 255)
#define __NR_migrate_pages (__X32_SYSCALL_BIT + 256)
#define __NR_openat (__X32_SYSCALL_BIT + 257)
#define __NR_mkdirat (__X32_SYSCALL_BIT + 258)
#define __NR_mknodat (__X32_SYSCALL_BIT + 259)
#define __NR_fchownat (__X32_SYSCALL_BIT + 260)
#define __NR_futimesat (__X32_SYSCALL_BIT + 261)
#define __NR_newfstatat (__X32_SYSCALL_BIT + 262)
#define __NR_unlinkat (__X32_SYSCALL_BIT + 263)
#define __NR_renameat (__X32_SYSCALL_BIT + 264)
#define __NR_linkat (__X32_SYSCALL_BIT + 265)
#define __NR_symlinkat (__X32_SYSCALL_BIT + 266)
#define __NR_readlinkat (__X32_SYSCALL_BIT + 267)
#define __NR_fchmodat (__X32_SYSCALL_BIT + 268)
#define __NR_faccessat (__X32_SYSCALL_BIT + 269)
#define __NR_pselect6 (__X32_SYSCALL_BIT + 270)
#define __NR_ppoll (__X32_SYSCALL_BIT + 271)
#define __NR_unshare (__X32_SYSCALL_BIT + 272)
#define __NR_splice (__X32_SYSCALL_BIT + 275)
#define __NR_tee (__X32_SYSCALL_BIT + 276)
#define __NR_sync_file_range (__X32_SYSCALL_BIT + 277)
#define __NR_utimensat (__X32_SYSCALL_BIT + 280)
#define __NR_epoll_pwait (__X32_SYSCALL_BIT + 281)
#define __NR_signalfd (__X32_SYSCALL_BIT + 282)
#define __NR_timerfd_create (__X32_SYSCALL_BIT + 283)
#define __NR_eventfd (__X32_SYSCALL_BIT + 284)
#define __NR_fallocate (__X32_SYSCALL_BIT + 285)
#define __NR_timerfd_settime (__X32_SYSCALL_BIT + 286)
#define __NR_timerfd_gettime (__X32_SYSCALL_BIT + 287)
#define __NR_accept4 (__X32_SYSCALL_BIT + 288)
#define __NR_signalfd4 (__X32_SYSCALL_BIT + 289)
#define __NR_eventfd2 (__X32_SYSCALL_BIT + 290)
#define __NR_epoll_create1 (__X32_SYSCALL_BIT + 291)
#define __NR_dup3 (__X32_SYSCALL_BIT + 292)
#define __NR_pipe2 (__X32_SYSCALL_BIT + 293)
#define __NR_inotify_init1 (__X32_SYSCALL_BIT + 294)
#define __NR_perf_event_open (__X32_SYSCALL_BIT + 298)
#define __NR_fanotify_init (__X32_SYSCALL_BIT + 300)
#define __NR_fanotify_mark (__X32_SYSCALL_BIT + 301)
#define __NR_prlimit64 (__X32_SYSCALL_BIT + 302)
#define __NR_name_to_handle_at (__X32_SYSCALL_BIT + 303)
#define __NR_open_by_handle_at (__X32_SYSCALL_BIT + 304)
#define __NR_clock_adjtime (__X32_SYSCALL_BIT + 305)
#define __NR_syncfs (__X32_SYSCALL_BIT + 306)
#define __NR_setns (__X32_SYSCALL_BIT + 308)
#define __NR_getcpu (__X32_SYSCALL_BIT + 309)
#define __NR_kcmp (__X32_SYSCALL_BIT + 312)
#define __NR_finit_module (__X32_SYSCALL_BIT + 313)
#define __NR_sched_setattr (__X32_SYSCALL_BIT + 314)
#define __NR_sched_getattr (__X32_SYSCALL_BIT + 315)
#define __NR_renameat2 (__X32_SYSCALL_BIT + 316)
#define __NR_seccomp (__X32_SYSCALL_BIT + 317)
#define __NR_getrandom (__X32_SYSCALL_BIT + 318)
#define __NR_memfd_create (__X32_SYSCALL_BIT + 319)
#define __NR_kexec_file_load (__X32_SYSCALL_BIT + 320)
#define __NR_bpf (__X32_SYSCALL_BIT + 321)
#define __NR_userfaultfd (__X32_SYSCALL_BIT + 323)
#define __NR_membarrier (__X32_SYSCALL_BIT + 324)
#define __NR_mlock2 (__X32_SYSCALL_BIT + 325)
#define __NR_copy_file_range (__X32_SYSCALL_BIT + 326)
#define __NR_pkey_mprotect (__X32_SYSCALL_BIT + 329)
#define __NR_pkey_alloc (__X32_SYSCALL_BIT + 330)
#define __NR_pkey_free (__X32_SYSCALL_BIT + 331)
#define __NR_statx (__X32_SYSCALL_BIT + 332)
#define __NR_io_pgetevents (__X32_SYSCALL_BIT + 333)
#define __NR_rseq (__X32_SYSCALL_BIT + 334)
#define __NR_pidfd_send_signal (__X32_SYSCALL_BIT + 424)
#define __NR_io_uring_setup (__X32_SYSCALL_BIT + 425)
#define __NR_io_uring_enter (__X32_SYSCALL_BIT + 426)
#define __NR_io_uring_register (__X32_SYSCALL_BIT + 427)
#define __NR_open_tree (__X32_SYSCALL_BIT + 428)
#define __NR_move_mount (__X32_SYSCALL_BIT + 429)
#define __NR_fsopen (__X32_SYSCALL_BIT + 430)
#define __NR_fsconfig (__X32_SYSCALL_BIT + 431)
#define __NR_fsmount (__X32_SYSCALL_BIT + 432)
#define __NR_fspick (__X32_SYSCALL_BIT + 433)
#define __NR_pidfd_open (__X32_SYSCALL_BIT + 434)
#define __NR_clone3 (__X32_SYSCALL_BIT + 435)
#define __NR_close_range (__X32_SYSCALL_BIT + 436)
#define __NR_openat2 (__X32_SYSCALL_BIT + 437)
#define __NR_pidfd_getfd (__X32_SYSCALL_BIT + 438)
#define __NR_faccessat2 (__X32_SYSCALL_BIT + 439)
#define __NR_process_madvise (__X32_SYSCALL_BIT + 440)
#define __NR_epoll_pwait2 (__X32_SYSCALL_BIT + 441)
#define __NR_mount_setattr (__X32_SYSCALL_BIT + 442)
#define __NR_quotactl_fd (__X32_SYSCALL_BIT + 443)
#define __NR_landlock_create_ruleset (__X32_SYSCALL_BIT + 444)
#define __NR_landlock_add_rule (__X32_SYSCALL_BIT + 445)
#define __NR_landlock_restrict_self (__X32_SYSCALL_BIT + 446)
#define __NR_memfd_secret (__X32_SYSCALL_BIT + 447)
#define __NR_process_mrelease (__X32_SYSCALL_BIT + 448)
#define __NR_futex_waitv (__X32_SYSCALL_BIT + 449)
#define __NR_set_mempolicy_home_node (__X32_SYSCALL_BIT + 450)
#define __NR_cachestat (__X32_SYSCALL_BIT + 451)
#define __NR_fchmodat2 (__X32_SYSCALL_BIT + 452)
#define __NR_map_shadow_stack (__X32_SYSCALL_BIT + 453)
#define __NR_futex_wake (__X32_SYSCALL_BIT + 454)
#define __NR_futex_wait (__X32_SYSCALL_BIT + 455)
#define __NR_futex_requeue (__X32_SYSCALL_BIT + 456)
#define __NR_statmount (__X32_SYSCALL_BIT + 457)
#define __NR_listmount (__X32_SYSCALL_BIT + 458)
#define __NR_lsm_get_self_attr (__X32_SYSCALL_BIT + 459)
#define __NR_lsm_set_self_attr (__X32_SYSCALL_BIT + 460)
#define __NR_lsm_list_modules (__X32_SYSCALL_BIT + 461)
#define __NR_mseal (__X32_SYSCALL_BIT + 462)
#define __NR_rt_sigaction (__X32_SYSCALL_BIT + 512)
#define __NR_rt_sigreturn (__X32_SYSCALL_BIT + 513)
#define __NR_ioctl (__X32_SYSCALL_BIT + 514)
#define __NR_readv (__X32_SYSCALL_BIT + 515)
#define __NR_writev (__X32_SYSCALL_BIT + 516)
#define __NR_recvfrom (__X32_SYSCALL_BIT + 517)
#define __NR_sendmsg (__X32_SYSCALL_BIT + 518)
#define __NR_recvmsg (__X32_SYSCALL_BIT + 519)
#define __NR_execve (__X32_SYSCALL_BIT + 520)
#define __NR_ptrace (__X32_SYSCALL_BIT + 521)
#define __NR_rt_sigpending (__X32_SYSCALL_BIT + 522)
#define __NR_rt_sigtimedwait (__X32_SYSCALL_BIT + 523)
#define __NR_rt_sigqueueinfo (__X32_SYSCALL_BIT + 524)
#define __NR_sigaltstack (__X32_SYSCALL_BIT + 525)
#define __NR_timer_create (__X32_SYSCALL_BIT + 526)
#define __NR_mq_notify (__X32_SYSCALL_BIT + 527)
#define __NR_kexec_load (__X32_SYSCALL_BIT + 528)
#define __NR_waitid (__X32_SYSCALL_BIT + 529)
#define __NR_set_robust_list (__X32_SYSCALL_BIT + 530)
#define __NR_get_robust_list (__X32_SYSCALL_BIT + 531)
#define __NR_vmsplice (__X32_SYSCALL_BIT + 532)
#define __NR_move_pages (__X32_SYSCALL_BIT + 533)
#define __NR_preadv (__X32_SYSCALL_BIT + 534)
#define __NR_pwritev (__X32_SYSCALL_BIT + 535)
#define __NR_rt_tgsigqueueinfo (__X32_SYSCALL_BIT + 536)
#define __NR_recvmmsg (__X32_SYSCALL_BIT + 537)
#define __NR_sendmmsg (__X32_SYSCALL_BIT + 538)
#define __NR_process_vm_readv (__X32_SYSCALL_BIT + 539)
#define __NR_process_vm_writev (__X32_SYSCALL_BIT + 540)
#define __NR_setsockopt (__X32_SYSCALL_BIT + 541)
#define __NR_getsockopt (__X32_SYSCALL_BIT + 542)
#define __NR_io_setup (__X32_SYSCALL_BIT + 543)
#define __NR_io_submit (__X32_SYSCALL_BIT + 544)
#define __NR_execveat (__X32_SYSCALL_BIT + 545)
#define __NR_preadv2 (__X32_SYSCALL_BIT + 546)
#define __NR_pwritev2 (__X32_SYSCALL_BIT + 547)
#endif

#ifdef __NR_FAST_atomic_update
#define SYS_FAST_atomic_update __NR_FAST_atomic_update
#endif

#ifdef __NR_FAST_cmpxchg
#define SYS_FAST_cmpxchg __NR_FAST_cmpxchg
#endif

#ifdef __NR_FAST_cmpxchg64
#define SYS_FAST_cmpxchg64 __NR_FAST_cmpxchg64
#endif

#ifdef __NR__llseek
#define SYS__llseek __NR__llseek
#endif

#ifdef __NR__newselect
#define SYS__newselect __NR__newselect
#endif

#ifdef __NR__sysctl
#define SYS__sysctl __NR__sysctl
#endif

#ifdef __NR_accept
#define SYS_accept __NR_accept
#endif

#ifdef __NR_accept4
#define SYS_accept4 __NR_accept4
#endif

#ifdef __NR_access
#define SYS_access __NR_access
#endif

#ifdef __NR_acct
#define SYS_acct __NR_acct
#endif

#ifdef __NR_acl_get
#define SYS_acl_get __NR_acl_get
#endif

#ifdef __NR_acl_set
#define SYS_acl_set __NR_acl_set
#endif

#ifdef __NR_add_key
#define SYS_add_key __NR_add_key
#endif

#ifdef __NR_adjtimex
#define SYS_adjtimex __NR_adjtimex
#endif

#ifdef __NR_afs_syscall
#define SYS_afs_syscall __NR_afs_syscall
#endif

#ifdef __NR_alarm
#define SYS_alarm __NR_alarm
#endif

#ifdef __NR_alloc_hugepages
#define SYS_alloc_hugepages __NR_alloc_hugepages
#endif

#ifdef __NR_arc_gettls
#define SYS_arc_gettls __NR_arc_gettls
#endif

#ifdef __NR_arc_settls
#define SYS_arc_settls __NR_arc_settls
#endif

#ifdef __NR_arc_usr_cmpxchg
#define SYS_arc_usr_cmpxchg __NR_arc_usr_cmpxchg
#endif

#ifdef __NR_arch_prctl
#define SYS_arch_prctl __NR_arch_prctl
#endif

#ifdef __NR_arm_fadvise64_64
#define SYS_arm_fadvise64_64 __NR_arm_fadvise64_64
#endif

#ifdef __NR_arm_sync_file_range
#define SYS_arm_sync_file_range __NR_arm_sync_file_range
#endif

#ifdef __NR_atomic_barrier
#define SYS_atomic_barrier __NR_atomic_barrier
#endif

#ifdef __NR_atomic_cmpxchg_32
#define SYS_atomic_cmpxchg_32 __NR_atomic_cmpxchg_32
#endif

#ifdef __NR_attrctl
#define SYS_attrctl __NR_attrctl
#endif

#ifdef __NR_bdflush
#define SYS_bdflush __NR_bdflush
#endif

#ifdef __NR_bind
#define SYS_bind __NR_bind
#endif

#ifdef __NR_bpf
#define SYS_bpf __NR_bpf
#endif

#ifdef __NR_break
#define SYS_break __NR_break
#endif

#ifdef __NR_breakpoint
#define SYS_breakpoint __NR_breakpoint
#endif

#ifdef __NR_brk
#define SYS_brk __NR_brk
#endif

#ifdef __NR_cachectl
#define SYS_cachectl __NR_cachectl
#endif

#ifdef __NR_cacheflush
#define SYS_cacheflush __NR_cacheflush
#endif

#ifdef __NR_cachestat
#define SYS_cachestat __NR_cachestat
#endif

#ifdef __NR_capget
#define SYS_capget __NR_capget
#endif

#ifdef __NR_capset
#define SYS_capset __NR_capset
#endif

#ifdef __NR_chdir
#define SYS_chdir __NR_chdir
#endif

#ifdef __NR_chmod
#define SYS_chmod __NR_chmod
#endif

#ifdef __NR_chown
#define SYS_chown __NR_chown
#endif

#ifdef __NR_chown32
#define SYS_chown32 __NR_chown32
#endif

#ifdef __NR_chroot
#define SYS_chroot __NR_chroot
#endif

#ifdef __NR_clock_adjtime
#define SYS_clock_adjtime __NR_clock_adjtime
#endif

#ifdef __NR_clock_adjtime64
#define SYS_clock_adjtime64 __NR_clock_adjtime64
#endif

#ifdef __NR_clock_getres
#define SYS_clock_getres __NR_clock_getres
#endif

#ifdef __NR_clock_getres_time64
#define SYS_clock_getres_time64 __NR_clock_getres_time64
#endif

#ifdef __NR_clock_gettime
#define SYS_clock_gettime __NR_clock_gettime
#endif

#ifdef __NR_clock_gettime64
#define SYS_clock_gettime64 __NR_clock_gettime64
#endif

#ifdef __NR_clock_nanosleep
#define SYS_clock_nanosleep __NR_clock_nanosleep
#endif

#ifdef __NR_clock_nanosleep_time64
#define SYS_clock_nanosleep_time64 __NR_clock_nanosleep_time64
#endif

#ifdef __NR_clock_settime
#define SYS_clock_settime __NR_clock_settime
#endif

#ifdef __NR_clock_settime64
#define SYS_clock_settime64 __NR_clock_settime64
#endif

#ifdef __NR_clone
#define SYS_clone __NR_clone
#endif

#ifdef __NR_clone2
#define SYS_clone2 __NR_clone2
#endif

#ifdef __NR_clone3
#define SYS_clone3 __NR_clone3
#endif

#ifdef __NR_close
#define SYS_close __NR_close
#endif

#ifdef __NR_close_range
#define SYS_close_range __NR_close_range
#endif

#ifdef __NR_cmpxchg_badaddr
#define SYS_cmpxchg_badaddr __NR_cmpxchg_badaddr
#endif

#ifdef __NR_connect
#define SYS_connect __NR_connect
#endif

#ifdef __NR_copy_file_range
#define SYS_copy_file_range __NR_copy_file_range
#endif

#ifdef __NR_creat
#define SYS_creat __NR_creat
#endif

#ifdef __NR_create_module
#define SYS_create_module __NR_create_module
#endif

#ifdef __NR_delete_module
#define SYS_delete_module __NR_delete_module
#endif

#ifdef __NR_dipc
#define SYS_dipc __NR_dipc
#endif

#ifdef __NR_dup
#define SYS_dup __NR_dup
#endif

#ifdef __NR_dup2
#define SYS_dup2 __NR_dup2
#endif

#ifdef __NR_dup3
#define SYS_dup3 __NR_dup3
#endif

#ifdef __NR_epoll_create
#define SYS_epoll_create __NR_epoll_create
#endif

#ifdef __NR_epoll_create1
#define SYS_epoll_create1 __NR_epoll_create1
#endif

#ifdef __NR_epoll_ctl
#define SYS_epoll_ctl __NR_epoll_ctl
#endif

#ifdef __NR_epoll_ctl_old
#define SYS_epoll_ctl_old __NR_epoll_ctl_old
#endif

#ifdef __NR_epoll_pwait
#define SYS_epoll_pwait __NR_epoll_pwait
#endif

#ifdef __NR_epoll_pwait2
#define SYS_epoll_pwait2 __NR_epoll_pwait2
#endif

#ifdef __NR_epoll_wait
#define SYS_epoll_wait __NR_epoll_wait
#endif

#ifdef __NR_epoll_wait_old
#define SYS_epoll_wait_old __NR_epoll_wait_old
#endif

#ifdef __NR_eventfd
#define SYS_eventfd __NR_eventfd
#endif

#ifdef __NR_eventfd2
#define SYS_eventfd2 __NR_eventfd2
#endif

#ifdef __NR_exec_with_loader
#define SYS_exec_with_loader __NR_exec_with_loader
#endif

#ifdef __NR_execv
#define SYS_execv __NR_execv
#endif

#ifdef __NR_execve
#define SYS_execve __NR_execve
#endif

#ifdef __NR_execveat
#define SYS_execveat __NR_execveat
#endif

#ifdef __NR_exit
#define SYS_exit __NR_exit
#endif

#ifdef __NR_exit_group
#define SYS_exit_group __NR_exit_group
#endif

#ifdef __NR_faccessat
#define SYS_faccessat __NR_faccessat
#endif

#ifdef __NR_faccessat2
#define SYS_faccessat2 __NR_faccessat2
#endif

#ifdef __NR_fadvise64
#define SYS_fadvise64 __NR_fadvise64
#endif

#ifdef __NR_fadvise64_64
#define SYS_fadvise64_64 __NR_fadvise64_64
#endif

#ifdef __NR_fallocate
#define SYS_fallocate __NR_fallocate
#endif

#ifdef __NR_fanotify_init
#define SYS_fanotify_init __NR_fanotify_init
#endif

#ifdef __NR_fanotify_mark
#define SYS_fanotify_mark __NR_fanotify_mark
#endif

#ifdef __NR_fchdir
#define SYS_fchdir __NR_fchdir
#endif

#ifdef __NR_fchmod
#define SYS_fchmod __NR_fchmod
#endif

#ifdef __NR_fchmodat
#define SYS_fchmodat __NR_fchmodat
#endif

#ifdef __NR_fchmodat2
#define SYS_fchmodat2 __NR_fchmodat2
#endif

#ifdef __NR_fchown
#define SYS_fchown __NR_fchown
#endif

#ifdef __NR_fchown32
#define SYS_fchown32 __NR_fchown32
#endif

#ifdef __NR_fchownat
#define SYS_fchownat __NR_fchownat
#endif

#ifdef __NR_fcntl
#define SYS_fcntl __NR_fcntl
#endif

#ifdef __NR_fcntl64
#define SYS_fcntl64 __NR_fcntl64
#endif

#ifdef __NR_fdatasync
#define SYS_fdatasync __NR_fdatasync
#endif

#ifdef __NR_fgetxattr
#define SYS_fgetxattr __NR_fgetxattr
#endif

#ifdef __NR_finit_module
#define SYS_finit_module __NR_finit_module
#endif

#ifdef __NR_flistxattr
#define SYS_flistxattr __NR_flistxattr
#endif

#ifdef __NR_flock
#define SYS_flock __NR_flock
#endif

#ifdef __NR_fork
#define SYS_fork __NR_fork
#endif

#ifdef __NR_fp_udfiex_crtl
#define SYS_fp_udfiex_crtl __NR_fp_udfiex_crtl
#endif

#ifdef __NR_free_hugepages
#define SYS_free_hugepages __NR_free_hugepages
#endif

#ifdef __NR_fremovexattr
#define SYS_fremovexattr __NR_fremovexattr
#endif

#ifdef __NR_fsconfig
#define SYS_fsconfig __NR_fsconfig
#endif

#ifdef __NR_fsetxattr
#define SYS_fsetxattr __NR_fsetxattr
#endif

#ifdef __NR_fsmount
#define SYS_fsmount __NR_fsmount
#endif

#ifdef __NR_fsopen
#define SYS_fsopen __NR_fsopen
#endif

#ifdef __NR_fspick
#define SYS_fspick __NR_fspick
#endif

#ifdef __NR_fstat
#define SYS_fstat __NR_fstat
#endif

#ifdef __NR_fstat64
#define SYS_fstat64 __NR_fstat64
#endif

#ifdef __NR_fstatat64
#define SYS_fstatat64 __NR_fstatat64
#endif

#ifdef __NR_fstatfs
#define SYS_fstatfs __NR_fstatfs
#endif

#ifdef __NR_fstatfs64
#define SYS_fstatfs64 __NR_fstatfs64
#endif

#ifdef __NR_fsync
#define SYS_fsync __NR_fsync
#endif

#ifdef __NR_ftime
#define SYS_ftime __NR_ftime
#endif

#ifdef __NR_ftruncate
#define SYS_ftruncate __NR_ftruncate
#endif

#ifdef __NR_ftruncate64
#define SYS_ftruncate64 __NR_ftruncate64
#endif

#ifdef __NR_futex
#define SYS_futex __NR_futex
#endif

#ifdef __NR_futex_requeue
#define SYS_futex_requeue __NR_futex_requeue
#endif

#ifdef __NR_futex_time64
#define SYS_futex_time64 __NR_futex_time64
#endif

#ifdef __NR_futex_wait
#define SYS_futex_wait __NR_futex_wait
#endif

#ifdef __NR_futex_waitv
#define SYS_futex_waitv __NR_futex_waitv
#endif

#ifdef __NR_futex_wake
#define SYS_futex_wake __NR_futex_wake
#endif

#ifdef __NR_futimesat
#define SYS_futimesat __NR_futimesat
#endif

#ifdef __NR_get_kernel_syms
#define SYS_get_kernel_syms __NR_get_kernel_syms
#endif

#ifdef __NR_get_mempolicy
#define SYS_get_mempolicy __NR_get_mempolicy
#endif

#ifdef __NR_get_robust_list
#define SYS_get_robust_list __NR_get_robust_list
#endif

#ifdef __NR_get_thread_area
#define SYS_get_thread_area __NR_get_thread_area
#endif

#ifdef __NR_get_tls
#define SYS_get_tls __NR_get_tls
#endif

#ifdef __NR_getcpu
#define SYS_getcpu __NR_getcpu
#endif

#ifdef __NR_getcwd
#define SYS_getcwd __NR_getcwd
#endif

#ifdef __NR_getdents
#define SYS_getdents __NR_getdents
#endif

#ifdef __NR_getdents64
#define SYS_getdents64 __NR_getdents64
#endif

#ifdef __NR_getdomainname
#define SYS_getdomainname __NR_getdomainname
#endif

#ifdef __NR_getdtablesize
#define SYS_getdtablesize __NR_getdtablesize
#endif

#ifdef __NR_getegid
#define SYS_getegid __NR_getegid
#endif

#ifdef __NR_getegid32
#define SYS_getegid32 __NR_getegid32
#endif

#ifdef __NR_geteuid
#define SYS_geteuid __NR_geteuid
#endif

#ifdef __NR_geteuid32
#define SYS_geteuid32 __NR_geteuid32
#endif

#ifdef __NR_getgid
#define SYS_getgid __NR_getgid
#endif

#ifdef __NR_getgid32
#define SYS_getgid32 __NR_getgid32
#endif

#ifdef __NR_getgroups
#define SYS_getgroups __NR_getgroups
#endif

#ifdef __NR_getgroups32
#define SYS_getgroups32 __NR_getgroups32
#endif

#ifdef __NR_gethostname
#define SYS_gethostname __NR_gethostname
#endif

#ifdef __NR_getitimer
#define SYS_getitimer __NR_getitimer
#endif

#ifdef __NR_getpagesize
#define SYS_getpagesize __NR_getpagesize
#endif

#ifdef __NR_getpeername
#define SYS_getpeername __NR_getpeername
#endif

#ifdef __NR_getpgid
#define SYS_getpgid __NR_getpgid
#endif

#ifdef __NR_getpgrp
#define SYS_getpgrp __NR_getpgrp
#endif

#ifdef __NR_getpid
#define SYS_getpid __NR_getpid
#endif

#ifdef __NR_getpmsg
#define SYS_getpmsg __NR_getpmsg
#endif

#ifdef __NR_getppid
#define SYS_getppid __NR_getppid
#endif

#ifdef __NR_getpriority
#define SYS_getpriority __NR_getpriority
#endif

#ifdef __NR_getrandom
#define SYS_getrandom __NR_getrandom
#endif

#ifdef __NR_getresgid
#define SYS_getresgid __NR_getresgid
#endif

#ifdef __NR_getresgid32
#define SYS_getresgid32 __NR_getresgid32
#endif

#ifdef __NR_getresuid
#define SYS_getresuid __NR_getresuid
#endif

#ifdef __NR_getresuid32
#define SYS_getresuid32 __NR_getresuid32
#endif

#ifdef __NR_getrlimit
#define SYS_getrlimit __NR_getrlimit
#endif

#ifdef __NR_getrusage
#define SYS_getrusage __NR_getrusage
#endif

#ifdef __NR_getsid
#define SYS_getsid __NR_getsid
#endif

#ifdef __NR_getsockname
#define SYS_getsockname __NR_getsockname
#endif

#ifdef __NR_getsockopt
#define SYS_getsockopt __NR_getsockopt
#endif

#ifdef __NR_gettid
#define SYS_gettid __NR_gettid
#endif

#ifdef __NR_gettimeofday
#define SYS_gettimeofday __NR_gettimeofday
#endif

#ifdef __NR_getuid
#define SYS_getuid __NR_getuid
#endif

#ifdef __NR_getuid32
#define SYS_getuid32 __NR_getuid32
#endif

#ifdef __NR_getunwind
#define SYS_getunwind __NR_getunwind
#endif

#ifdef __NR_getxattr
#define SYS_getxattr __NR_getxattr
#endif

#ifdef __NR_getxgid
#define SYS_getxgid __NR_getxgid
#endif

#ifdef __NR_getxpid
#define SYS_getxpid __NR_getxpid
#endif

#ifdef __NR_getxuid
#define SYS_getxuid __NR_getxuid
#endif

#ifdef __NR_gtty
#define SYS_gtty __NR_gtty
#endif

#ifdef __NR_idle
#define SYS_idle __NR_idle
#endif

#ifdef __NR_init_module
#define SYS_init_module __NR_init_module
#endif

#ifdef __NR_inotify_add_watch
#define SYS_inotify_add_watch __NR_inotify_add_watch
#endif

#ifdef __NR_inotify_init
#define SYS_inotify_init __NR_inotify_init
#endif

#ifdef __NR_inotify_init1
#define SYS_inotify_init1 __NR_inotify_init1
#endif

#ifdef __NR_inotify_rm_watch
#define SYS_inotify_rm_watch __NR_inotify_rm_watch
#endif

#ifdef __NR_io_cancel
#define SYS_io_cancel __NR_io_cancel
#endif

#ifdef __NR_io_destroy
#define SYS_io_destroy __NR_io_destroy
#endif

#ifdef __NR_io_getevents
#define SYS_io_getevents __NR_io_getevents
#endif

#ifdef __NR_io_pgetevents
#define SYS_io_pgetevents __NR_io_pgetevents
#endif

#ifdef __NR_io_pgetevents_time64
#define SYS_io_pgetevents_time64 __NR_io_pgetevents_time64
#endif

#ifdef __NR_io_setup
#define SYS_io_setup __NR_io_setup
#endif

#ifdef __NR_io_submit
#define SYS_io_submit __NR_io_submit
#endif

#ifdef __NR_io_uring_enter
#define SYS_io_uring_enter __NR_io_uring_enter
#endif

#ifdef __NR_io_uring_register
#define SYS_io_uring_register __NR_io_uring_register
#endif

#ifdef __NR_io_uring_setup
#define SYS_io_uring_setup __NR_io_uring_setup
#endif

#ifdef __NR_ioctl
#define SYS_ioctl __NR_ioctl
#endif

#ifdef __NR_ioperm
#define SYS_ioperm __NR_ioperm
#endif

#ifdef __NR_iopl
#define SYS_iopl __NR_iopl
#endif

#ifdef __NR_ioprio_get
#define SYS_ioprio_get __NR_ioprio_get
#endif

#ifdef __NR_ioprio_set
#define SYS_ioprio_set __NR_ioprio_set
#endif

#ifdef __NR_ipc
#define SYS_ipc __NR_ipc
#endif

#ifdef __NR_kcmp
#define SYS_kcmp __NR_kcmp
#endif

#ifdef __NR_kern_features
#define SYS_kern_features __NR_kern_features
#endif

#ifdef __NR_kexec_file_load
#define SYS_kexec_file_load __NR_kexec_file_load
#endif

#ifdef __NR_kexec_load
#define SYS_kexec_load __NR_kexec_load
#endif

#ifdef __NR_keyctl
#define SYS_keyctl __NR_keyctl
#endif

#ifdef __NR_kill
#define SYS_kill __NR_kill
#endif

#ifdef __NR_landlock_add_rule
#define SYS_landlock_add_rule __NR_landlock_add_rule
#endif

#ifdef __NR_landlock_create_ruleset
#define SYS_landlock_create_ruleset __NR_landlock_create_ruleset
#endif

#ifdef __NR_landlock_restrict_self
#define SYS_landlock_restrict_self __NR_landlock_restrict_self
#endif

#ifdef __NR_lchown
#define SYS_lchown __NR_lchown
#endif

#ifdef __NR_lchown32
#define SYS_lchown32 __NR_lchown32
#endif

#ifdef __NR_lgetxattr
#define SYS_lgetxattr __NR_lgetxattr
#endif

#ifdef __NR_link
#define SYS_link __NR_link
#endif

#ifdef __NR_linkat
#define SYS_linkat __NR_linkat
#endif

#ifdef __NR_listen
#define SYS_listen __NR_listen
#endif

#ifdef __NR_listxattr
#define SYS_listxattr __NR_listxattr
#endif

#ifdef __NR_llistxattr
#define SYS_llistxattr __NR_llistxattr
#endif

#ifdef __NR_llseek
#define SYS_llseek __NR_llseek
#endif

#ifdef __NR_lock
#define SYS_lock __NR_lock
#endif

#ifdef __NR_lookup_dcookie
#define SYS_lookup_dcookie __NR_lookup_dcookie
#endif

#ifdef __NR_lremovexattr
#define SYS_lremovexattr __NR_lremovexattr
#endif

#ifdef __NR_lseek
#define SYS_lseek __NR_lseek
#endif

#ifdef __NR_lsetxattr
#define SYS_lsetxattr __NR_lsetxattr
#endif

#ifdef __NR_lstat
#define SYS_lstat __NR_lstat
#endif

#ifdef __NR_lstat64
#define SYS_lstat64 __NR_lstat64
#endif

#ifdef __NR_madvise
#define SYS_madvise __NR_madvise
#endif

#ifdef __NR_map_shadow_stack
#define SYS_map_shadow_stack __NR_map_shadow_stack
#endif

#ifdef __NR_mbind
#define SYS_mbind __NR_mbind
#endif

#ifdef __NR_membarrier
#define SYS_membarrier __NR_membarrier
#endif

#ifdef __NR_memfd_create
#define SYS_memfd_create __NR_memfd_create
#endif

#ifdef __NR_memfd_secret
#define SYS_memfd_secret __NR_memfd_secret
#endif

#ifdef __NR_memory_ordering
#define SYS_memory_ordering __NR_memory_ordering
#endif

#ifdef __NR_migrate_pages
#define SYS_migrate_pages __NR_migrate_pages
#endif

#ifdef __NR_mincore
#define SYS_mincore __NR_mincore
#endif

#ifdef __NR_mkdir
#define SYS_mkdir __NR_mkdir
#endif

#ifdef __NR_mkdirat
#define SYS_mkdirat __NR_mkdirat
#endif

#ifdef __NR_mknod
#define SYS_mknod __NR_mknod
#endif

#ifdef __NR_mknodat
#define SYS_mknodat __NR_mknodat
#endif

#ifdef __NR_mlock
#define SYS_mlock __NR_mlock
#endif

#ifdef __NR_mlock2
#define SYS_mlock2 __NR_mlock2
#endif

#ifdef __NR_mlockall
#define SYS_mlockall __NR_mlockall
#endif

#ifdef __NR_mmap
#define SYS_mmap __NR_mmap
#endif

#ifdef __NR_mmap2
#define SYS_mmap2 __NR_mmap2
#endif

#ifdef __NR_modify_ldt
#define SYS_modify_ldt __NR_modify_ldt
#endif

#ifdef __NR_mount
#define SYS_mount __NR_mount
#endif

#ifdef __NR_mount_setattr
#define SYS_mount_setattr __NR_mount_setattr
#endif

#ifdef __NR_move_mount
#define SYS_move_mount __NR_move_mount
#endif

#ifdef __NR_move_pages
#define SYS_move_pages __NR_move_pages
#endif

#ifdef __NR_mprotect
#define SYS_mprotect __NR_mprotect
#endif

#ifdef __NR_mpx
#define SYS_mpx __NR_mpx
#endif

#ifdef __NR_mq_getsetattr
#define SYS_mq_getsetattr __NR_mq_getsetattr
#endif

#ifdef __NR_mq_notify
#define SYS_mq_notify __NR_mq_notify
#endif

#ifdef __NR_mq_open
#define SYS_mq_open __NR_mq_open
#endif

#ifdef __NR_mq_timedreceive
#define SYS_mq_timedreceive __NR_mq_timedreceive
#endif

#ifdef __NR_mq_timedreceive_time64
#define SYS_mq_timedreceive_time64 __NR_mq_timedreceive_time64
#endif

#ifdef __NR_mq_timedsend
#define SYS_mq_timedsend __NR_mq_timedsend
#endif

#ifdef __NR_mq_timedsend_time64
#define SYS_mq_timedsend_time64 __NR_mq_timedsend_time64
#endif

#ifdef __NR_mq_unlink
#define SYS_mq_unlink __NR_mq_unlink
#endif

#ifdef __NR_mremap
#define SYS_mremap __NR_mremap
#endif

#ifdef __NR_msgctl
#define SYS_msgctl __NR_msgctl
#endif

#ifdef __NR_msgget
#define SYS_msgget __NR_msgget
#endif

#ifdef __NR_msgrcv
#define SYS_msgrcv __NR_msgrcv
#endif

#ifdef __NR_msgsnd
#define SYS_msgsnd __NR_msgsnd
#endif

#ifdef __NR_msync
#define SYS_msync __NR_msync
#endif

#ifdef __NR_multiplexer
#define SYS_multiplexer __NR_multiplexer
#endif

#ifdef __NR_munlock
#define SYS_munlock __NR_munlock
#endif

#ifdef __NR_munlockall
#define SYS_munlockall __NR_munlockall
#endif

#ifdef __NR_munmap
#define SYS_munmap __NR_munmap
#endif

#ifdef __NR_name_to_handle_at
#define SYS_name_to_handle_at __NR_name_to_handle_at
#endif

#ifdef __NR_nanosleep
#define SYS_nanosleep __NR_nanosleep
#endif

#ifdef __NR_newfstatat
#define SYS_newfstatat __NR_newfstatat
#endif

#ifdef __NR_nfsservctl
#define SYS_nfsservctl __NR_nfsservctl
#endif

#ifdef __NR_ni_syscall
#define SYS_ni_syscall __NR_ni_syscall
#endif

#ifdef __NR_nice
#define SYS_nice __NR_nice
#endif

#ifdef __NR_old_adjtimex
#define SYS_old_adjtimex __NR_old_adjtimex
#endif

#ifdef __NR_old_getpagesize
#define SYS_old_getpagesize __NR_old_getpagesize
#endif

#ifdef __NR_oldfstat
#define SYS_oldfstat __NR_oldfstat
#endif

#ifdef __NR_oldlstat
#define SYS_oldlstat __NR_oldlstat
#endif

#ifdef __NR_oldolduname
#define SYS_oldolduname __NR_oldolduname
#endif

#ifdef __NR_oldstat
#define SYS_oldstat __NR_oldstat
#endif

#ifdef __NR_oldumount
#define SYS_oldumount __NR_oldumount
#endif

#ifdef __NR_olduname
#define SYS_olduname __NR_olduname
#endif

#ifdef __NR_open
#define SYS_open __NR_open
#endif

#ifdef __NR_open_by_handle_at
#define SYS_open_by_handle_at __NR_open_by_handle_at
#endif

#ifdef __NR_open_tree
#define SYS_open_tree __NR_open_tree
#endif

#ifdef __NR_openat
#define SYS_openat __NR_openat
#endif

#ifdef __NR_openat2
#define SYS_openat2 __NR_openat2
#endif

#ifdef __NR_or1k_atomic
#define SYS_or1k_atomic __NR_or1k_atomic
#endif

#ifdef __NR_osf_adjtime
#define SYS_osf_adjtime __NR_osf_adjtime
#endif

#ifdef __NR_osf_afs_syscall
#define SYS_osf_afs_syscall __NR_osf_afs_syscall
#endif

#ifdef __NR_osf_alt_plock
#define SYS_osf_alt_plock __NR_osf_alt_plock
#endif

#ifdef __NR_osf_alt_setsid
#define SYS_osf_alt_setsid __NR_osf_alt_setsid
#endif

#ifdef __NR_osf_alt_sigpending
#define SYS_osf_alt_sigpending __NR_osf_alt_sigpending
#endif

#ifdef __NR_osf_asynch_daemon
#define SYS_osf_asynch_daemon __NR_osf_asynch_daemon
#endif

#ifdef __NR_osf_audcntl
#define SYS_osf_audcntl __NR_osf_audcntl
#endif

#ifdef __NR_osf_audgen
#define SYS_osf_audgen __NR_osf_audgen
#endif

#ifdef __NR_osf_chflags
#define SYS_osf_chflags __NR_osf_chflags
#endif

#ifdef __NR_osf_execve
#define SYS_osf_execve __NR_osf_execve
#endif

#ifdef __NR_osf_exportfs
#define SYS_osf_exportfs __NR_osf_exportfs
#endif

#ifdef __NR_osf_fchflags
#define SYS_osf_fchflags __NR_osf_fchflags
#endif

#ifdef __NR_osf_fdatasync
#define SYS_osf_fdatasync __NR_osf_fdatasync
#endif

#ifdef __NR_osf_fpathconf
#define SYS_osf_fpathconf __NR_osf_fpathconf
#endif

#ifdef __NR_osf_fstat
#define SYS_osf_fstat __NR_osf_fstat
#endif

#ifdef __NR_osf_fstatfs
#define SYS_osf_fstatfs __NR_osf_fstatfs
#endif

#ifdef __NR_osf_fstatfs64
#define SYS_osf_fstatfs64 __NR_osf_fstatfs64
#endif

#ifdef __NR_osf_fuser
#define SYS_osf_fuser __NR_osf_fuser
#endif

#ifdef __NR_osf_getaddressconf
#define SYS_osf_getaddressconf __NR_osf_getaddressconf
#endif

#ifdef __NR_osf_getdirentries
#define SYS_osf_getdirentries __NR_osf_getdirentries
#endif

#ifdef __NR_osf_getdomainname
#define SYS_osf_getdomainname __NR_osf_getdomainname
#endif

#ifdef __NR_osf_getfh
#define SYS_osf_getfh __NR_osf_getfh
#endif

#ifdef __NR_osf_getfsstat
#define SYS_osf_getfsstat __NR_osf_getfsstat
#endif

#ifdef __NR_osf_gethostid
#define SYS_osf_gethostid __NR_osf_gethostid
#endif

#ifdef __NR_osf_getitimer
#define SYS_osf_getitimer __NR_osf_getitimer
#endif

#ifdef __NR_osf_getlogin
#define SYS_osf_getlogin __NR_osf_getlogin
#endif

#ifdef __NR_osf_getmnt
#define SYS_osf_getmnt __NR_osf_getmnt
#endif

#ifdef __NR_osf_getrusage
#define SYS_osf_getrusage __NR_osf_getrusage
#endif

#ifdef __NR_osf_getsysinfo
#define SYS_osf_getsysinfo __NR_osf_getsysinfo
#endif

#ifdef __NR_osf_gettimeofday
#define SYS_osf_gettimeofday __NR_osf_gettimeofday
#endif

#ifdef __NR_osf_kloadcall
#define SYS_osf_kloadcall __NR_osf_kloadcall
#endif

#ifdef __NR_osf_kmodcall
#define SYS_osf_kmodcall __NR_osf_kmodcall
#endif

#ifdef __NR_osf_lstat
#define SYS_osf_lstat __NR_osf_lstat
#endif

#ifdef __NR_osf_memcntl
#define SYS_osf_memcntl __NR_osf_memcntl
#endif

#ifdef __NR_osf_mincore
#define SYS_osf_mincore __NR_osf_mincore
#endif

#ifdef __NR_osf_mount
#define SYS_osf_mount __NR_osf_mount
#endif

#ifdef __NR_osf_mremap
#define SYS_osf_mremap __NR_osf_mremap
#endif

#ifdef __NR_osf_msfs_syscall
#define SYS_osf_msfs_syscall __NR_osf_msfs_syscall
#endif

#ifdef __NR_osf_msleep
#define SYS_osf_msleep __NR_osf_msleep
#endif

#ifdef __NR_osf_mvalid
#define SYS_osf_mvalid __NR_osf_mvalid
#endif

#ifdef __NR_osf_mwakeup
#define SYS_osf_mwakeup __NR_osf_mwakeup
#endif

#ifdef __NR_osf_naccept
#define SYS_osf_naccept __NR_osf_naccept
#endif

#ifdef __NR_osf_nfssvc
#define SYS_osf_nfssvc __NR_osf_nfssvc
#endif

#ifdef __NR_osf_ngetpeername
#define SYS_osf_ngetpeername __NR_osf_ngetpeername
#endif

#ifdef __NR_osf_ngetsockname
#define SYS_osf_ngetsockname __NR_osf_ngetsockname
#endif

#ifdef __NR_osf_nrecvfrom
#define SYS_osf_nrecvfrom __NR_osf_nrecvfrom
#endif

#ifdef __NR_osf_nrecvmsg
#define SYS_osf_nrecvmsg __NR_osf_nrecvmsg
#endif

#ifdef __NR_osf_nsendmsg
#define SYS_osf_nsendmsg __NR_osf_nsendmsg
#endif

#ifdef __NR_osf_ntp_adjtime
#define SYS_osf_ntp_adjtime __NR_osf_ntp_adjtime
#endif

#ifdef __NR_osf_ntp_gettime
#define SYS_osf_ntp_gettime __NR_osf_ntp_gettime
#endif

#ifdef __NR_osf_old_creat
#define SYS_osf_old_creat __NR_osf_old_creat
#endif

#ifdef __NR_osf_old_fstat
#define SYS_osf_old_fstat __NR_osf_old_fstat
#endif

#ifdef __NR_osf_old_getpgrp
#define SYS_osf_old_getpgrp __NR_osf_old_getpgrp
#endif

#ifdef __NR_osf_old_killpg
#define SYS_osf_old_killpg __NR_osf_old_killpg
#endif

#ifdef __NR_osf_old_lstat
#define SYS_osf_old_lstat __NR_osf_old_lstat
#endif

#ifdef __NR_osf_old_open
#define SYS_osf_old_open __NR_osf_old_open
#endif

#ifdef __NR_osf_old_sigaction
#define SYS_osf_old_sigaction __NR_osf_old_sigaction
#endif

#ifdef __NR_osf_old_sigblock
#define SYS_osf_old_sigblock __NR_osf_old_sigblock
#endif

#ifdef __NR_osf_old_sigreturn
#define SYS_osf_old_sigreturn __NR_osf_old_sigreturn
#endif

#ifdef __NR_osf_old_sigsetmask
#define SYS_osf_old_sigsetmask __NR_osf_old_sigsetmask
#endif

#ifdef __NR_osf_old_sigvec
#define SYS_osf_old_sigvec __NR_osf_old_sigvec
#endif

#ifdef __NR_osf_old_stat
#define SYS_osf_old_stat __NR_osf_old_stat
#endif

#ifdef __NR_osf_old_vadvise
#define SYS_osf_old_vadvise __NR_osf_old_vadvise
#endif

#ifdef __NR_osf_old_vtrace
#define SYS_osf_old_vtrace __NR_osf_old_vtrace
#endif

#ifdef __NR_osf_old_wait
#define SYS_osf_old_wait __NR_osf_old_wait
#endif

#ifdef __NR_osf_oldquota
#define SYS_osf_oldquota __NR_osf_oldquota
#endif

#ifdef __NR_osf_pathconf
#define SYS_osf_pathconf __NR_osf_pathconf
#endif

#ifdef __NR_osf_pid_block
#define SYS_osf_pid_block __NR_osf_pid_block
#endif

#ifdef __NR_osf_pid_unblock
#define SYS_osf_pid_unblock __NR_osf_pid_unblock
#endif

#ifdef __NR_osf_plock
#define SYS_osf_plock __NR_osf_plock
#endif

#ifdef __NR_osf_priocntlset
#define SYS_osf_priocntlset __NR_osf_priocntlset
#endif

#ifdef __NR_osf_profil
#define SYS_osf_profil __NR_osf_profil
#endif

#ifdef __NR_osf_proplist_syscall
#define SYS_osf_proplist_syscall __NR_osf_proplist_syscall
#endif

#ifdef __NR_osf_reboot
#define SYS_osf_reboot __NR_osf_reboot
#endif

#ifdef __NR_osf_revoke
#define SYS_osf_revoke __NR_osf_revoke
#endif

#ifdef __NR_osf_sbrk
#define SYS_osf_sbrk __NR_osf_sbrk
#endif

#ifdef __NR_osf_security
#define SYS_osf_security __NR_osf_security
#endif

#ifdef __NR_osf_select
#define SYS_osf_select __NR_osf_select
#endif

#ifdef __NR_osf_set_program_attributes
#define SYS_osf_set_program_attributes __NR_osf_set_program_attributes
#endif

#ifdef __NR_osf_set_speculative
#define SYS_osf_set_speculative __NR_osf_set_speculative
#endif

#ifdef __NR_osf_sethostid
#define SYS_osf_sethostid __NR_osf_sethostid
#endif

#ifdef __NR_osf_setitimer
#define SYS_osf_setitimer __NR_osf_setitimer
#endif

#ifdef __NR_osf_setlogin
#define SYS_osf_setlogin __NR_osf_setlogin
#endif

#ifdef __NR_osf_setsysinfo
#define SYS_osf_setsysinfo __NR_osf_setsysinfo
#endif

#ifdef __NR_osf_settimeofday
#define SYS_osf_settimeofday __NR_osf_settimeofday
#endif

#ifdef __NR_osf_shmat
#define SYS_osf_shmat __NR_osf_shmat
#endif

#ifdef __NR_osf_signal
#define SYS_osf_signal __NR_osf_signal
#endif

#ifdef __NR_osf_sigprocmask
#define SYS_osf_sigprocmask __NR_osf_sigprocmask
#endif

#ifdef __NR_osf_sigsendset
#define SYS_osf_sigsendset __NR_osf_sigsendset
#endif

#ifdef __NR_osf_sigstack
#define SYS_osf_sigstack __NR_osf_sigstack
#endif

#ifdef __NR_osf_sigwaitprim
#define SYS_osf_sigwaitprim __NR_osf_sigwaitprim
#endif

#ifdef __NR_osf_sstk
#define SYS_osf_sstk __NR_osf_sstk
#endif

#ifdef __NR_osf_stat
#define SYS_osf_stat __NR_osf_stat
#endif

#ifdef __NR_osf_statfs
#define SYS_osf_statfs __NR_osf_statfs
#endif

#ifdef __NR_osf_statfs64
#define SYS_osf_statfs64 __NR_osf_statfs64
#endif

#ifdef __NR_osf_subsys_info
#define SYS_osf_subsys_info __NR_osf_subsys_info
#endif

#ifdef __NR_osf_swapctl
#define SYS_osf_swapctl __NR_osf_swapctl
#endif

#ifdef __NR_osf_swapon
#define SYS_osf_swapon __NR_osf_swapon
#endif

#ifdef __NR_osf_syscall
#define SYS_osf_syscall __NR_osf_syscall
#endif

#ifdef __NR_osf_sysinfo
#define SYS_osf_sysinfo __NR_osf_sysinfo
#endif

#ifdef __NR_osf_table
#define SYS_osf_table __NR_osf_table
#endif

#ifdef __NR_osf_uadmin
#define SYS_osf_uadmin __NR_osf_uadmin
#endif

#ifdef __NR_osf_usleep_thread
#define SYS_osf_usleep_thread __NR_osf_usleep_thread
#endif

#ifdef __NR_osf_uswitch
#define SYS_osf_uswitch __NR_osf_uswitch
#endif

#ifdef __NR_osf_utc_adjtime
#define SYS_osf_utc_adjtime __NR_osf_utc_adjtime
#endif

#ifdef __NR_osf_utc_gettime
#define SYS_osf_utc_gettime __NR_osf_utc_gettime
#endif

#ifdef __NR_osf_utimes
#define SYS_osf_utimes __NR_osf_utimes
#endif

#ifdef __NR_osf_utsname
#define SYS_osf_utsname __NR_osf_utsname
#endif

#ifdef __NR_osf_wait4
#define SYS_osf_wait4 __NR_osf_wait4
#endif

#ifdef __NR_osf_waitid
#define SYS_osf_waitid __NR_osf_waitid
#endif

#ifdef __NR_pause
#define SYS_pause __NR_pause
#endif

#ifdef __NR_pciconfig_iobase
#define SYS_pciconfig_iobase __NR_pciconfig_iobase
#endif

#ifdef __NR_pciconfig_read
#define SYS_pciconfig_read __NR_pciconfig_read
#endif

#ifdef __NR_pciconfig_write
#define SYS_pciconfig_write __NR_pciconfig_write
#endif

#ifdef __NR_perf_event_open
#define SYS_perf_event_open __NR_perf_event_open
#endif

#ifdef __NR_perfctr
#define SYS_perfctr __NR_perfctr
#endif

#ifdef __NR_perfmonctl
#define SYS_perfmonctl __NR_perfmonctl
#endif

#ifdef __NR_personality
#define SYS_personality __NR_personality
#endif

#ifdef __NR_pidfd_getfd
#define SYS_pidfd_getfd __NR_pidfd_getfd
#endif

#ifdef __NR_pidfd_open
#define SYS_pidfd_open __NR_pidfd_open
#endif

#ifdef __NR_pidfd_send_signal
#define SYS_pidfd_send_signal __NR_pidfd_send_signal
#endif

#ifdef __NR_pipe
#define SYS_pipe __NR_pipe
#endif

#ifdef __NR_pipe2
#define SYS_pipe2 __NR_pipe2
#endif

#ifdef __NR_pivot_root
#define SYS_pivot_root __NR_pivot_root
#endif

#ifdef __NR_pkey_alloc
#define SYS_pkey_alloc __NR_pkey_alloc
#endif

#ifdef __NR_pkey_free
#define SYS_pkey_free __NR_pkey_free
#endif

#ifdef __NR_pkey_mprotect
#define SYS_pkey_mprotect __NR_pkey_mprotect
#endif

#ifdef __NR_poll
#define SYS_poll __NR_poll
#endif

#ifdef __NR_ppoll
#define SYS_ppoll __NR_ppoll
#endif

#ifdef __NR_ppoll_time64
#define SYS_ppoll_time64 __NR_ppoll_time64
#endif

#ifdef __NR_prctl
#define SYS_prctl __NR_prctl
#endif

#ifdef __NR_pread64
#define SYS_pread64 __NR_pread64
#endif

#ifdef __NR_preadv
#define SYS_preadv __NR_preadv
#endif

#ifdef __NR_preadv2
#define SYS_preadv2 __NR_preadv2
#endif

#ifdef __NR_prlimit64
#define SYS_prlimit64 __NR_prlimit64
#endif

#ifdef __NR_process_madvise
#define SYS_process_madvise __NR_process_madvise
#endif

#ifdef __NR_process_mrelease
#define SYS_process_mrelease __NR_process_mrelease
#endif

#ifdef __NR_process_vm_readv
#define SYS_process_vm_readv __NR_process_vm_readv
#endif

#ifdef __NR_process_vm_writev
#define SYS_process_vm_writev __NR_process_vm_writev
#endif

#ifdef __NR_prof
#define SYS_prof __NR_prof
#endif

#ifdef __NR_profil
#define SYS_profil __NR_profil
#endif

#ifdef __NR_pselect6
#define SYS_pselect6 __NR_pselect6
#endif

#ifdef __NR_pselect6_time64
#define SYS_pselect6_time64 __NR_pselect6_time64
#endif

#ifdef __NR_ptrace
#define SYS_ptrace __NR_ptrace
#endif

#ifdef __NR_putpmsg
#define SYS_putpmsg __NR_putpmsg
#endif

#ifdef __NR_pwrite64
#define SYS_pwrite64 __NR_pwrite64
#endif

#ifdef __NR_pwritev
#define SYS_pwritev __NR_pwritev
#endif

#ifdef __NR_pwritev2
#define SYS_pwritev2 __NR_pwritev2
#endif

#ifdef __NR_query_module
#define SYS_query_module __NR_query_module
#endif

#ifdef __NR_quotactl
#define SYS_quotactl __NR_quotactl
#endif

#ifdef __NR_quotactl_fd
#define SYS_quotactl_fd __NR_quotactl_fd
#endif

#ifdef __NR_read
#define SYS_read __NR_read
#endif

#ifdef __NR_readahead
#define SYS_readahead __NR_readahead
#endif

#ifdef __NR_readdir
#define SYS_readdir __NR_readdir
#endif

#ifdef __NR_readlink
#define SYS_readlink __NR_readlink
#endif

#ifdef __NR_readlinkat
#define SYS_readlinkat __NR_readlinkat
#endif

#ifdef __NR_readv
#define SYS_readv __NR_readv
#endif

#ifdef __NR_reboot
#define SYS_reboot __NR_reboot
#endif

#ifdef __NR_recv
#define SYS_recv __NR_recv
#endif

#ifdef __NR_recvfrom
#define SYS_recvfrom __NR_recvfrom
#endif

#ifdef __NR_recvmmsg
#define SYS_recvmmsg __NR_recvmmsg
#endif

#ifdef __NR_recvmmsg_time64
#define SYS_recvmmsg_time64 __NR_recvmmsg_time64
#endif

#ifdef __NR_recvmsg
#define SYS_recvmsg __NR_recvmsg
#endif

#ifdef __NR_remap_file_pages
#define SYS_remap_file_pages __NR_remap_file_pages
#endif

#ifdef __NR_removexattr
#define SYS_removexattr __NR_removexattr
#endif

#ifdef __NR_rename
#define SYS_rename __NR_rename
#endif

#ifdef __NR_renameat
#define SYS_renameat __NR_renameat
#endif

#ifdef __NR_renameat2
#define SYS_renameat2 __NR_renameat2
#endif

#ifdef __NR_request_key
#define SYS_request_key __NR_request_key
#endif

#ifdef __NR_restart_syscall
#define SYS_restart_syscall __NR_restart_syscall
#endif

#ifdef __NR_riscv_flush_icache
#define SYS_riscv_flush_icache __NR_riscv_flush_icache
#endif

#ifdef __NR_riscv_hwprobe
#define SYS_riscv_hwprobe __NR_riscv_hwprobe
#endif

#ifdef __NR_rmdir
#define SYS_rmdir __NR_rmdir
#endif

#ifdef __NR_rseq
#define SYS_rseq __NR_rseq
#endif

#ifdef __NR_rt_sigaction
#define SYS_rt_sigaction __NR_rt_sigaction
#endif

#ifdef __NR_rt_sigpending
#define SYS_rt_sigpending __NR_rt_sigpending
#endif

#ifdef __NR_rt_sigprocmask
#define SYS_rt_sigprocmask __NR_rt_sigprocmask
#endif

#ifdef __NR_rt_sigqueueinfo
#define SYS_rt_sigqueueinfo __NR_rt_sigqueueinfo
#endif

#ifdef __NR_rt_sigreturn
#define SYS_rt_sigreturn __NR_rt_sigreturn
#endif

#ifdef __NR_rt_sigsuspend
#define SYS_rt_sigsuspend __NR_rt_sigsuspend
#endif

#ifdef __NR_rt_sigtimedwait
#define SYS_rt_sigtimedwait __NR_rt_sigtimedwait
#endif

#ifdef __NR_rt_sigtimedwait_time64
#define SYS_rt_sigtimedwait_time64 __NR_rt_sigtimedwait_time64
#endif

#ifdef __NR_rt_tgsigqueueinfo
#define SYS_rt_tgsigqueueinfo __NR_rt_tgsigqueueinfo
#endif

#ifdef __NR_rtas
#define SYS_rtas __NR_rtas
#endif

#ifdef __NR_s390_guarded_storage
#define SYS_s390_guarded_storage __NR_s390_guarded_storage
#endif

#ifdef __NR_s390_pci_mmio_read
#define SYS_s390_pci_mmio_read __NR_s390_pci_mmio_read
#endif

#ifdef __NR_s390_pci_mmio_write
#define SYS_s390_pci_mmio_write __NR_s390_pci_mmio_write
#endif

#ifdef __NR_s390_runtime_instr
#define SYS_s390_runtime_instr __NR_s390_runtime_instr
#endif

#ifdef __NR_s390_sthyi
#define SYS_s390_sthyi __NR_s390_sthyi
#endif

#ifdef __NR_sched_get_affinity
#define SYS_sched_get_affinity __NR_sched_get_affinity
#endif

#ifdef __NR_sched_get_priority_max
#define SYS_sched_get_priority_max __NR_sched_get_priority_max
#endif

#ifdef __NR_sched_get_priority_min
#define SYS_sched_get_priority_min __NR_sched_get_priority_min
#endif

#ifdef __NR_sched_getaffinity
#define SYS_sched_getaffinity __NR_sched_getaffinity
#endif

#ifdef __NR_sched_getattr
#define SYS_sched_getattr __NR_sched_getattr
#endif

#ifdef __NR_sched_getparam
#define SYS_sched_getparam __NR_sched_getparam
#endif

#ifdef __NR_sched_getscheduler
#define SYS_sched_getscheduler __NR_sched_getscheduler
#endif

#ifdef __NR_sched_rr_get_interval
#define SYS_sched_rr_get_interval __NR_sched_rr_get_interval
#endif

#ifdef __NR_sched_rr_get_interval_time64
#define SYS_sched_rr_get_interval_time64 __NR_sched_rr_get_interval_time64
#endif

#ifdef __NR_sched_set_affinity
#define SYS_sched_set_affinity __NR_sched_set_affinity
#endif

#ifdef __NR_sched_setaffinity
#define SYS_sched_setaffinity __NR_sched_setaffinity
#endif

#ifdef __NR_sched_setattr
#define SYS_sched_setattr __NR_sched_setattr
#endif

#ifdef __NR_sched_setparam
#define SYS_sched_setparam __NR_sched_setparam
#endif

#ifdef __NR_sched_setscheduler
#define SYS_sched_setscheduler __NR_sched_setscheduler
#endif

#ifdef __NR_sched_yield
#define SYS_sched_yield __NR_sched_yield
#endif

#ifdef __NR_seccomp
#define SYS_seccomp __NR_seccomp
#endif

#ifdef __NR_security
#define SYS_security __NR_security
#endif

#ifdef __NR_select
#define SYS_select __NR_select
#endif

#ifdef __NR_semctl
#define SYS_semctl __NR_semctl
#endif

#ifdef __NR_semget
#define SYS_semget __NR_semget
#endif

#ifdef __NR_semop
#define SYS_semop __NR_semop
#endif

#ifdef __NR_semtimedop
#define SYS_semtimedop __NR_semtimedop
#endif

#ifdef __NR_semtimedop_time64
#define SYS_semtimedop_time64 __NR_semtimedop_time64
#endif

#ifdef __NR_send
#define SYS_send __NR_send
#endif

#ifdef __NR_sendfile
#define SYS_sendfile __NR_sendfile
#endif

#ifdef __NR_sendfile64
#define SYS_sendfile64 __NR_sendfile64
#endif

#ifdef __NR_sendmmsg
#define SYS_sendmmsg __NR_sendmmsg
#endif

#ifdef __NR_sendmsg
#define SYS_sendmsg __NR_sendmsg
#endif

#ifdef __NR_sendto
#define SYS_sendto __NR_sendto
#endif

#ifdef __NR_set_mempolicy
#define SYS_set_mempolicy __NR_set_mempolicy
#endif

#ifdef __NR_set_mempolicy_home_node
#define SYS_set_mempolicy_home_node __NR_set_mempolicy_home_node
#endif

#ifdef __NR_set_robust_list
#define SYS_set_robust_list __NR_set_robust_list
#endif

#ifdef __NR_set_thread_area
#define SYS_set_thread_area __NR_set_thread_area
#endif

#ifdef __NR_set_tid_address
#define SYS_set_tid_address __NR_set_tid_address
#endif

#ifdef __NR_set_tls
#define SYS_set_tls __NR_set_tls
#endif

#ifdef __NR_setdomainname
#define SYS_setdomainname __NR_setdomainname
#endif

#ifdef __NR_setfsgid
#define SYS_setfsgid __NR_setfsgid
#endif

#ifdef __NR_setfsgid32
#define SYS_setfsgid32 __NR_setfsgid32
#endif

#ifdef __NR_setfsuid
#define SYS_setfsuid __NR_setfsuid
#endif

#ifdef __NR_setfsuid32
#define SYS_setfsuid32 __NR_setfsuid32
#endif

#ifdef __NR_setgid
#define SYS_setgid __NR_setgid
#endif

#ifdef __NR_setgid32
#define SYS_setgid32 __NR_setgid32
#endif

#ifdef __NR_setgroups
#define SYS_setgroups __NR_setgroups
#endif

#ifdef __NR_setgroups32
#define SYS_setgroups32 __NR_setgroups32
#endif

#ifdef __NR_sethae
#define SYS_sethae __NR_sethae
#endif

#ifdef __NR_sethostname
#define SYS_sethostname __NR_sethostname
#endif

#ifdef __NR_setitimer
#define SYS_setitimer __NR_setitimer
#endif

#ifdef __NR_setns
#define SYS_setns __NR_setns
#endif

#ifdef __NR_setpgid
#define SYS_setpgid __NR_setpgid
#endif

#ifdef __NR_setpgrp
#define SYS_setpgrp __NR_setpgrp
#endif

#ifdef __NR_setpriority
#define SYS_setpriority __NR_setpriority
#endif

#ifdef __NR_setregid
#define SYS_setregid __NR_setregid
#endif

#ifdef __NR_setregid32
#define SYS_setregid32 __NR_setregid32
#endif

#ifdef __NR_setresgid
#define SYS_setresgid __NR_setresgid
#endif

#ifdef __NR_setresgid32
#define SYS_setresgid32 __NR_setresgid32
#endif

#ifdef __NR_setresuid
#define SYS_setresuid __NR_setresuid
#endif

#ifdef __NR_setresuid32
#define SYS_setresuid32 __NR_setresuid32
#endif

#ifdef __NR_setreuid
#define SYS_setreuid __NR_setreuid
#endif

#ifdef __NR_setreuid32
#define SYS_setreuid32 __NR_setreuid32
#endif

#ifdef __NR_setrlimit
#define SYS_setrlimit __NR_setrlimit
#endif

#ifdef __NR_setsid
#define SYS_setsid __NR_setsid
#endif

#ifdef __NR_setsockopt
#define SYS_setsockopt __NR_setsockopt
#endif

#ifdef __NR_settimeofday
#define SYS_settimeofday __NR_settimeofday
#endif

#ifdef __NR_setuid
#define SYS_setuid __NR_setuid
#endif

#ifdef __NR_setuid32
#define SYS_setuid32 __NR_setuid32
#endif

#ifdef __NR_setxattr
#define SYS_setxattr __NR_setxattr
#endif

#ifdef __NR_sgetmask
#define SYS_sgetmask __NR_sgetmask
#endif

#ifdef __NR_shmat
#define SYS_shmat __NR_shmat
#endif

#ifdef __NR_shmctl
#define SYS_shmctl __NR_shmctl
#endif

#ifdef __NR_shmdt
#define SYS_shmdt __NR_shmdt
#endif

#ifdef __NR_shmget
#define SYS_shmget __NR_shmget
#endif

#ifdef __NR_shutdown
#define SYS_shutdown __NR_shutdown
#endif

#ifdef __NR_sigaction
#define SYS_sigaction __NR_sigaction
#endif

#ifdef __NR_sigaltstack
#define SYS_sigaltstack __NR_sigaltstack
#endif

#ifdef __NR_signal
#define SYS_signal __NR_signal
#endif

#ifdef __NR_signalfd
#define SYS_signalfd __NR_signalfd
#endif

#ifdef __NR_signalfd4
#define SYS_signalfd4 __NR_signalfd4
#endif

#ifdef __NR_sigpending
#define SYS_sigpending __NR_sigpending
#endif

#ifdef __NR_sigprocmask
#define SYS_sigprocmask __NR_sigprocmask
#endif

#ifdef __NR_sigreturn
#define SYS_sigreturn __NR_sigreturn
#endif

#ifdef __NR_sigsuspend
#define SYS_sigsuspend __NR_sigsuspend
#endif

#ifdef __NR_socket
#define SYS_socket __NR_socket
#endif

#ifdef __NR_socketcall
#define SYS_socketcall __NR_socketcall
#endif

#ifdef __NR_socketpair
#define SYS_socketpair __NR_socketpair
#endif

#ifdef __NR_splice
#define SYS_splice __NR_splice
#endif

#ifdef __NR_spu_create
#define SYS_spu_create __NR_spu_create
#endif

#ifdef __NR_spu_run
#define SYS_spu_run __NR_spu_run
#endif

#ifdef __NR_ssetmask
#define SYS_ssetmask __NR_ssetmask
#endif

#ifdef __NR_stat
#define SYS_stat __NR_stat
#endif

#ifdef __NR_stat64
#define SYS_stat64 __NR_stat64
#endif

#ifdef __NR_statfs
#define SYS_statfs __NR_statfs
#endif

#ifdef __NR_statfs64
#define SYS_statfs64 __NR_statfs64
#endif

#ifdef __NR_statx
#define SYS_statx __NR_statx
#endif

#ifdef __NR_stime
#define SYS_stime __NR_stime
#endif

#ifdef __NR_stty
#define SYS_stty __NR_stty
#endif

#ifdef __NR_subpage_prot
#define SYS_subpage_prot __NR_subpage_prot
#endif

#ifdef __NR_swapcontext
#define SYS_swapcontext __NR_swapcontext
#endif

#ifdef __NR_swapoff
#define SYS_swapoff __NR_swapoff
#endif

#ifdef __NR_swapon
#define SYS_swapon __NR_swapon
#endif

#ifdef __NR_switch_endian
#define SYS_switch_endian __NR_switch_endian
#endif

#ifdef __NR_symlink
#define SYS_symlink __NR_symlink
#endif

#ifdef __NR_symlinkat
#define SYS_symlinkat __NR_symlinkat
#endif

#ifdef __NR_sync
#define SYS_sync __NR_sync
#endif

#ifdef __NR_sync_file_range
#define SYS_sync_file_range __NR_sync_file_range
#endif

#ifdef __NR_sync_file_range2
#define SYS_sync_file_range2 __NR_sync_file_range2
#endif

#ifdef __NR_syncfs
#define SYS_syncfs __NR_syncfs
#endif

#ifdef __NR_sys_debug_setcontext
#define SYS_sys_debug_setcontext __NR_sys_debug_setcontext
#endif

#ifdef __NR_sys_epoll_create
#define SYS_sys_epoll_create __NR_sys_epoll_create
#endif

#ifdef __NR_sys_epoll_ctl
#define SYS_sys_epoll_ctl __NR_sys_epoll_ctl
#endif

#ifdef __NR_sys_epoll_wait
#define SYS_sys_epoll_wait __NR_sys_epoll_wait
#endif

#ifdef __NR_syscall
#define SYS_syscall __NR_syscall
#endif

#ifdef __NR_sysfs
#define SYS_sysfs __NR_sysfs
#endif

#ifdef __NR_sysinfo
#define SYS_sysinfo __NR_sysinfo
#endif

#ifdef __NR_syslog
#define SYS_syslog __NR_syslog
#endif

#ifdef __NR_sysmips
#define SYS_sysmips __NR_sysmips
#endif

#ifdef __NR_tee
#define SYS_tee __NR_tee
#endif

#ifdef __NR_tgkill
#define SYS_tgkill __NR_tgkill
#endif

#ifdef __NR_time
#define SYS_time __NR_time
#endif

#ifdef __NR_timer_create
#define SYS_timer_create __NR_timer_create
#endif

#ifdef __NR_timer_delete
#define SYS_timer_delete __NR_timer_delete
#endif

#ifdef __NR_timer_getoverrun
#define SYS_timer_getoverrun __NR_timer_getoverrun
#endif

#ifdef __NR_timer_gettime
#define SYS_timer_gettime __NR_timer_gettime
#endif

#ifdef __NR_timer_gettime64
#define SYS_timer_gettime64 __NR_timer_gettime64
#endif

#ifdef __NR_timer_settime
#define SYS_timer_settime __NR_timer_settime
#endif

#ifdef __NR_timer_settime64
#define SYS_timer_settime64 __NR_timer_settime64
#endif

#ifdef __NR_timerfd
#define SYS_timerfd __NR_timerfd
#endif

#ifdef __NR_timerfd_create
#define SYS_timerfd_create __NR_timerfd_create
#endif

#ifdef __NR_timerfd_gettime
#define SYS_timerfd_gettime __NR_timerfd_gettime
#endif

#ifdef __NR_timerfd_gettime64
#define SYS_timerfd_gettime64 __NR_timerfd_gettime64
#endif

#ifdef __NR_timerfd_settime
#define SYS_timerfd_settime __NR_timerfd_settime
#endif

#ifdef __NR_timerfd_settime64
#define SYS_timerfd_settime64 __NR_timerfd_settime64
#endif

#ifdef __NR_times
#define SYS_times __NR_times
#endif

#ifdef __NR_tkill
#define SYS_tkill __NR_tkill
#endif

#ifdef __NR_truncate
#define SYS_truncate __NR_truncate
#endif

#ifdef __NR_truncate64
#define SYS_truncate64 __NR_truncate64
#endif

#ifdef __NR_tuxcall
#define SYS_tuxcall __NR_tuxcall
#endif

#ifdef __NR_udftrap
#define SYS_udftrap __NR_udftrap
#endif

#ifdef __NR_ugetrlimit
#define SYS_ugetrlimit __NR_ugetrlimit
#endif

#ifdef __NR_ulimit
#define SYS_ulimit __NR_ulimit
#endif

#ifdef __NR_umask
#define SYS_umask __NR_umask
#endif

#ifdef __NR_umount
#define SYS_umount __NR_umount
#endif

#ifdef __NR_umount2
#define SYS_umount2 __NR_umount2
#endif

#ifdef __NR_uname
#define SYS_uname __NR_uname
#endif

#ifdef __NR_unlink
#define SYS_unlink __NR_unlink
#endif

#ifdef __NR_unlinkat
#define SYS_unlinkat __NR_unlinkat
#endif

#ifdef __NR_unshare
#define SYS_unshare __NR_unshare
#endif

#ifdef __NR_uselib
#define SYS_uselib __NR_uselib
#endif

#ifdef __NR_userfaultfd
#define SYS_userfaultfd __NR_userfaultfd
#endif

#ifdef __NR_usr26
#define SYS_usr26 __NR_usr26
#endif

#ifdef __NR_usr32
#define SYS_usr32 __NR_usr32
#endif

#ifdef __NR_ustat
#define SYS_ustat __NR_ustat
#endif

#ifdef __NR_utime
#define SYS_utime __NR_utime
#endif

#ifdef __NR_utimensat
#define SYS_utimensat __NR_utimensat
#endif

#ifdef __NR_utimensat_time64
#define SYS_utimensat_time64 __NR_utimensat_time64
#endif

#ifdef __NR_utimes
#define SYS_utimes __NR_utimes
#endif

#ifdef __NR_utrap_install
#define SYS_utrap_install __NR_utrap_install
#endif

#ifdef __NR_vfork
#define SYS_vfork __NR_vfork
#endif

#ifdef __NR_vhangup
#define SYS_vhangup __NR_vhangup
#endif

#ifdef __NR_vm86
#define SYS_vm86 __NR_vm86
#endif

#ifdef __NR_vm86old
#define SYS_vm86old __NR_vm86old
#endif

#ifdef __NR_vmsplice
#define SYS_vmsplice __NR_vmsplice
#endif

#ifdef __NR_vserver
#define SYS_vserver __NR_vserver
#endif

#ifdef __NR_wait4
#define SYS_wait4 __NR_wait4
#endif

#ifdef __NR_waitid
#define SYS_waitid __NR_waitid
#endif

#ifdef __NR_waitpid
#define SYS_waitpid __NR_waitpid
#endif

#ifdef __NR_write
#define SYS_write __NR_write
#endif

#ifdef __NR_writev
#define SYS_writev __NR_writev
#endif


