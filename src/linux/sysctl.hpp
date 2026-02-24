//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file license_1_0.txt or copy at
//  http://www.boost.org/license_1_0.txt
#pragma once

#include "../types.hpp"

// NOTE: this provides for a high level utility to sysctl(), it does not invoke the syscall (deprecated since lx5.5), rather parses
// /proc/sys to acquire similar data

namespace micron
{
constexpr static const u32 __sysctl_ctl_kern = 1;
constexpr static const u32 __sysctl_ctl_vm = 2;
constexpr static const u32 __sysctl_ctl_net = 3;
constexpr static const u32 __sysctl_ctl_proc = 4;
constexpr static const u32 __sysctl_ctl_fs = 5;
constexpr static const u32 __sysctl_ctl_debug = 6;
constexpr static const u32 __sysctl_ctl_dev = 7;
constexpr static const u32 __sysctl_ctl_bus = 8;
constexpr static const u32 __sysctl_ctl_abi = 9;
constexpr static const u32 __sysctl_ctl_cpu = 10;
constexpr static const u32 __sysctl_ctl_arlan = 254;
constexpr static const u32 __sysctl_ctl_s390dbf = 5677;
constexpr static const u32 __sysctl_ctl_sunrpc = 7249;
constexpr static const u32 __sysctl_ctl_pm = 9899;
constexpr static const u32 __sysctl_ctl_frv = 9898;

constexpr static const u32 __sysctl_ctl_bus_isa = 1;

constexpr static const u32 __sysctl_inotify_max_user_instances = 1;
constexpr static const u32 __sysctl_inotify_max_user_watches = 2;
constexpr static const u32 __sysctl_inotify_max_queued_events = 3;

constexpr static const u32 __sysctl_kern_ostype = 1;
constexpr static const u32 __sysctl_kern_osrelease = 2;
constexpr static const u32 __sysctl_kern_osrev = 3;
constexpr static const u32 __sysctl_kern_version = 4;
constexpr static const u32 __sysctl_kern_securemask = 5;
constexpr static const u32 __sysctl_kern_prof = 6;
constexpr static const u32 __sysctl_kern_nodename = 7;
constexpr static const u32 __sysctl_kern_domainname = 8;
constexpr static const u32 __sysctl_kern_panic = 15;
constexpr static const u32 __sysctl_kern_realrootdev = 16;
constexpr static const u32 __sysctl_kern_sparc_reboot = 21;
constexpr static const u32 __sysctl_kern_ctlaltdel = 22;
constexpr static const u32 __sysctl_kern_printk = 23;
constexpr static const u32 __sysctl_kern_nametrans = 24;
constexpr static const u32 __sysctl_kern_ppc_htabreclaim = 25;
constexpr static const u32 __sysctl_kern_ppc_zeropaged = 26;
constexpr static const u32 __sysctl_kern_ppc_powersave_nap = 27;
constexpr static const u32 __sysctl_kern_modprobe = 28;
constexpr static const u32 __sysctl_kern_sg_big_buff = 29;
constexpr static const u32 __sysctl_kern_acct = 30;
constexpr static const u32 __sysctl_kern_ppc_l2cr = 31;
constexpr static const u32 __sysctl_kern_rtsignr = 32;
constexpr static const u32 __sysctl_kern_rtsigmax = 33;
constexpr static const u32 __sysctl_kern_shmmax = 34;
constexpr static const u32 __sysctl_kern_msgmax = 35;
constexpr static const u32 __sysctl_kern_msgmnb = 36;
constexpr static const u32 __sysctl_kern_msgpool = 37;
constexpr static const u32 __sysctl_kern_sysrq = 38;
constexpr static const u32 __sysctl_kern_max_threads = 39;
constexpr static const u32 __sysctl_kern_random = 40;
constexpr static const u32 __sysctl_kern_shmall = 41;
constexpr static const u32 __sysctl_kern_msgmni = 42;
constexpr static const u32 __sysctl_kern_sem = 43;
constexpr static const u32 __sysctl_kern_sparc_stop_a = 44;
constexpr static const u32 __sysctl_kern_shmmni = 45;
constexpr static const u32 __sysctl_kern_overflowuid = 46;
constexpr static const u32 __sysctl_kern_overflowgid = 47;
constexpr static const u32 __sysctl_kern_shmpath = 48;
constexpr static const u32 __sysctl_kern_hotplug = 49;
constexpr static const u32 __sysctl_kern_ieee_emulation_warnings = 50;
constexpr static const u32 __sysctl_kern_s390_user_debug_logging = 51;
constexpr static const u32 __sysctl_kern_core_uses_pid = 52;
constexpr static const u32 __sysctl_kern_tainted = 53;
constexpr static const u32 __sysctl_kern_cadpid = 54;
constexpr static const u32 __sysctl_kern_pidmax = 55;
constexpr static const u32 __sysctl_kern_core_pattern = 56;
constexpr static const u32 __sysctl_kern_panic_on_oops = 57;
constexpr static const u32 __sysctl_kern_hppa_pwrsw = 58;
constexpr static const u32 __sysctl_kern_hppa_unaligned = 59;
constexpr static const u32 __sysctl_kern_printk_ratelimit = 60;
constexpr static const u32 __sysctl_kern_printk_ratelimit_burst = 61;
constexpr static const u32 __sysctl_kern_pty = 62;
constexpr static const u32 __sysctl_kern_ngroups_max = 63;
constexpr static const u32 __sysctl_kern_sparc_scons_pwroff = 64;
constexpr static const u32 __sysctl_kern_hz_timer = 65;
constexpr static const u32 __sysctl_kern_unknown_nmi_panic = 66;
constexpr static const u32 __sysctl_kern_bootloader_type = 67;
constexpr static const u32 __sysctl_kern_randomize = 68;
constexpr static const u32 __sysctl_kern_setuid_dumpable = 69;
constexpr static const u32 __sysctl_kern_spin_retry = 70;
constexpr static const u32 __sysctl_kern_acpi_video_flags = 71;
constexpr static const u32 __sysctl_kern_ia64_unaligned = 72;
constexpr static const u32 __sysctl_kern_compat_log = 73;
constexpr static const u32 __sysctl_kern_max_lock_depth = 74;
constexpr static const u32 __sysctl_kern_nmi_watchdog = 75;
constexpr static const u32 __sysctl_kern_panic_on_nmi = 76;
constexpr static const u32 __sysctl_kern_panic_on_warn = 77;
constexpr static const u32 __sysctl_kern_panic_print = 78;

constexpr static const u32 __sysctl_vm_unused1 = 1;
constexpr static const u32 __sysctl_vm_unused2 = 2;
constexpr static const u32 __sysctl_vm_unused3 = 3;
constexpr static const u32 __sysctl_vm_unused4 = 4;
constexpr static const u32 __sysctl_vm_overcommit_memory = 5;
constexpr static const u32 __sysctl_vm_unused5 = 6;
constexpr static const u32 __sysctl_vm_unused7 = 7;
constexpr static const u32 __sysctl_vm_unused8 = 8;
constexpr static const u32 __sysctl_vm_unused9 = 9;
constexpr static const u32 __sysctl_vm_page_cluster = 10;
constexpr static const u32 __sysctl_vm_dirty_background = 11;
constexpr static const u32 __sysctl_vm_dirty_ratio = 12;
constexpr static const u32 __sysctl_vm_dirty_wb_cs = 13;
constexpr static const u32 __sysctl_vm_dirty_expire_cs = 14;
constexpr static const u32 __sysctl_vm_nr_pdflush_threads = 15;
constexpr static const u32 __sysctl_vm_overcommit_ratio = 16;
constexpr static const u32 __sysctl_vm_pagebuf = 17;
constexpr static const u32 __sysctl_vm_hugetlb_pages = 18;
constexpr static const u32 __sysctl_vm_swappiness = 19;
constexpr static const u32 __sysctl_vm_lowmem_reserve_ratio = 20;
constexpr static const u32 __sysctl_vm_min_free_kbytes = 21;
constexpr static const u32 __sysctl_vm_max_map_count = 22;
constexpr static const u32 __sysctl_vm_laptop_mode = 23;
constexpr static const u32 __sysctl_vm_block_dump = 24;
constexpr static const u32 __sysctl_vm_hugetlb_group = 25;
constexpr static const u32 __sysctl_vm_vfs_cache_pressure = 26;
constexpr static const u32 __sysctl_vm_legacy_va_layout = 27;
constexpr static const u32 __sysctl_vm_swap_token_timeout = 28;
constexpr static const u32 __sysctl_vm_drop_pagecache = 29;
constexpr static const u32 __sysctl_vm_percpu_pagelist_fraction = 30;
constexpr static const u32 __sysctl_vm_zone_reclaim_mode = 31;
constexpr static const u32 __sysctl_vm_min_unmapped = 32;
constexpr static const u32 __sysctl_vm_panic_on_oom = 33;
constexpr static const u32 __sysctl_vm_vdso_enabled = 34;
constexpr static const u32 __sysctl_vm_min_slab = 35;

constexpr static const u32 __sysctl_net_core = 1;
constexpr static const u32 __sysctl_net_ether = 2;
constexpr static const u32 __sysctl_net_802 = 3;
constexpr static const u32 __sysctl_net_unix = 4;
constexpr static const u32 __sysctl_net_ipv4 = 5;
constexpr static const u32 __sysctl_net_ipx = 6;
constexpr static const u32 __sysctl_net_atalk = 7;
constexpr static const u32 __sysctl_net_netrom = 8;
constexpr static const u32 __sysctl_net_ax25 = 9;
constexpr static const u32 __sysctl_net_bridge = 10;
constexpr static const u32 __sysctl_net_rose = 11;
constexpr static const u32 __sysctl_net_ipv6 = 12;
constexpr static const u32 __sysctl_net_x25 = 13;
constexpr static const u32 __sysctl_net_tr = 14;
constexpr static const u32 __sysctl_net_decnet = 15;
constexpr static const u32 __sysctl_net_econet = 16;
constexpr static const u32 __sysctl_net_sctp = 17;
constexpr static const u32 __sysctl_net_llc = 18;
constexpr static const u32 __sysctl_net_netfilter = 19;
constexpr static const u32 __sysctl_net_dccp = 20;
constexpr static const u32 __sysctl_net_irda = 412;

constexpr static const u32 __sysctl_random_poolsize = 1;
constexpr static const u32 __sysctl_random_entropy_count = 2;
constexpr static const u32 __sysctl_random_read_thresh = 3;
constexpr static const u32 __sysctl_random_write_thresh = 4;
constexpr static const u32 __sysctl_random_boot_id = 5;
constexpr static const u32 __sysctl_random_uuid = 6;

constexpr static const u32 __sysctl_pty_max = 1;
constexpr static const u32 __sysctl_pty_nr = 2;

constexpr static const u32 __sysctl_bus_isa_mem_base = 1;
constexpr static const u32 __sysctl_bus_isa_port_base = 2;
constexpr static const u32 __sysctl_bus_isa_port_shift = 3;

constexpr static const u32 __sysctl_net_core_wmem_max = 1;
constexpr static const u32 __sysctl_net_core_rmem_max = 2;
constexpr static const u32 __sysctl_net_core_wmem_default = 3;
constexpr static const u32 __sysctl_net_core_rmem_default = 4;
constexpr static const u32 __sysctl_net_core_max_backlog = 6;
constexpr static const u32 __sysctl_net_core_fastroute = 7;
constexpr static const u32 __sysctl_net_core_msg_cost = 8;
constexpr static const u32 __sysctl_net_core_msg_burst = 9;
constexpr static const u32 __sysctl_net_core_optmem_max = 10;
constexpr static const u32 __sysctl_net_core_hot_list_length = 11;
constexpr static const u32 __sysctl_net_core_divert_version = 12;
constexpr static const u32 __sysctl_net_core_no_cong_thresh = 13;
constexpr static const u32 __sysctl_net_core_no_cong = 14;
constexpr static const u32 __sysctl_net_core_lo_cong = 15;
constexpr static const u32 __sysctl_net_core_mod_cong = 16;
constexpr static const u32 __sysctl_net_core_dev_weight = 17;
constexpr static const u32 __sysctl_net_core_somaxconn = 18;
constexpr static const u32 __sysctl_net_core_budget = 19;
constexpr static const u32 __sysctl_net_core_aevent_etime = 20;
constexpr static const u32 __sysctl_net_core_aevent_rseqth = 21;
constexpr static const u32 __sysctl_net_core_warnings = 22;

constexpr static const u32 __sysctl_net_unix_destroy_delay = 1;
constexpr static const u32 __sysctl_net_unix_delete_delay = 2;
constexpr static const u32 __sysctl_net_unix_max_dgram_qlen = 3;

constexpr static const u32 __sysctl_net_nf_conntrack_max = 1;
constexpr static const u32 __sysctl_net_nf_conntrack_tcp_timeout_syn_sent = 2;
constexpr static const u32 __sysctl_net_nf_conntrack_tcp_timeout_syn_recv = 3;
constexpr static const u32 __sysctl_net_nf_conntrack_tcp_timeout_established = 4;
constexpr static const u32 __sysctl_net_nf_conntrack_tcp_timeout_fin_wait = 5;
constexpr static const u32 __sysctl_net_nf_conntrack_tcp_timeout_close_wait = 6;
constexpr static const u32 __sysctl_net_nf_conntrack_tcp_timeout_last_ack = 7;
constexpr static const u32 __sysctl_net_nf_conntrack_tcp_timeout_time_wait = 8;
constexpr static const u32 __sysctl_net_nf_conntrack_tcp_timeout_close = 9;
constexpr static const u32 __sysctl_net_nf_conntrack_udp_timeout = 10;
constexpr static const u32 __sysctl_net_nf_conntrack_udp_timeout_stream = 11;
constexpr static const u32 __sysctl_net_nf_conntrack_icmp_timeout = 12;
constexpr static const u32 __sysctl_net_nf_conntrack_generic_timeout = 13;
constexpr static const u32 __sysctl_net_nf_conntrack_buckets = 14;
constexpr static const u32 __sysctl_net_nf_conntrack_log_invalid = 15;
constexpr static const u32 __sysctl_net_nf_conntrack_tcp_timeout_max_retrans = 16;
constexpr static const u32 __sysctl_net_nf_conntrack_tcp_loose = 17;
constexpr static const u32 __sysctl_net_nf_conntrack_tcp_be_liberal = 18;
constexpr static const u32 __sysctl_net_nf_conntrack_tcp_max_retrans = 19;
constexpr static const u32 __sysctl_net_nf_conntrack_sctp_timeout_closed = 20;
constexpr static const u32 __sysctl_net_nf_conntrack_sctp_timeout_cookie_wait = 21;
constexpr static const u32 __sysctl_net_nf_conntrack_sctp_timeout_cookie_echoed = 22;
constexpr static const u32 __sysctl_net_nf_conntrack_sctp_timeout_established = 23;
constexpr static const u32 __sysctl_net_nf_conntrack_sctp_timeout_shutdown_sent = 24;
constexpr static const u32 __sysctl_net_nf_conntrack_sctp_timeout_shutdown_recd = 25;
constexpr static const u32 __sysctl_net_nf_conntrack_sctp_timeout_shutdown_ack_sent = 26;
constexpr static const u32 __sysctl_net_nf_conntrack_count = 27;
constexpr static const u32 __sysctl_net_nf_conntrack_icmpv6_timeout = 28;
constexpr static const u32 __sysctl_net_nf_conntrack_frag6_timeout = 29;
constexpr static const u32 __sysctl_net_nf_conntrack_frag6_low_thresh = 30;
constexpr static const u32 __sysctl_net_nf_conntrack_frag6_high_thresh = 31;
constexpr static const u32 __sysctl_net_nf_conntrack_checksum = 32;

constexpr static const u32 __sysctl_net_ipv4_forward = 8;
constexpr static const u32 __sysctl_net_ipv4_dynaddr = 9;
constexpr static const u32 __sysctl_net_ipv4_conf = 16;
constexpr static const u32 __sysctl_net_ipv4_neigh = 17;
constexpr static const u32 __sysctl_net_ipv4_route = 18;
constexpr static const u32 __sysctl_net_ipv4_fib_hash = 19;
constexpr static const u32 __sysctl_net_ipv4_netfilter = 20;
constexpr static const u32 __sysctl_net_ipv4_tcp_timestamps = 33;
constexpr static const u32 __sysctl_net_ipv4_tcp_window_scaling = 34;
constexpr static const u32 __sysctl_net_ipv4_tcp_sack = 35;
constexpr static const u32 __sysctl_net_ipv4_tcp_retrans_collapse = 36;
constexpr static const u32 __sysctl_net_ipv4_default_ttl = 37;
constexpr static const u32 __sysctl_net_ipv4_autoconfig = 38;
constexpr static const u32 __sysctl_net_ipv4_no_pmtu_disc = 39;
constexpr static const u32 __sysctl_net_ipv4_tcp_syn_retries = 40;
constexpr static const u32 __sysctl_net_ipv4_ipfrag_high_thresh = 41;
constexpr static const u32 __sysctl_net_ipv4_ipfrag_low_thresh = 42;
constexpr static const u32 __sysctl_net_ipv4_ipfrag_time = 43;
constexpr static const u32 __sysctl_net_ipv4_tcp_max_ka_probes = 44;
constexpr static const u32 __sysctl_net_ipv4_tcp_keepalive_time = 45;
constexpr static const u32 __sysctl_net_ipv4_tcp_keepalive_probes = 46;
constexpr static const u32 __sysctl_net_ipv4_tcp_retries1 = 47;
constexpr static const u32 __sysctl_net_ipv4_tcp_retries2 = 48;
constexpr static const u32 __sysctl_net_ipv4_tcp_fin_timeout = 49;
constexpr static const u32 __sysctl_net_ipv4_ip_masq_debug = 50;
constexpr static const u32 __sysctl_net_tcp_syncookies = 51;
constexpr static const u32 __sysctl_net_tcp_stdurg = 52;
constexpr static const u32 __sysctl_net_tcp_rfc1337 = 53;
constexpr static const u32 __sysctl_net_tcp_syn_taildrop = 54;
constexpr static const u32 __sysctl_net_tcp_max_syn_backlog = 55;
constexpr static const u32 __sysctl_net_ipv4_local_port_range = 56;
constexpr static const u32 __sysctl_net_ipv4_icmp_echo_ignore_all = 57;
constexpr static const u32 __sysctl_net_ipv4_icmp_echo_ignore_broadcasts = 58;
constexpr static const u32 __sysctl_net_ipv4_icmp_sourcequench_rate = 59;
constexpr static const u32 __sysctl_net_ipv4_icmp_destunreach_rate = 60;
constexpr static const u32 __sysctl_net_ipv4_icmp_timeexceed_rate = 61;
constexpr static const u32 __sysctl_net_ipv4_icmp_paramprob_rate = 62;
constexpr static const u32 __sysctl_net_ipv4_icmp_echoreply_rate = 63;
constexpr static const u32 __sysctl_net_ipv4_icmp_ignore_bogus_error_responses = 64;
constexpr static const u32 __sysctl_net_ipv4_igmp_max_memberships = 65;
constexpr static const u32 __sysctl_net_tcp_tw_recycle = 66;
constexpr static const u32 __sysctl_net_ipv4_always_defrag = 67;
constexpr static const u32 __sysctl_net_ipv4_tcp_keepalive_intvl = 68;
constexpr static const u32 __sysctl_net_ipv4_inet_peer_threshold = 69;
constexpr static const u32 __sysctl_net_ipv4_inet_peer_minttl = 70;
constexpr static const u32 __sysctl_net_ipv4_inet_peer_maxttl = 71;
constexpr static const u32 __sysctl_net_ipv4_inet_peer_gc_mintime = 72;
constexpr static const u32 __sysctl_net_ipv4_inet_peer_gc_maxtime = 73;
constexpr static const u32 __sysctl_net_tcp_orphan_retries = 74;
constexpr static const u32 __sysctl_net_tcp_abort_on_overflow = 75;
constexpr static const u32 __sysctl_net_tcp_synack_retries = 76;
constexpr static const u32 __sysctl_net_tcp_max_orphans = 77;
constexpr static const u32 __sysctl_net_tcp_max_tw_buckets = 78;
constexpr static const u32 __sysctl_net_tcp_fack = 79;
constexpr static const u32 __sysctl_net_tcp_reordering = 80;
constexpr static const u32 __sysctl_net_tcp_ecn = 81;
constexpr static const u32 __sysctl_net_tcp_dsack = 82;
constexpr static const u32 __sysctl_net_tcp_mem = 83;
constexpr static const u32 __sysctl_net_tcp_wmem = 84;
constexpr static const u32 __sysctl_net_tcp_rmem = 85;
constexpr static const u32 __sysctl_net_tcp_app_win = 86;
constexpr static const u32 __sysctl_net_tcp_adv_win_scale = 87;
constexpr static const u32 __sysctl_net_ipv4_nonlocal_bind = 88;
constexpr static const u32 __sysctl_net_ipv4_icmp_ratelimit = 89;
constexpr static const u32 __sysctl_net_ipv4_icmp_ratemask = 90;
constexpr static const u32 __sysctl_net_tcp_tw_reuse = 91;
constexpr static const u32 __sysctl_net_tcp_frto = 92;
constexpr static const u32 __sysctl_net_tcp_low_latency = 93;
constexpr static const u32 __sysctl_net_ipv4_ipfrag_secret_interval = 94;
constexpr static const u32 __sysctl_net_ipv4_igmp_max_msf = 96;
constexpr static const u32 __sysctl_net_tcp_no_metrics_save = 97;
constexpr static const u32 __sysctl_net_tcp_default_win_scale = 105;
constexpr static const u32 __sysctl_net_tcp_moderate_rcvbuf = 106;
constexpr static const u32 __sysctl_net_tcp_tso_win_divisor = 107;
constexpr static const u32 __sysctl_net_tcp_bic_beta = 108;
constexpr static const u32 __sysctl_net_ipv4_icmp_errors_use_inbound_ifaddr = 109;
constexpr static const u32 __sysctl_net_tcp_cong_control = 110;
constexpr static const u32 __sysctl_net_tcp_abc = 111;
constexpr static const u32 __sysctl_net_ipv4_ipfrag_max_dist = 112;
constexpr static const u32 __sysctl_net_tcp_mtu_probing = 113;
constexpr static const u32 __sysctl_net_tcp_base_mss = 114;
constexpr static const u32 __sysctl_net_ipv4_tcp_workaround_signed_windows = 115;
constexpr static const u32 __sysctl_net_tcp_dma_copybreak = 116;
constexpr static const u32 __sysctl_net_tcp_slow_start_after_idle = 117;
constexpr static const u32 __sysctl_net_cipsov4_cache_enable = 118;
constexpr static const u32 __sysctl_net_cipsov4_cache_bucket_size = 119;
constexpr static const u32 __sysctl_net_cipsov4_rbm_optfmt = 120;
constexpr static const u32 __sysctl_net_cipsov4_rbm_strictvalid = 121;
constexpr static const u32 __sysctl_net_tcp_avail_cong_control = 122;
constexpr static const u32 __sysctl_net_tcp_allowed_cong_control = 123;
constexpr static const u32 __sysctl_net_tcp_max_ssthresh = 124;
constexpr static const u32 __sysctl_net_tcp_frto_response = 125;

constexpr static const u32 __sysctl_net_ipv4_route_flush = 1;
constexpr static const u32 __sysctl_net_ipv4_route_min_delay = 2;
constexpr static const u32 __sysctl_net_ipv4_route_max_delay = 3;
constexpr static const u32 __sysctl_net_ipv4_route_gc_thresh = 4;
constexpr static const u32 __sysctl_net_ipv4_route_max_size = 5;
constexpr static const u32 __sysctl_net_ipv4_route_gc_min_interval = 6;
constexpr static const u32 __sysctl_net_ipv4_route_gc_timeout = 7;
constexpr static const u32 __sysctl_net_ipv4_route_gc_interval = 8;
constexpr static const u32 __sysctl_net_ipv4_route_redirect_load = 9;
constexpr static const u32 __sysctl_net_ipv4_route_redirect_number = 10;
constexpr static const u32 __sysctl_net_ipv4_route_redirect_silence = 11;
constexpr static const u32 __sysctl_net_ipv4_route_error_cost = 12;
constexpr static const u32 __sysctl_net_ipv4_route_error_burst = 13;
constexpr static const u32 __sysctl_net_ipv4_route_gc_elasticity = 14;
constexpr static const u32 __sysctl_net_ipv4_route_mtu_expires = 15;
constexpr static const u32 __sysctl_net_ipv4_route_min_pmtu = 16;
constexpr static const u32 __sysctl_net_ipv4_route_min_advmss = 17;
constexpr static const u32 __sysctl_net_ipv4_route_secret_interval = 18;
constexpr static const u32 __sysctl_net_ipv4_route_gc_min_interval_ms = 19;

constexpr static const u32 __sysctl_net_proto_conf_all = static_cast<u32>(-2);
constexpr static const u32 __sysctl_net_proto_conf_default = static_cast<u32>(-3);

constexpr static const u32 __sysctl_net_ipv4_conf_forwarding = 1;
constexpr static const u32 __sysctl_net_ipv4_conf_mc_forwarding = 2;
constexpr static const u32 __sysctl_net_ipv4_conf_proxy_arp = 3;
constexpr static const u32 __sysctl_net_ipv4_conf_accept_redirects = 4;
constexpr static const u32 __sysctl_net_ipv4_conf_secure_redirects = 5;
constexpr static const u32 __sysctl_net_ipv4_conf_send_redirects = 6;
constexpr static const u32 __sysctl_net_ipv4_conf_shared_media = 7;
constexpr static const u32 __sysctl_net_ipv4_conf_rp_filter = 8;
constexpr static const u32 __sysctl_net_ipv4_conf_accept_source_route = 9;
constexpr static const u32 __sysctl_net_ipv4_conf_bootp_relay = 10;
constexpr static const u32 __sysctl_net_ipv4_conf_log_martians = 11;
constexpr static const u32 __sysctl_net_ipv4_conf_tag = 12;
constexpr static const u32 __sysctl_net_ipv4_conf_arpfilter = 13;
constexpr static const u32 __sysctl_net_ipv4_conf_medium_id = 14;
constexpr static const u32 __sysctl_net_ipv4_conf_noxfrm = 15;
constexpr static const u32 __sysctl_net_ipv4_conf_nopolicy = 16;
constexpr static const u32 __sysctl_net_ipv4_conf_force_igmp_version = 17;
constexpr static const u32 __sysctl_net_ipv4_conf_arp_announce = 18;
constexpr static const u32 __sysctl_net_ipv4_conf_arp_ignore = 19;
constexpr static const u32 __sysctl_net_ipv4_conf_promote_secondaries = 20;
constexpr static const u32 __sysctl_net_ipv4_conf_arp_accept = 21;
constexpr static const u32 __sysctl_net_ipv4_conf_arp_notify = 22;
constexpr static const u32 __sysctl_net_ipv4_conf_arp_evict_nocarrier = 23;

constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_max = 1;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_tcp_timeout_syn_sent = 2;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_tcp_timeout_syn_recv = 3;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_tcp_timeout_established = 4;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_tcp_timeout_fin_wait = 5;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_tcp_timeout_close_wait = 6;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_tcp_timeout_last_ack = 7;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_tcp_timeout_time_wait = 8;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_tcp_timeout_close = 9;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_udp_timeout = 10;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_udp_timeout_stream = 11;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_icmp_timeout = 12;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_generic_timeout = 13;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_buckets = 14;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_log_invalid = 15;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_tcp_timeout_max_retrans = 16;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_tcp_loose = 17;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_tcp_be_liberal = 18;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_tcp_max_retrans = 19;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_sctp_timeout_closed = 20;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_sctp_timeout_cookie_wait = 21;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_sctp_timeout_cookie_echoed = 22;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_sctp_timeout_established = 23;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_sctp_timeout_shutdown_sent = 24;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_sctp_timeout_shutdown_recd = 25;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_sctp_timeout_shutdown_ack_sent = 26;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_count = 27;
constexpr static const u32 __sysctl_net_ipv4_nf_conntrack_checksum = 28;

constexpr static const u32 __sysctl_net_ipv6_conf = 16;
constexpr static const u32 __sysctl_net_ipv6_neigh = 17;
constexpr static const u32 __sysctl_net_ipv6_route = 18;
constexpr static const u32 __sysctl_net_ipv6_icmp = 19;
constexpr static const u32 __sysctl_net_ipv6_bindv6only = 20;
constexpr static const u32 __sysctl_net_ipv6_ip6frag_high_thresh = 21;
constexpr static const u32 __sysctl_net_ipv6_ip6frag_low_thresh = 22;
constexpr static const u32 __sysctl_net_ipv6_ip6frag_time = 23;
constexpr static const u32 __sysctl_net_ipv6_ip6frag_secret_interval = 24;
constexpr static const u32 __sysctl_net_ipv6_mld_max_msf = 25;

constexpr static const u32 __sysctl_net_ipv6_route_flush = 1;
constexpr static const u32 __sysctl_net_ipv6_route_gc_thresh = 2;
constexpr static const u32 __sysctl_net_ipv6_route_max_size = 3;
constexpr static const u32 __sysctl_net_ipv6_route_gc_min_interval = 4;
constexpr static const u32 __sysctl_net_ipv6_route_gc_timeout = 5;
constexpr static const u32 __sysctl_net_ipv6_route_gc_interval = 6;
constexpr static const u32 __sysctl_net_ipv6_route_gc_elasticity = 7;
constexpr static const u32 __sysctl_net_ipv6_route_mtu_expires = 8;
constexpr static const u32 __sysctl_net_ipv6_route_min_advmss = 9;
constexpr static const u32 __sysctl_net_ipv6_route_gc_min_interval_ms = 10;

constexpr static const u32 __sysctl_net_ipv6_forwarding = 1;
constexpr static const u32 __sysctl_net_ipv6_hop_limit = 2;
constexpr static const u32 __sysctl_net_ipv6_mtu = 3;
constexpr static const u32 __sysctl_net_ipv6_accept_ra = 4;
constexpr static const u32 __sysctl_net_ipv6_accept_redirects = 5;
constexpr static const u32 __sysctl_net_ipv6_autoconf = 6;
constexpr static const u32 __sysctl_net_ipv6_dad_transmits = 7;
constexpr static const u32 __sysctl_net_ipv6_rtr_solicits = 8;
constexpr static const u32 __sysctl_net_ipv6_rtr_solicit_interval = 9;
constexpr static const u32 __sysctl_net_ipv6_rtr_solicit_delay = 10;
constexpr static const u32 __sysctl_net_ipv6_use_tempaddr = 11;
constexpr static const u32 __sysctl_net_ipv6_temp_valid_lft = 12;
constexpr static const u32 __sysctl_net_ipv6_temp_prefered_lft = 13;
constexpr static const u32 __sysctl_net_ipv6_regen_max_retry = 14;
constexpr static const u32 __sysctl_net_ipv6_max_desync_factor = 15;
constexpr static const u32 __sysctl_net_ipv6_max_addresses = 16;
constexpr static const u32 __sysctl_net_ipv6_force_mld_version = 17;
constexpr static const u32 __sysctl_net_ipv6_accept_ra_defrtr = 18;
constexpr static const u32 __sysctl_net_ipv6_accept_ra_pinfo = 19;
constexpr static const u32 __sysctl_net_ipv6_accept_ra_rtr_pref = 20;
constexpr static const u32 __sysctl_net_ipv6_rtr_probe_interval = 21;
constexpr static const u32 __sysctl_net_ipv6_accept_ra_rt_info_max_plen = 22;
constexpr static const u32 __sysctl_net_ipv6_proxy_ndp = 23;
constexpr static const u32 __sysctl_net_ipv6_accept_source_route = 25;
constexpr static const u32 __sysctl_net_ipv6_accept_ra_from_local = 26;
constexpr static const u32 __sysctl_net_ipv6_accept_ra_rt_info_min_plen = 27;
constexpr static const u32 __sysctl_net_ipv6_ra_defrtr_metric = 28;
constexpr static const u32 __sysctl_net_ipv6_force_forwarding = 29;

constexpr static const u32 __sysctl_net_ipv6_icmp_ratelimit = 1;
constexpr static const u32 __sysctl_net_ipv6_icmp_echo_ignore_all = 2;

constexpr static const u32 __sysctl_net_neigh_mcast_solicit = 1;
constexpr static const u32 __sysctl_net_neigh_ucast_solicit = 2;
constexpr static const u32 __sysctl_net_neigh_app_solicit = 3;
constexpr static const u32 __sysctl_net_neigh_retrans_time = 4;
constexpr static const u32 __sysctl_net_neigh_reachable_time = 5;
constexpr static const u32 __sysctl_net_neigh_delay_probe_time = 6;
constexpr static const u32 __sysctl_net_neigh_gc_stale_time = 7;
constexpr static const u32 __sysctl_net_neigh_unres_qlen = 8;
constexpr static const u32 __sysctl_net_neigh_proxy_qlen = 9;
constexpr static const u32 __sysctl_net_neigh_anycast_delay = 10;
constexpr static const u32 __sysctl_net_neigh_proxy_delay = 11;
constexpr static const u32 __sysctl_net_neigh_locktime = 12;
constexpr static const u32 __sysctl_net_neigh_gc_interval = 13;
constexpr static const u32 __sysctl_net_neigh_gc_thresh1 = 14;
constexpr static const u32 __sysctl_net_neigh_gc_thresh2 = 15;
constexpr static const u32 __sysctl_net_neigh_gc_thresh3 = 16;
constexpr static const u32 __sysctl_net_neigh_retrans_time_ms = 17;
constexpr static const u32 __sysctl_net_neigh_reachable_time_ms = 18;
constexpr static const u32 __sysctl_net_neigh_interval_probe_time_ms = 19;

constexpr static const u32 __sysctl_net_dccp_default = 1;

constexpr static const u32 __sysctl_net_ipx_pprop_broadcasting = 1;
constexpr static const u32 __sysctl_net_ipx_forwarding = 2;

constexpr static const u32 __sysctl_net_llc2 = 1;
constexpr static const u32 __sysctl_net_llc_station = 2;

constexpr static const u32 __sysctl_net_llc2_timeout = 1;

constexpr static const u32 __sysctl_net_llc_station_ack_timeout = 1;

constexpr static const u32 __sysctl_net_llc2_ack_timeout = 1;
constexpr static const u32 __sysctl_net_llc2_p_timeout = 2;
constexpr static const u32 __sysctl_net_llc2_rej_timeout = 3;
constexpr static const u32 __sysctl_net_llc2_busy_timeout = 4;

constexpr static const u32 __sysctl_net_atalk_aarp_expiry_time = 1;
constexpr static const u32 __sysctl_net_atalk_aarp_tick_time = 2;
constexpr static const u32 __sysctl_net_atalk_aarp_retransmit_limit = 3;
constexpr static const u32 __sysctl_net_atalk_aarp_resolve_time = 4;

constexpr static const u32 __sysctl_net_netrom_default_path_quality = 1;
constexpr static const u32 __sysctl_net_netrom_obsolescence_count_initialiser = 2;
constexpr static const u32 __sysctl_net_netrom_network_ttl_initialiser = 3;
constexpr static const u32 __sysctl_net_netrom_transport_timeout = 4;
constexpr static const u32 __sysctl_net_netrom_transport_maximum_tries = 5;
constexpr static const u32 __sysctl_net_netrom_transport_acknowledge_delay = 6;
constexpr static const u32 __sysctl_net_netrom_transport_busy_delay = 7;
constexpr static const u32 __sysctl_net_netrom_transport_requested_window_size = 8;
constexpr static const u32 __sysctl_net_netrom_transport_no_activity_timeout = 9;
constexpr static const u32 __sysctl_net_netrom_routing_control = 10;
constexpr static const u32 __sysctl_net_netrom_link_fails_count = 11;
constexpr static const u32 __sysctl_net_netrom_reset = 12;

constexpr static const u32 __sysctl_net_ax25_ip_default_mode = 1;
constexpr static const u32 __sysctl_net_ax25_default_mode = 2;
constexpr static const u32 __sysctl_net_ax25_backoff_type = 3;
constexpr static const u32 __sysctl_net_ax25_connect_mode = 4;
constexpr static const u32 __sysctl_net_ax25_standard_window = 5;
constexpr static const u32 __sysctl_net_ax25_extended_window = 6;
constexpr static const u32 __sysctl_net_ax25_t1_timeout = 7;
constexpr static const u32 __sysctl_net_ax25_t2_timeout = 8;
constexpr static const u32 __sysctl_net_ax25_t3_timeout = 9;
constexpr static const u32 __sysctl_net_ax25_idle_timeout = 10;
constexpr static const u32 __sysctl_net_ax25_n2 = 11;
constexpr static const u32 __sysctl_net_ax25_paclen = 12;
constexpr static const u32 __sysctl_net_ax25_protocol = 13;
constexpr static const u32 __sysctl_net_ax25_dama_slave_timeout = 14;

constexpr static const u32 __sysctl_net_rose_restart_request_timeout = 1;
constexpr static const u32 __sysctl_net_rose_call_request_timeout = 2;
constexpr static const u32 __sysctl_net_rose_reset_request_timeout = 3;
constexpr static const u32 __sysctl_net_rose_clear_request_timeout = 4;
constexpr static const u32 __sysctl_net_rose_ack_hold_back_timeout = 5;
constexpr static const u32 __sysctl_net_rose_routing_control = 6;
constexpr static const u32 __sysctl_net_rose_link_fail_timeout = 7;
constexpr static const u32 __sysctl_net_rose_max_vcs = 8;
constexpr static const u32 __sysctl_net_rose_window_size = 9;
constexpr static const u32 __sysctl_net_rose_no_activity_timeout = 10;

constexpr static const u32 __sysctl_net_x25_restart_request_timeout = 1;
constexpr static const u32 __sysctl_net_x25_call_request_timeout = 2;
constexpr static const u32 __sysctl_net_x25_reset_request_timeout = 3;
constexpr static const u32 __sysctl_net_x25_clear_request_timeout = 4;
constexpr static const u32 __sysctl_net_x25_ack_hold_back_timeout = 5;
constexpr static const u32 __sysctl_net_x25_forward = 6;

constexpr static const u32 __sysctl_net_tr_rif_timeout = 1;

constexpr static const u32 __sysctl_net_decnet_node_type = 1;
constexpr static const u32 __sysctl_net_decnet_node_address = 2;
constexpr static const u32 __sysctl_net_decnet_node_name = 3;
constexpr static const u32 __sysctl_net_decnet_default_device = 4;
constexpr static const u32 __sysctl_net_decnet_time_wait = 5;
constexpr static const u32 __sysctl_net_decnet_dn_count = 6;
constexpr static const u32 __sysctl_net_decnet_di_count = 7;
constexpr static const u32 __sysctl_net_decnet_dr_count = 8;
constexpr static const u32 __sysctl_net_decnet_dst_gc_interval = 9;
constexpr static const u32 __sysctl_net_decnet_conf = 10;
constexpr static const u32 __sysctl_net_decnet_no_fc_max_cwnd = 11;
constexpr static const u32 __sysctl_net_decnet_mem = 12;
constexpr static const u32 __sysctl_net_decnet_rmem = 13;
constexpr static const u32 __sysctl_net_decnet_wmem = 14;
constexpr static const u32 __sysctl_net_decnet_debug_level = 255;

constexpr static const u32 __sysctl_net_decnet_conf_loopback = static_cast<u32>(-2);
constexpr static const u32 __sysctl_net_decnet_conf_ddcmp = static_cast<u32>(-3);
constexpr static const u32 __sysctl_net_decnet_conf_ppp = static_cast<u32>(-4);
constexpr static const u32 __sysctl_net_decnet_conf_x25 = static_cast<u32>(-5);
constexpr static const u32 __sysctl_net_decnet_conf_gre = static_cast<u32>(-6);
constexpr static const u32 __sysctl_net_decnet_conf_ether = static_cast<u32>(-7);

constexpr static const u32 __sysctl_net_decnet_conf_dev_priority = 1;
constexpr static const u32 __sysctl_net_decnet_conf_dev_t1 = 2;
constexpr static const u32 __sysctl_net_decnet_conf_dev_t2 = 3;
constexpr static const u32 __sysctl_net_decnet_conf_dev_t3 = 4;
constexpr static const u32 __sysctl_net_decnet_conf_dev_forwarding = 5;
constexpr static const u32 __sysctl_net_decnet_conf_dev_blksize = 6;
constexpr static const u32 __sysctl_net_decnet_conf_dev_state = 7;

constexpr static const u32 __sysctl_net_sctp_rto_initial = 1;
constexpr static const u32 __sysctl_net_sctp_rto_min = 2;
constexpr static const u32 __sysctl_net_sctp_rto_max = 3;
constexpr static const u32 __sysctl_net_sctp_rto_alpha = 4;
constexpr static const u32 __sysctl_net_sctp_rto_beta = 5;
constexpr static const u32 __sysctl_net_sctp_valid_cookie_life = 6;
constexpr static const u32 __sysctl_net_sctp_association_max_retrans = 7;
constexpr static const u32 __sysctl_net_sctp_path_max_retrans = 8;
constexpr static const u32 __sysctl_net_sctp_max_init_retransmits = 9;
constexpr static const u32 __sysctl_net_sctp_hb_interval = 10;
constexpr static const u32 __sysctl_net_sctp_preserve_enable = 11;
constexpr static const u32 __sysctl_net_sctp_max_burst = 12;
constexpr static const u32 __sysctl_net_sctp_addip_enable = 13;
constexpr static const u32 __sysctl_net_sctp_prsctp_enable = 14;
constexpr static const u32 __sysctl_net_sctp_sndbuf_policy = 15;
constexpr static const u32 __sysctl_net_sctp_sack_timeout = 16;
constexpr static const u32 __sysctl_net_sctp_rcvbuf_policy = 17;

constexpr static const u32 __sysctl_net_bridge_nf_call_arptables = 1;
constexpr static const u32 __sysctl_net_bridge_nf_call_iptables = 2;
constexpr static const u32 __sysctl_net_bridge_nf_call_ip6tables = 3;
constexpr static const u32 __sysctl_net_bridge_nf_filter_vlan_tagged = 4;
constexpr static const u32 __sysctl_net_bridge_nf_filter_pppoe_tagged = 5;

constexpr static const u32 __sysctl_fs_nrinode = 1;
constexpr static const u32 __sysctl_fs_statinode = 2;
constexpr static const u32 __sysctl_fs_maxinode = 3;
constexpr static const u32 __sysctl_fs_nrdquot = 4;
constexpr static const u32 __sysctl_fs_maxdquot = 5;
constexpr static const u32 __sysctl_fs_nrfile = 6;
constexpr static const u32 __sysctl_fs_maxfile = 7;
constexpr static const u32 __sysctl_fs_dentry = 8;
constexpr static const u32 __sysctl_fs_nrsuper = 9;
constexpr static const u32 __sysctl_fs_maxsuper = 10;
constexpr static const u32 __sysctl_fs_overflowuid = 11;
constexpr static const u32 __sysctl_fs_overflowgid = 12;
constexpr static const u32 __sysctl_fs_leases = 13;
constexpr static const u32 __sysctl_fs_dir_notify = 14;
constexpr static const u32 __sysctl_fs_lease_time = 15;
constexpr static const u32 __sysctl_fs_dqstats = 16;
constexpr static const u32 __sysctl_fs_xfs = 17;
constexpr static const u32 __sysctl_fs_aio_nr = 18;
constexpr static const u32 __sysctl_fs_aio_max_nr = 19;
constexpr static const u32 __sysctl_fs_inotify = 20;
constexpr static const u32 __sysctl_fs_ocfs2 = 988;

constexpr static const u32 __sysctl_fs_dq_lookups = 1;
constexpr static const u32 __sysctl_fs_dq_drops = 2;
constexpr static const u32 __sysctl_fs_dq_reads = 3;
constexpr static const u32 __sysctl_fs_dq_writes = 4;
constexpr static const u32 __sysctl_fs_dq_cache_hits = 5;
constexpr static const u32 __sysctl_fs_dq_allocated = 6;
constexpr static const u32 __sysctl_fs_dq_free = 7;
constexpr static const u32 __sysctl_fs_dq_syncs = 8;
constexpr static const u32 __sysctl_fs_dq_warnings = 9;

constexpr static const u32 __sysctl_dev_cdrom = 1;
constexpr static const u32 __sysctl_dev_hwmon = 2;
constexpr static const u32 __sysctl_dev_parport = 3;
constexpr static const u32 __sysctl_dev_raid = 4;
constexpr static const u32 __sysctl_dev_mac_hid = 5;
constexpr static const u32 __sysctl_dev_scsi = 6;
constexpr static const u32 __sysctl_dev_ipmi = 7;

constexpr static const u32 __sysctl_dev_cdrom_info = 1;
constexpr static const u32 __sysctl_dev_cdrom_autoclose = 2;
constexpr static const u32 __sysctl_dev_cdrom_autoeject = 3;
constexpr static const u32 __sysctl_dev_cdrom_debug = 4;
constexpr static const u32 __sysctl_dev_cdrom_lock = 5;
constexpr static const u32 __sysctl_dev_cdrom_check_media = 6;

constexpr static const u32 __sysctl_dev_parport_default = static_cast<u32>(-3);

constexpr static const u32 __sysctl_dev_raid_speed_limit_min = 1;
constexpr static const u32 __sysctl_dev_raid_speed_limit_max = 2;

constexpr static const u32 __sysctl_dev_parport_default_timeslice = 1;
constexpr static const u32 __sysctl_dev_parport_default_spintime = 2;

constexpr static const u32 __sysctl_dev_parport_spintime = 1;
constexpr static const u32 __sysctl_dev_parport_base_addr = 2;
constexpr static const u32 __sysctl_dev_parport_irq = 3;
constexpr static const u32 __sysctl_dev_parport_dma = 4;
constexpr static const u32 __sysctl_dev_parport_modes = 5;
constexpr static const u32 __sysctl_dev_parport_devices = 6;
constexpr static const u32 __sysctl_dev_parport_autoprobe = 16;

constexpr static const u32 __sysctl_dev_parport_devices_active = static_cast<u32>(-3);

constexpr static const u32 __sysctl_dev_parport_device_timeslice = 1;

constexpr static const u32 __sysctl_dev_mac_hid_keyboard_sends_linux_keycodes = 1;
constexpr static const u32 __sysctl_dev_mac_hid_keyboard_lock_keycodes = 2;
constexpr static const u32 __sysctl_dev_mac_hid_mouse_button_emulation = 3;
constexpr static const u32 __sysctl_dev_mac_hid_mouse_button2_keycode = 4;
constexpr static const u32 __sysctl_dev_mac_hid_mouse_button3_keycode = 5;
constexpr static const u32 __sysctl_dev_mac_hid_adb_mouse_sends_keycodes = 6;

constexpr static const u32 __sysctl_dev_scsi_logging_level = 1;

constexpr static const u32 __sysctl_dev_ipmi_poweroff_powercycle = 1;

constexpr static const u32 __sysctl_abi_defhandler_coff = 1;
constexpr static const u32 __sysctl_abi_defhandler_elf = 2;
constexpr static const u32 __sysctl_abi_defhandler_lcall7 = 3;
constexpr static const u32 __sysctl_abi_defhandler_libcso = 4;
constexpr static const u32 __sysctl_abi_trace = 5;
constexpr static const u32 __sysctl_abi_fake_utsname = 6;

enum class sysctl_toplevel : u32 {
  kern = __sysctl_ctl_kern,
  vm = __sysctl_ctl_vm,
  net = __sysctl_ctl_net,
  proc = __sysctl_ctl_proc,
  fs = __sysctl_ctl_fs,
  debug = __sysctl_ctl_debug,
  dev = __sysctl_ctl_dev,
  bus = __sysctl_ctl_bus,
  abi = __sysctl_ctl_abi,
  cpu = __sysctl_ctl_cpu,
  arlan = __sysctl_ctl_arlan,
  s390dbf = __sysctl_ctl_s390dbf,
  sunrpc = __sysctl_ctl_sunrpc,
  pm = __sysctl_ctl_pm,
  frv = __sysctl_ctl_frv,
};

enum class sysctl_bus : u32 {
  isa = __sysctl_ctl_bus_isa,
};

enum class sysctl_inotify : u32 {
  max_user_instances = __sysctl_inotify_max_user_instances,
  max_user_watches = __sysctl_inotify_max_user_watches,
  max_queued_events = __sysctl_inotify_max_queued_events,
};

enum class sysctl_kern : u32 {
  ostype = __sysctl_kern_ostype,
  osrelease = __sysctl_kern_osrelease,
  osrev = __sysctl_kern_osrev,
  version = __sysctl_kern_version,
  securemask = __sysctl_kern_securemask,
  prof = __sysctl_kern_prof,
  nodename = __sysctl_kern_nodename,
  domainname = __sysctl_kern_domainname,
  panic = __sysctl_kern_panic,
  realrootdev = __sysctl_kern_realrootdev,
  sparc_reboot = __sysctl_kern_sparc_reboot,
  ctlaltdel = __sysctl_kern_ctlaltdel,
  printk = __sysctl_kern_printk,
  nametrans = __sysctl_kern_nametrans,
  ppc_htabreclaim = __sysctl_kern_ppc_htabreclaim,
  ppc_zeropaged = __sysctl_kern_ppc_zeropaged,
  ppc_powersave_nap = __sysctl_kern_ppc_powersave_nap,
  modprobe = __sysctl_kern_modprobe,
  sg_big_buff = __sysctl_kern_sg_big_buff,
  acct = __sysctl_kern_acct,
  ppc_l2cr = __sysctl_kern_ppc_l2cr,
  rtsignr = __sysctl_kern_rtsignr,
  rtsigmax = __sysctl_kern_rtsigmax,
  shmmax = __sysctl_kern_shmmax,
  msgmax = __sysctl_kern_msgmax,
  msgmnb = __sysctl_kern_msgmnb,
  msgpool = __sysctl_kern_msgpool,
  sysrq = __sysctl_kern_sysrq,
  max_threads = __sysctl_kern_max_threads,
  random = __sysctl_kern_random,
  shmall = __sysctl_kern_shmall,
  msgmni = __sysctl_kern_msgmni,
  sem = __sysctl_kern_sem,
  sparc_stop_a = __sysctl_kern_sparc_stop_a,
  shmmni = __sysctl_kern_shmmni,
  overflowuid = __sysctl_kern_overflowuid,
  overflowgid = __sysctl_kern_overflowgid,
  shmpath = __sysctl_kern_shmpath,
  hotplug = __sysctl_kern_hotplug,
  ieee_emulation_warnings = __sysctl_kern_ieee_emulation_warnings,
  s390_user_debug_logging = __sysctl_kern_s390_user_debug_logging,
  core_uses_pid = __sysctl_kern_core_uses_pid,
  tainted = __sysctl_kern_tainted,
  cadpid = __sysctl_kern_cadpid,
  pidmax = __sysctl_kern_pidmax,
  core_pattern = __sysctl_kern_core_pattern,
  panic_on_oops = __sysctl_kern_panic_on_oops,
  hppa_pwrsw = __sysctl_kern_hppa_pwrsw,
  hppa_unaligned = __sysctl_kern_hppa_unaligned,
  printk_ratelimit = __sysctl_kern_printk_ratelimit,
  printk_ratelimit_burst = __sysctl_kern_printk_ratelimit_burst,
  pty = __sysctl_kern_pty,
  ngroups_max = __sysctl_kern_ngroups_max,
  sparc_scons_pwroff = __sysctl_kern_sparc_scons_pwroff,
  hz_timer = __sysctl_kern_hz_timer,
  unknown_nmi_panic = __sysctl_kern_unknown_nmi_panic,
  bootloader_type = __sysctl_kern_bootloader_type,
  randomize = __sysctl_kern_randomize,
  setuid_dumpable = __sysctl_kern_setuid_dumpable,
  spin_retry = __sysctl_kern_spin_retry,
  acpi_video_flags = __sysctl_kern_acpi_video_flags,
  ia64_unaligned = __sysctl_kern_ia64_unaligned,
  compat_log = __sysctl_kern_compat_log,
  max_lock_depth = __sysctl_kern_max_lock_depth,
  nmi_watchdog = __sysctl_kern_nmi_watchdog,
  panic_on_nmi = __sysctl_kern_panic_on_nmi,
  panic_on_warn = __sysctl_kern_panic_on_warn,
  panic_print = __sysctl_kern_panic_print,
};

enum class sysctl_vm : u32 {
  unused1 = __sysctl_vm_unused1,
  unused2 = __sysctl_vm_unused2,
  unused3 = __sysctl_vm_unused3,
  unused4 = __sysctl_vm_unused4,
  overcommit_memory = __sysctl_vm_overcommit_memory,
  unused5 = __sysctl_vm_unused5,
  unused7 = __sysctl_vm_unused7,
  unused8 = __sysctl_vm_unused8,
  unused9 = __sysctl_vm_unused9,
  page_cluster = __sysctl_vm_page_cluster,
  dirty_background = __sysctl_vm_dirty_background,
  dirty_ratio = __sysctl_vm_dirty_ratio,
  dirty_wb_cs = __sysctl_vm_dirty_wb_cs,
  dirty_expire_cs = __sysctl_vm_dirty_expire_cs,
  nr_pdflush_threads = __sysctl_vm_nr_pdflush_threads,
  overcommit_ratio = __sysctl_vm_overcommit_ratio,
  pagebuf = __sysctl_vm_pagebuf,
  hugetlb_pages = __sysctl_vm_hugetlb_pages,
  swappiness = __sysctl_vm_swappiness,
  lowmem_reserve_ratio = __sysctl_vm_lowmem_reserve_ratio,
  min_free_kbytes = __sysctl_vm_min_free_kbytes,
  max_map_count = __sysctl_vm_max_map_count,
  laptop_mode = __sysctl_vm_laptop_mode,
  block_dump = __sysctl_vm_block_dump,
  hugetlb_group = __sysctl_vm_hugetlb_group,
  vfs_cache_pressure = __sysctl_vm_vfs_cache_pressure,
  legacy_va_layout = __sysctl_vm_legacy_va_layout,
  swap_token_timeout = __sysctl_vm_swap_token_timeout,
  drop_pagecache = __sysctl_vm_drop_pagecache,
  percpu_pagelist_fraction = __sysctl_vm_percpu_pagelist_fraction,
  zone_reclaim_mode = __sysctl_vm_zone_reclaim_mode,
  min_unmapped = __sysctl_vm_min_unmapped,
  panic_on_oom = __sysctl_vm_panic_on_oom,
  vdso_enabled = __sysctl_vm_vdso_enabled,
  min_slab = __sysctl_vm_min_slab,
};

enum class sysctl_net : u32 {
  core = __sysctl_net_core,
  ether = __sysctl_net_ether,
  net802 = __sysctl_net_802,
  unix = __sysctl_net_unix,
  ipv4 = __sysctl_net_ipv4,
  ipx = __sysctl_net_ipx,
  atalk = __sysctl_net_atalk,
  netrom = __sysctl_net_netrom,
  ax25 = __sysctl_net_ax25,
  bridge = __sysctl_net_bridge,
  rose = __sysctl_net_rose,
  ipv6 = __sysctl_net_ipv6,
  x25 = __sysctl_net_x25,
  tr = __sysctl_net_tr,
  decnet = __sysctl_net_decnet,
  econet = __sysctl_net_econet,
  sctp = __sysctl_net_sctp,
  llc = __sysctl_net_llc,
  netfilter = __sysctl_net_netfilter,
  dccp = __sysctl_net_dccp,
  irda = __sysctl_net_irda,
};

enum class sysctl_random : u32 {
  poolsize = __sysctl_random_poolsize,
  entropy_count = __sysctl_random_entropy_count,
  read_thresh = __sysctl_random_read_thresh,
  write_thresh = __sysctl_random_write_thresh,
  boot_id = __sysctl_random_boot_id,
  uuid = __sysctl_random_uuid,
};

enum class sysctl_pty : u32 {
  max = __sysctl_pty_max,
  nr = __sysctl_pty_nr,
};

enum class sysctl_busisa : u32 {
  mem_base = __sysctl_bus_isa_mem_base,
  port_base = __sysctl_bus_isa_port_base,
  port_shift = __sysctl_bus_isa_port_shift,
};

enum class sysctl_netcore : u32 {
  wmem_max = __sysctl_net_core_wmem_max,
  rmem_max = __sysctl_net_core_rmem_max,
  wmem_default = __sysctl_net_core_wmem_default,
  rmem_default = __sysctl_net_core_rmem_default,
  max_backlog = __sysctl_net_core_max_backlog,
  fastroute = __sysctl_net_core_fastroute,
  msg_cost = __sysctl_net_core_msg_cost,
  msg_burst = __sysctl_net_core_msg_burst,
  optmem_max = __sysctl_net_core_optmem_max,
  hot_list_length = __sysctl_net_core_hot_list_length,
  divert_version = __sysctl_net_core_divert_version,
  no_cong_thresh = __sysctl_net_core_no_cong_thresh,
  no_cong = __sysctl_net_core_no_cong,
  lo_cong = __sysctl_net_core_lo_cong,
  mod_cong = __sysctl_net_core_mod_cong,
  dev_weight = __sysctl_net_core_dev_weight,
  somaxconn = __sysctl_net_core_somaxconn,
  budget = __sysctl_net_core_budget,
  aevent_etime = __sysctl_net_core_aevent_etime,
  aevent_rseqth = __sysctl_net_core_aevent_rseqth,
  warnings = __sysctl_net_core_warnings,
};

enum class sysctl_netunix : u32 {
  destroy_delay = __sysctl_net_unix_destroy_delay,
  delete_delay = __sysctl_net_unix_delete_delay,
  max_dgram_qlen = __sysctl_net_unix_max_dgram_qlen,
};

enum class sysctl_netfilter : u32 {
  conntrack_max = __sysctl_net_nf_conntrack_max,
  conntrack_tcp_timeout_syn_sent = __sysctl_net_nf_conntrack_tcp_timeout_syn_sent,
  conntrack_tcp_timeout_syn_recv = __sysctl_net_nf_conntrack_tcp_timeout_syn_recv,
  conntrack_tcp_timeout_established = __sysctl_net_nf_conntrack_tcp_timeout_established,
  conntrack_tcp_timeout_fin_wait = __sysctl_net_nf_conntrack_tcp_timeout_fin_wait,
  conntrack_tcp_timeout_close_wait = __sysctl_net_nf_conntrack_tcp_timeout_close_wait,
  conntrack_tcp_timeout_last_ack = __sysctl_net_nf_conntrack_tcp_timeout_last_ack,
  conntrack_tcp_timeout_time_wait = __sysctl_net_nf_conntrack_tcp_timeout_time_wait,
  conntrack_tcp_timeout_close = __sysctl_net_nf_conntrack_tcp_timeout_close,
  conntrack_udp_timeout = __sysctl_net_nf_conntrack_udp_timeout,
  conntrack_udp_timeout_stream = __sysctl_net_nf_conntrack_udp_timeout_stream,
  conntrack_icmp_timeout = __sysctl_net_nf_conntrack_icmp_timeout,
  conntrack_generic_timeout = __sysctl_net_nf_conntrack_generic_timeout,
  conntrack_buckets = __sysctl_net_nf_conntrack_buckets,
  conntrack_log_invalid = __sysctl_net_nf_conntrack_log_invalid,
  conntrack_tcp_timeout_max_retrans = __sysctl_net_nf_conntrack_tcp_timeout_max_retrans,
  conntrack_tcp_loose = __sysctl_net_nf_conntrack_tcp_loose,
  conntrack_tcp_be_liberal = __sysctl_net_nf_conntrack_tcp_be_liberal,
  conntrack_tcp_max_retrans = __sysctl_net_nf_conntrack_tcp_max_retrans,
  conntrack_sctp_timeout_closed = __sysctl_net_nf_conntrack_sctp_timeout_closed,
  conntrack_sctp_timeout_cookie_wait = __sysctl_net_nf_conntrack_sctp_timeout_cookie_wait,
  conntrack_sctp_timeout_cookie_echoed = __sysctl_net_nf_conntrack_sctp_timeout_cookie_echoed,
  conntrack_sctp_timeout_established = __sysctl_net_nf_conntrack_sctp_timeout_established,
  conntrack_sctp_timeout_shutdown_sent = __sysctl_net_nf_conntrack_sctp_timeout_shutdown_sent,
  conntrack_sctp_timeout_shutdown_recd = __sysctl_net_nf_conntrack_sctp_timeout_shutdown_recd,
  conntrack_sctp_timeout_shutdown_ack_sent = __sysctl_net_nf_conntrack_sctp_timeout_shutdown_ack_sent,
  conntrack_count = __sysctl_net_nf_conntrack_count,
  conntrack_icmpv6_timeout = __sysctl_net_nf_conntrack_icmpv6_timeout,
  conntrack_frag6_timeout = __sysctl_net_nf_conntrack_frag6_timeout,
  conntrack_frag6_low_thresh = __sysctl_net_nf_conntrack_frag6_low_thresh,
  conntrack_frag6_high_thresh = __sysctl_net_nf_conntrack_frag6_high_thresh,
  conntrack_checksum = __sysctl_net_nf_conntrack_checksum,
};

enum class sysctl_netipv4 : u32 {
  forward = __sysctl_net_ipv4_forward,
  dynaddr = __sysctl_net_ipv4_dynaddr,
  conf = __sysctl_net_ipv4_conf,
  neigh = __sysctl_net_ipv4_neigh,
  route = __sysctl_net_ipv4_route,
  fib_hash = __sysctl_net_ipv4_fib_hash,
  netfilter = __sysctl_net_ipv4_netfilter,
  tcp_timestamps = __sysctl_net_ipv4_tcp_timestamps,
  tcp_window_scaling = __sysctl_net_ipv4_tcp_window_scaling,
  tcp_sack = __sysctl_net_ipv4_tcp_sack,
  tcp_retrans_collapse = __sysctl_net_ipv4_tcp_retrans_collapse,
  default_ttl = __sysctl_net_ipv4_default_ttl,
  autoconfig = __sysctl_net_ipv4_autoconfig,
  no_pmtu_disc = __sysctl_net_ipv4_no_pmtu_disc,
  tcp_syn_retries = __sysctl_net_ipv4_tcp_syn_retries,
  ipfrag_high_thresh = __sysctl_net_ipv4_ipfrag_high_thresh,
  ipfrag_low_thresh = __sysctl_net_ipv4_ipfrag_low_thresh,
  ipfrag_time = __sysctl_net_ipv4_ipfrag_time,
  tcp_max_ka_probes = __sysctl_net_ipv4_tcp_max_ka_probes,
  tcp_keepalive_time = __sysctl_net_ipv4_tcp_keepalive_time,
  tcp_keepalive_probes = __sysctl_net_ipv4_tcp_keepalive_probes,
  tcp_retries1 = __sysctl_net_ipv4_tcp_retries1,
  tcp_retries2 = __sysctl_net_ipv4_tcp_retries2,
  tcp_fin_timeout = __sysctl_net_ipv4_tcp_fin_timeout,
  ip_masq_debug = __sysctl_net_ipv4_ip_masq_debug,
  tcp_syncookies = __sysctl_net_tcp_syncookies,
  tcp_stdurg = __sysctl_net_tcp_stdurg,
  tcp_rfc1337 = __sysctl_net_tcp_rfc1337,
  tcp_syn_taildrop = __sysctl_net_tcp_syn_taildrop,
  tcp_max_syn_backlog = __sysctl_net_tcp_max_syn_backlog,
  local_port_range = __sysctl_net_ipv4_local_port_range,
  icmp_echo_ignore_all = __sysctl_net_ipv4_icmp_echo_ignore_all,
  icmp_echo_ignore_broadcasts = __sysctl_net_ipv4_icmp_echo_ignore_broadcasts,
  icmp_sourcequench_rate = __sysctl_net_ipv4_icmp_sourcequench_rate,
  icmp_destunreach_rate = __sysctl_net_ipv4_icmp_destunreach_rate,
  icmp_timeexceed_rate = __sysctl_net_ipv4_icmp_timeexceed_rate,
  icmp_paramprob_rate = __sysctl_net_ipv4_icmp_paramprob_rate,
  icmp_echoreply_rate = __sysctl_net_ipv4_icmp_echoreply_rate,
  icmp_ignore_bogus_error_responses = __sysctl_net_ipv4_icmp_ignore_bogus_error_responses,
  igmp_max_memberships = __sysctl_net_ipv4_igmp_max_memberships,
  tcp_tw_recycle = __sysctl_net_tcp_tw_recycle,
  always_defrag = __sysctl_net_ipv4_always_defrag,
  tcp_keepalive_intvl = __sysctl_net_ipv4_tcp_keepalive_intvl,
  inet_peer_threshold = __sysctl_net_ipv4_inet_peer_threshold,
  inet_peer_minttl = __sysctl_net_ipv4_inet_peer_minttl,
  inet_peer_maxttl = __sysctl_net_ipv4_inet_peer_maxttl,
  inet_peer_gc_mintime = __sysctl_net_ipv4_inet_peer_gc_mintime,
  inet_peer_gc_maxtime = __sysctl_net_ipv4_inet_peer_gc_maxtime,
  tcp_orphan_retries = __sysctl_net_tcp_orphan_retries,
  tcp_abort_on_overflow = __sysctl_net_tcp_abort_on_overflow,
  tcp_synack_retries = __sysctl_net_tcp_synack_retries,
  tcp_max_orphans = __sysctl_net_tcp_max_orphans,
  tcp_max_tw_buckets = __sysctl_net_tcp_max_tw_buckets,
  tcp_fack = __sysctl_net_tcp_fack,
  tcp_reordering = __sysctl_net_tcp_reordering,
  tcp_ecn = __sysctl_net_tcp_ecn,
  tcp_dsack = __sysctl_net_tcp_dsack,
  tcp_mem = __sysctl_net_tcp_mem,
  tcp_wmem = __sysctl_net_tcp_wmem,
  tcp_rmem = __sysctl_net_tcp_rmem,
  tcp_app_win = __sysctl_net_tcp_app_win,
  tcp_adv_win_scale = __sysctl_net_tcp_adv_win_scale,
  nonlocal_bind = __sysctl_net_ipv4_nonlocal_bind,
  icmp_ratelimit = __sysctl_net_ipv4_icmp_ratelimit,
  icmp_ratemask = __sysctl_net_ipv4_icmp_ratemask,
  tcp_tw_reuse = __sysctl_net_tcp_tw_reuse,
  tcp_frto = __sysctl_net_tcp_frto,
  tcp_low_latency = __sysctl_net_tcp_low_latency,
  ipfrag_secret_interval = __sysctl_net_ipv4_ipfrag_secret_interval,
  igmp_max_msf = __sysctl_net_ipv4_igmp_max_msf,
  tcp_no_metrics_save = __sysctl_net_tcp_no_metrics_save,
  tcp_default_win_scale = __sysctl_net_tcp_default_win_scale,
  tcp_moderate_rcvbuf = __sysctl_net_tcp_moderate_rcvbuf,
  tcp_tso_win_divisor = __sysctl_net_tcp_tso_win_divisor,
  tcp_bic_beta = __sysctl_net_tcp_bic_beta,
  icmp_errors_use_inbound_ifaddr = __sysctl_net_ipv4_icmp_errors_use_inbound_ifaddr,
  tcp_cong_control = __sysctl_net_tcp_cong_control,
  tcp_abc = __sysctl_net_tcp_abc,
  ipfrag_max_dist = __sysctl_net_ipv4_ipfrag_max_dist,
  tcp_mtu_probing = __sysctl_net_tcp_mtu_probing,
  tcp_base_mss = __sysctl_net_tcp_base_mss,
  tcp_workaround_signed_windows = __sysctl_net_ipv4_tcp_workaround_signed_windows,
  tcp_dma_copybreak = __sysctl_net_tcp_dma_copybreak,
  tcp_slow_start_after_idle = __sysctl_net_tcp_slow_start_after_idle,
  cipsov4_cache_enable = __sysctl_net_cipsov4_cache_enable,
  cipsov4_cache_bucket_size = __sysctl_net_cipsov4_cache_bucket_size,
  cipsov4_rbm_optfmt = __sysctl_net_cipsov4_rbm_optfmt,
  cipsov4_rbm_strictvalid = __sysctl_net_cipsov4_rbm_strictvalid,
  tcp_avail_cong_control = __sysctl_net_tcp_avail_cong_control,
  tcp_allowed_cong_control = __sysctl_net_tcp_allowed_cong_control,
  tcp_max_ssthresh = __sysctl_net_tcp_max_ssthresh,
  tcp_frto_response = __sysctl_net_tcp_frto_response,
};

enum class sysctl_netipv4route : u32 {
  flush = __sysctl_net_ipv4_route_flush,
  min_delay = __sysctl_net_ipv4_route_min_delay,
  max_delay = __sysctl_net_ipv4_route_max_delay,
  gc_thresh = __sysctl_net_ipv4_route_gc_thresh,
  max_size = __sysctl_net_ipv4_route_max_size,
  gc_min_interval = __sysctl_net_ipv4_route_gc_min_interval,
  gc_timeout = __sysctl_net_ipv4_route_gc_timeout,
  gc_interval = __sysctl_net_ipv4_route_gc_interval,
  redirect_load = __sysctl_net_ipv4_route_redirect_load,
  redirect_number = __sysctl_net_ipv4_route_redirect_number,
  redirect_silence = __sysctl_net_ipv4_route_redirect_silence,
  error_cost = __sysctl_net_ipv4_route_error_cost,
  error_burst = __sysctl_net_ipv4_route_error_burst,
  gc_elasticity = __sysctl_net_ipv4_route_gc_elasticity,
  mtu_expires = __sysctl_net_ipv4_route_mtu_expires,
  min_pmtu = __sysctl_net_ipv4_route_min_pmtu,
  min_advmss = __sysctl_net_ipv4_route_min_advmss,
  secret_interval = __sysctl_net_ipv4_route_secret_interval,
  gc_min_interval_ms = __sysctl_net_ipv4_route_gc_min_interval_ms,
};

enum class sysctl_netipv4conf : u32 {
  forwarding = __sysctl_net_ipv4_conf_forwarding,
  mc_forwarding = __sysctl_net_ipv4_conf_mc_forwarding,
  proxy_arp = __sysctl_net_ipv4_conf_proxy_arp,
  accept_redirects = __sysctl_net_ipv4_conf_accept_redirects,
  secure_redirects = __sysctl_net_ipv4_conf_secure_redirects,
  send_redirects = __sysctl_net_ipv4_conf_send_redirects,
  shared_media = __sysctl_net_ipv4_conf_shared_media,
  rp_filter = __sysctl_net_ipv4_conf_rp_filter,
  accept_source_route = __sysctl_net_ipv4_conf_accept_source_route,
  bootp_relay = __sysctl_net_ipv4_conf_bootp_relay,
  log_martians = __sysctl_net_ipv4_conf_log_martians,
  tag = __sysctl_net_ipv4_conf_tag,
  arpfilter = __sysctl_net_ipv4_conf_arpfilter,
  medium_id = __sysctl_net_ipv4_conf_medium_id,
  noxfrm = __sysctl_net_ipv4_conf_noxfrm,
  nopolicy = __sysctl_net_ipv4_conf_nopolicy,
  force_igmp_version = __sysctl_net_ipv4_conf_force_igmp_version,
  arp_announce = __sysctl_net_ipv4_conf_arp_announce,
  arp_ignore = __sysctl_net_ipv4_conf_arp_ignore,
  promote_secondaries = __sysctl_net_ipv4_conf_promote_secondaries,
  arp_accept = __sysctl_net_ipv4_conf_arp_accept,
  arp_notify = __sysctl_net_ipv4_conf_arp_notify,
  arp_evict_nocarrier = __sysctl_net_ipv4_conf_arp_evict_nocarrier,
};

enum class sysctl_netipv4netfilter : u32 {
  conntrack_max = __sysctl_net_ipv4_nf_conntrack_max,
  conntrack_tcp_timeout_syn_sent = __sysctl_net_ipv4_nf_conntrack_tcp_timeout_syn_sent,
  conntrack_tcp_timeout_syn_recv = __sysctl_net_ipv4_nf_conntrack_tcp_timeout_syn_recv,
  conntrack_tcp_timeout_established = __sysctl_net_ipv4_nf_conntrack_tcp_timeout_established,
  conntrack_tcp_timeout_fin_wait = __sysctl_net_ipv4_nf_conntrack_tcp_timeout_fin_wait,
  conntrack_tcp_timeout_close_wait = __sysctl_net_ipv4_nf_conntrack_tcp_timeout_close_wait,
  conntrack_tcp_timeout_last_ack = __sysctl_net_ipv4_nf_conntrack_tcp_timeout_last_ack,
  conntrack_tcp_timeout_time_wait = __sysctl_net_ipv4_nf_conntrack_tcp_timeout_time_wait,
  conntrack_tcp_timeout_close = __sysctl_net_ipv4_nf_conntrack_tcp_timeout_close,
  conntrack_udp_timeout = __sysctl_net_ipv4_nf_conntrack_udp_timeout,
  conntrack_udp_timeout_stream = __sysctl_net_ipv4_nf_conntrack_udp_timeout_stream,
  conntrack_icmp_timeout = __sysctl_net_ipv4_nf_conntrack_icmp_timeout,
  conntrack_generic_timeout = __sysctl_net_ipv4_nf_conntrack_generic_timeout,
  conntrack_buckets = __sysctl_net_ipv4_nf_conntrack_buckets,
  conntrack_log_invalid = __sysctl_net_ipv4_nf_conntrack_log_invalid,
  conntrack_tcp_timeout_max_retrans = __sysctl_net_ipv4_nf_conntrack_tcp_timeout_max_retrans,
  conntrack_tcp_loose = __sysctl_net_ipv4_nf_conntrack_tcp_loose,
  conntrack_tcp_be_liberal = __sysctl_net_ipv4_nf_conntrack_tcp_be_liberal,
  conntrack_tcp_max_retrans = __sysctl_net_ipv4_nf_conntrack_tcp_max_retrans,
  conntrack_sctp_timeout_closed = __sysctl_net_ipv4_nf_conntrack_sctp_timeout_closed,
  conntrack_sctp_timeout_cookie_wait = __sysctl_net_ipv4_nf_conntrack_sctp_timeout_cookie_wait,
  conntrack_sctp_timeout_cookie_echoed = __sysctl_net_ipv4_nf_conntrack_sctp_timeout_cookie_echoed,
  conntrack_sctp_timeout_established = __sysctl_net_ipv4_nf_conntrack_sctp_timeout_established,
  conntrack_sctp_timeout_shutdown_sent = __sysctl_net_ipv4_nf_conntrack_sctp_timeout_shutdown_sent,
  conntrack_sctp_timeout_shutdown_recd = __sysctl_net_ipv4_nf_conntrack_sctp_timeout_shutdown_recd,
  conntrack_sctp_timeout_shutdown_ack_sent = __sysctl_net_ipv4_nf_conntrack_sctp_timeout_shutdown_ack_sent,
  conntrack_count = __sysctl_net_ipv4_nf_conntrack_count,
  conntrack_checksum = __sysctl_net_ipv4_nf_conntrack_checksum,
};

enum class sysctl_netipv6 : u32 {
  conf = __sysctl_net_ipv6_conf,
  neigh = __sysctl_net_ipv6_neigh,
  route = __sysctl_net_ipv6_route,
  icmp = __sysctl_net_ipv6_icmp,
  bindv6only = __sysctl_net_ipv6_bindv6only,
  ip6frag_high_thresh = __sysctl_net_ipv6_ip6frag_high_thresh,
  ip6frag_low_thresh = __sysctl_net_ipv6_ip6frag_low_thresh,
  ip6frag_time = __sysctl_net_ipv6_ip6frag_time,
  ip6frag_secret_interval = __sysctl_net_ipv6_ip6frag_secret_interval,
  mld_max_msf = __sysctl_net_ipv6_mld_max_msf,
};

enum class sysctl_netipv6route : u32 {
  flush = __sysctl_net_ipv6_route_flush,
  gc_thresh = __sysctl_net_ipv6_route_gc_thresh,
  max_size = __sysctl_net_ipv6_route_max_size,
  gc_min_interval = __sysctl_net_ipv6_route_gc_min_interval,
  gc_timeout = __sysctl_net_ipv6_route_gc_timeout,
  gc_interval = __sysctl_net_ipv6_route_gc_interval,
  gc_elasticity = __sysctl_net_ipv6_route_gc_elasticity,
  mtu_expires = __sysctl_net_ipv6_route_mtu_expires,
  min_advmss = __sysctl_net_ipv6_route_min_advmss,
  gc_min_interval_ms = __sysctl_net_ipv6_route_gc_min_interval_ms,
};

enum class sysctl_netipv6conf : u32 {
  forwarding = __sysctl_net_ipv6_forwarding,
  hop_limit = __sysctl_net_ipv6_hop_limit,
  mtu = __sysctl_net_ipv6_mtu,
  accept_ra = __sysctl_net_ipv6_accept_ra,
  accept_redirects = __sysctl_net_ipv6_accept_redirects,
  autoconf = __sysctl_net_ipv6_autoconf,
  dad_transmits = __sysctl_net_ipv6_dad_transmits,
  rtr_solicits = __sysctl_net_ipv6_rtr_solicits,
  rtr_solicit_interval = __sysctl_net_ipv6_rtr_solicit_interval,
  rtr_solicit_delay = __sysctl_net_ipv6_rtr_solicit_delay,
  use_tempaddr = __sysctl_net_ipv6_use_tempaddr,
  temp_valid_lft = __sysctl_net_ipv6_temp_valid_lft,
  temp_prefered_lft = __sysctl_net_ipv6_temp_prefered_lft,
  regen_max_retry = __sysctl_net_ipv6_regen_max_retry,
  max_desync_factor = __sysctl_net_ipv6_max_desync_factor,
  max_addresses = __sysctl_net_ipv6_max_addresses,
  force_mld_version = __sysctl_net_ipv6_force_mld_version,
  accept_ra_defrtr = __sysctl_net_ipv6_accept_ra_defrtr,
  accept_ra_pinfo = __sysctl_net_ipv6_accept_ra_pinfo,
  accept_ra_rtr_pref = __sysctl_net_ipv6_accept_ra_rtr_pref,
  rtr_probe_interval = __sysctl_net_ipv6_rtr_probe_interval,
  accept_ra_rt_info_max_plen = __sysctl_net_ipv6_accept_ra_rt_info_max_plen,
  proxy_ndp = __sysctl_net_ipv6_proxy_ndp,
  accept_source_route = __sysctl_net_ipv6_accept_source_route,
  accept_ra_from_local = __sysctl_net_ipv6_accept_ra_from_local,
  accept_ra_rt_info_min_plen = __sysctl_net_ipv6_accept_ra_rt_info_min_plen,
  ra_defrtr_metric = __sysctl_net_ipv6_ra_defrtr_metric,
  force_forwarding = __sysctl_net_ipv6_force_forwarding,
};

enum class sysctl_netipv6icmp : u32 {
  ratelimit = __sysctl_net_ipv6_icmp_ratelimit,
  echo_ignore_all = __sysctl_net_ipv6_icmp_echo_ignore_all,
};

enum class sysctl_netneigh : u32 {
  mcast_solicit = __sysctl_net_neigh_mcast_solicit,
  ucast_solicit = __sysctl_net_neigh_ucast_solicit,
  app_solicit = __sysctl_net_neigh_app_solicit,
  retrans_time = __sysctl_net_neigh_retrans_time,
  reachable_time = __sysctl_net_neigh_reachable_time,
  delay_probe_time = __sysctl_net_neigh_delay_probe_time,
  gc_stale_time = __sysctl_net_neigh_gc_stale_time,
  unres_qlen = __sysctl_net_neigh_unres_qlen,
  proxy_qlen = __sysctl_net_neigh_proxy_qlen,
  anycast_delay = __sysctl_net_neigh_anycast_delay,
  proxy_delay = __sysctl_net_neigh_proxy_delay,
  locktime = __sysctl_net_neigh_locktime,
  gc_interval = __sysctl_net_neigh_gc_interval,
  gc_thresh1 = __sysctl_net_neigh_gc_thresh1,
  gc_thresh2 = __sysctl_net_neigh_gc_thresh2,
  gc_thresh3 = __sysctl_net_neigh_gc_thresh3,
  retrans_time_ms = __sysctl_net_neigh_retrans_time_ms,
  reachable_time_ms = __sysctl_net_neigh_reachable_time_ms,
  interval_probe_time_ms = __sysctl_net_neigh_interval_probe_time_ms,
};

enum class sysctl_netdccp : u32 {
  default_ = __sysctl_net_dccp_default,
};

enum class sysctl_netipx : u32 {
  pprop_broadcasting = __sysctl_net_ipx_pprop_broadcasting,
  forwarding = __sysctl_net_ipx_forwarding,
};

enum class sysctl_netllc : u32 {
  llc2 = __sysctl_net_llc2,
  station = __sysctl_net_llc_station,
};

enum class sysctl_netllc2timeout : u32 {
  ack = __sysctl_net_llc2_ack_timeout,
  p = __sysctl_net_llc2_p_timeout,
  rej = __sysctl_net_llc2_rej_timeout,
  busy = __sysctl_net_llc2_busy_timeout,
};

enum class sysctl_netatalk : u32 {
  aarp_expiry_time = __sysctl_net_atalk_aarp_expiry_time,
  aarp_tick_time = __sysctl_net_atalk_aarp_tick_time,
  aarp_retransmit_limit = __sysctl_net_atalk_aarp_retransmit_limit,
  aarp_resolve_time = __sysctl_net_atalk_aarp_resolve_time,
};

enum class sysctl_netnetrom : u32 {
  default_path_quality = __sysctl_net_netrom_default_path_quality,
  obsolescence_count_initialiser = __sysctl_net_netrom_obsolescence_count_initialiser,
  network_ttl_initialiser = __sysctl_net_netrom_network_ttl_initialiser,
  transport_timeout = __sysctl_net_netrom_transport_timeout,
  transport_maximum_tries = __sysctl_net_netrom_transport_maximum_tries,
  transport_acknowledge_delay = __sysctl_net_netrom_transport_acknowledge_delay,
  transport_busy_delay = __sysctl_net_netrom_transport_busy_delay,
  transport_requested_window_size = __sysctl_net_netrom_transport_requested_window_size,
  transport_no_activity_timeout = __sysctl_net_netrom_transport_no_activity_timeout,
  routing_control = __sysctl_net_netrom_routing_control,
  link_fails_count = __sysctl_net_netrom_link_fails_count,
  reset = __sysctl_net_netrom_reset,
};

enum class sysctl_netax25 : u32 {
  ip_default_mode = __sysctl_net_ax25_ip_default_mode,
  default_mode = __sysctl_net_ax25_default_mode,
  backoff_type = __sysctl_net_ax25_backoff_type,
  connect_mode = __sysctl_net_ax25_connect_mode,
  standard_window = __sysctl_net_ax25_standard_window,
  extended_window = __sysctl_net_ax25_extended_window,
  t1_timeout = __sysctl_net_ax25_t1_timeout,
  t2_timeout = __sysctl_net_ax25_t2_timeout,
  t3_timeout = __sysctl_net_ax25_t3_timeout,
  idle_timeout = __sysctl_net_ax25_idle_timeout,
  n2 = __sysctl_net_ax25_n2,
  paclen = __sysctl_net_ax25_paclen,
  protocol = __sysctl_net_ax25_protocol,
  dama_slave_timeout = __sysctl_net_ax25_dama_slave_timeout,
};

enum class sysctl_netrose : u32 {
  restart_request_timeout = __sysctl_net_rose_restart_request_timeout,
  call_request_timeout = __sysctl_net_rose_call_request_timeout,
  reset_request_timeout = __sysctl_net_rose_reset_request_timeout,
  clear_request_timeout = __sysctl_net_rose_clear_request_timeout,
  ack_hold_back_timeout = __sysctl_net_rose_ack_hold_back_timeout,
  routing_control = __sysctl_net_rose_routing_control,
  link_fail_timeout = __sysctl_net_rose_link_fail_timeout,
  max_vcs = __sysctl_net_rose_max_vcs,
  window_size = __sysctl_net_rose_window_size,
  no_activity_timeout = __sysctl_net_rose_no_activity_timeout,
};

enum class sysctl_netx25 : u32 {
  restart_request_timeout = __sysctl_net_x25_restart_request_timeout,
  call_request_timeout = __sysctl_net_x25_call_request_timeout,
  reset_request_timeout = __sysctl_net_x25_reset_request_timeout,
  clear_request_timeout = __sysctl_net_x25_clear_request_timeout,
  ack_hold_back_timeout = __sysctl_net_x25_ack_hold_back_timeout,
  forward = __sysctl_net_x25_forward,
};

enum class sysctl_nettr : u32 {
  rif_timeout = __sysctl_net_tr_rif_timeout,
};

enum class sysctl_netdecnet : u32 {
  node_type = __sysctl_net_decnet_node_type,
  node_address = __sysctl_net_decnet_node_address,
  node_name = __sysctl_net_decnet_node_name,
  default_device = __sysctl_net_decnet_default_device,
  time_wait = __sysctl_net_decnet_time_wait,
  dn_count = __sysctl_net_decnet_dn_count,
  di_count = __sysctl_net_decnet_di_count,
  dr_count = __sysctl_net_decnet_dr_count,
  dst_gc_interval = __sysctl_net_decnet_dst_gc_interval,
  conf = __sysctl_net_decnet_conf,
  no_fc_max_cwnd = __sysctl_net_decnet_no_fc_max_cwnd,
  mem = __sysctl_net_decnet_mem,
  rmem = __sysctl_net_decnet_rmem,
  wmem = __sysctl_net_decnet_wmem,
  debug_level = __sysctl_net_decnet_debug_level,
};

enum class sysctl_netdecnetconfdev : u32 {
  priority = __sysctl_net_decnet_conf_dev_priority,
  t1 = __sysctl_net_decnet_conf_dev_t1,
  t2 = __sysctl_net_decnet_conf_dev_t2,
  t3 = __sysctl_net_decnet_conf_dev_t3,
  forwarding = __sysctl_net_decnet_conf_dev_forwarding,
  blksize = __sysctl_net_decnet_conf_dev_blksize,
  state = __sysctl_net_decnet_conf_dev_state,
};

enum class sysctl_netsctp : u32 {
  rto_initial = __sysctl_net_sctp_rto_initial,
  rto_min = __sysctl_net_sctp_rto_min,
  rto_max = __sysctl_net_sctp_rto_max,
  rto_alpha = __sysctl_net_sctp_rto_alpha,
  rto_beta = __sysctl_net_sctp_rto_beta,
  valid_cookie_life = __sysctl_net_sctp_valid_cookie_life,
  association_max_retrans = __sysctl_net_sctp_association_max_retrans,
  path_max_retrans = __sysctl_net_sctp_path_max_retrans,
  max_init_retransmits = __sysctl_net_sctp_max_init_retransmits,
  hb_interval = __sysctl_net_sctp_hb_interval,
  preserve_enable = __sysctl_net_sctp_preserve_enable,
  max_burst = __sysctl_net_sctp_max_burst,
  addip_enable = __sysctl_net_sctp_addip_enable,
  prsctp_enable = __sysctl_net_sctp_prsctp_enable,
  sndbuf_policy = __sysctl_net_sctp_sndbuf_policy,
  sack_timeout = __sysctl_net_sctp_sack_timeout,
  rcvbuf_policy = __sysctl_net_sctp_rcvbuf_policy,
};

enum class sysctl_netbridge : u32 {
  nf_call_arptables = __sysctl_net_bridge_nf_call_arptables,
  nf_call_iptables = __sysctl_net_bridge_nf_call_iptables,
  nf_call_ip6tables = __sysctl_net_bridge_nf_call_ip6tables,
  nf_filter_vlan_tagged = __sysctl_net_bridge_nf_filter_vlan_tagged,
  nf_filter_pppoe_tagged = __sysctl_net_bridge_nf_filter_pppoe_tagged,
};

enum class sysctl_fs : u32 {
  nrinode = __sysctl_fs_nrinode,
  statinode = __sysctl_fs_statinode,
  maxinode = __sysctl_fs_maxinode,
  nrdquot = __sysctl_fs_nrdquot,
  maxdquot = __sysctl_fs_maxdquot,
  nrfile = __sysctl_fs_nrfile,
  maxfile = __sysctl_fs_maxfile,
  dentry = __sysctl_fs_dentry,
  nrsuper = __sysctl_fs_nrsuper,
  maxsuper = __sysctl_fs_maxsuper,
  overflowuid = __sysctl_fs_overflowuid,
  overflowgid = __sysctl_fs_overflowgid,
  leases = __sysctl_fs_leases,
  dir_notify = __sysctl_fs_dir_notify,
  lease_time = __sysctl_fs_lease_time,
  dqstats = __sysctl_fs_dqstats,
  xfs = __sysctl_fs_xfs,
  aio_nr = __sysctl_fs_aio_nr,
  aio_max_nr = __sysctl_fs_aio_max_nr,
  inotify = __sysctl_fs_inotify,
  ocfs2 = __sysctl_fs_ocfs2,
};

enum class sysctl_fsquota : u32 {
  lookups = __sysctl_fs_dq_lookups,
  drops = __sysctl_fs_dq_drops,
  reads = __sysctl_fs_dq_reads,
  writes = __sysctl_fs_dq_writes,
  cache_hits = __sysctl_fs_dq_cache_hits,
  allocated = __sysctl_fs_dq_allocated,
  free = __sysctl_fs_dq_free,
  syncs = __sysctl_fs_dq_syncs,
  warnings = __sysctl_fs_dq_warnings,
};

enum class sysctl_dev : u32 {
  cdrom = __sysctl_dev_cdrom,
  hwmon = __sysctl_dev_hwmon,
  parport = __sysctl_dev_parport,
  raid = __sysctl_dev_raid,
  mac_hid = __sysctl_dev_mac_hid,
  scsi = __sysctl_dev_scsi,
  ipmi = __sysctl_dev_ipmi,
};

enum class sysctl_devcdrom : u32 {
  info = __sysctl_dev_cdrom_info,
  autoclose = __sysctl_dev_cdrom_autoclose,
  autoeject = __sysctl_dev_cdrom_autoeject,
  debug = __sysctl_dev_cdrom_debug,
  lock = __sysctl_dev_cdrom_lock,
  check_media = __sysctl_dev_cdrom_check_media,
};

enum class sysctl_devraid : u32 {
  speed_limit_min = __sysctl_dev_raid_speed_limit_min,
  speed_limit_max = __sysctl_dev_raid_speed_limit_max,
};

enum class sysctl_devparportdefault : u32 {
  timeslice = __sysctl_dev_parport_default_timeslice,
  spintime = __sysctl_dev_parport_default_spintime,
};

enum class sysctl_devparport : u32 {
  spintime = __sysctl_dev_parport_spintime,
  base_addr = __sysctl_dev_parport_base_addr,
  irq = __sysctl_dev_parport_irq,
  dma = __sysctl_dev_parport_dma,
  modes = __sysctl_dev_parport_modes,
  devices = __sysctl_dev_parport_devices,
  autoprobe = __sysctl_dev_parport_autoprobe,
};

enum class sysctl_devmachid : u32 {
  keyboard_sends_linux_keycodes = __sysctl_dev_mac_hid_keyboard_sends_linux_keycodes,
  keyboard_lock_keycodes = __sysctl_dev_mac_hid_keyboard_lock_keycodes,
  mouse_button_emulation = __sysctl_dev_mac_hid_mouse_button_emulation,
  mouse_button2_keycode = __sysctl_dev_mac_hid_mouse_button2_keycode,
  mouse_button3_keycode = __sysctl_dev_mac_hid_mouse_button3_keycode,
  adb_mouse_sends_keycodes = __sysctl_dev_mac_hid_adb_mouse_sends_keycodes,
};

enum class sysctl_devscsi : u32 {
  logging_level = __sysctl_dev_scsi_logging_level,
};

enum class sysctl_devipmi : u32 {
  poweroff_powercycle = __sysctl_dev_ipmi_poweroff_powercycle,
};

enum class sysctl_abi : u32 {
  defhandler_coff = __sysctl_abi_defhandler_coff,
  defhandler_elf = __sysctl_abi_defhandler_elf,
  defhandler_lcall7 = __sysctl_abi_defhandler_lcall7,
  defhandler_libcso = __sysctl_abi_defhandler_libcso,
  trace = __sysctl_abi_trace,
  fake_utsname = __sysctl_abi_fake_utsname,
};

};     // namespace micron
