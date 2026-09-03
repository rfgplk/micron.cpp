//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// compile-validity gate for micron::sec and its raw layer. Not run: every call here would
// irreversibly confine the calling process. This checks the template surface, the kernel ABI
// static_asserts, and -- the point of the file -- that the four-way arch ladders are exhaustive,
// which only a build for arm32 / arm64 / i386 can prove.

#include "../../src/sec.hpp"

#include "../../src/linux/process/seccomp.hpp"      // the micron::seccomp compat alias

#include "../../src/linux/sys/landlock.hpp"
#include "../../src/linux/sys/namespaces.hpp"
#include "../../src/linux/sys/seccomp.hpp"
#include "../../src/linux/sys/xattr.hpp"

#include "../../src/kernel.hpp"

namespace mp = micron::posix;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// landlock ABI

static_assert(mp::landlock_access_fs_execute == 0x1uLL);
static_assert(mp::landlock_access_fs_refer == 0x2000uLL);
static_assert(mp::landlock_access_fs_truncate == 0x4000uLL);
static_assert(mp::landlock_access_fs_ioctl_dev == 0x8000uLL);
static_assert(mp::landlock_access_fs_resolve_unix == 0x10000uLL);
static_assert(mp::landlock_access_net_connect_tcp == 0x2uLL);
static_assert(mp::landlock_scope_signal == 0x2uLL);
static_assert(mp::landlock_create_ruleset_version == 0x1u);
static_assert(mp::landlock_restrict_self_tsync == 0x8u);

static_assert(mp::landlock_fs_mask_for(1) == 0x1fffuLL);
static_assert(mp::landlock_fs_mask_for(2) == 0x3fffuLL);
static_assert(mp::landlock_fs_mask_for(3) == 0x7fffuLL);
static_assert(mp::landlock_fs_mask_for(5) == 0xffffuLL);
static_assert(mp::landlock_fs_mask_for(9) == 0x1ffffuLL);
static_assert(mp::landlock_net_mask_for(3) == 0uLL);
static_assert(mp::landlock_net_mask_for(4) == 0x3uLL);
static_assert(mp::landlock_scope_mask_for(5) == 0uLL);

static_assert(mp::landlock_ruleset_size_for(1) == 8);
static_assert(mp::landlock_ruleset_size_for(4) == 16);
static_assert(mp::landlock_ruleset_size_for(6) == 24);

// the size asserts live in the header too; repeated here because an unpacked path_beneath_attr is
// a silent EINVAL at runtime and nothing else
static_assert(sizeof(mp::landlock_path_beneath_attr_t) == 12);
static_assert(sizeof(mp::landlock_ruleset_attr_t) == 24);
static_assert(sizeof(mp::landlock_net_port_attr_t) == 16);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// nsfs ioctl encodings, against the kernel's _IO/_IOR values

static_assert(mp::ns_get_userns == 0x0000B701uLL);
static_assert(mp::ns_get_parent == 0x0000B702uLL);
static_assert(mp::ns_get_nstype == 0x0000B703uLL);
static_assert(mp::ns_get_owner_uid == 0x0000B704uLL);
static_assert(mp::ns_get_mntns_id == 0x8008B705uLL);
static_assert(mp::ns_get_pid_from_pidns == 0x8004B706uLL);
static_assert(mp::ns_get_tgid_in_pidns == 0x8004B709uLL);
static_assert(mp::ns_mnt_get_info == 0x8010B70AuLL);
static_assert(mp::ns_get_id == 0x8008B70DuLL);
static_assert(sizeof(mp::mnt_ns_info_t) == 16);

static_assert(mp::user_ns_init_ino == 0xEFFFFFFDuLL);
static_assert(mp::mnt_ns_init_ino == 0xEFFFFFF8uLL);

// CLONE_NEWTIME sits in the CSIGNAL byte, so it is unshare/clone3-only -- never legacy clone
static_assert(mp::clone_newtime == 0x80uLL);
static_assert(mp::clone_newns == 0x00020000uLL);
static_assert(mp::clone_newuser == 0x10000000uLL);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// seccomp / xattr constants

static_assert(mp::seccomp_mode_strict == 1u);
static_assert(mp::seccomp_mode_filter == 2u);
static_assert(mp::seccomp_ret_action == 0x7fff0000u);
static_assert(mp::seccomp_ret_action_full == 0xffff0000u);
static_assert(mp::seccomp_ioctl_notif_set_flags == 0x40082104u);
static_assert(mp::xattr_create == 1 && mp::xattr_replace == 2);

static_assert(micron::kernel::feature::landlock == micron::kernel::ver(5, 13));
static_assert(micron::kernel::feature::landlock_net == micron::kernel::ver(6, 7));

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// instantiation: force every wrapper to be emitted

void
__sec_surface(void)
{
  (void)mp::landlock_abi_version();
  (void)mp::landlock_errata();
  (void)mp::landlock_add_path_beneath(-1, -1, mp::landlock_access_fs_read_file);
  (void)mp::landlock_add_net_port(-1, 0, mp::landlock_access_net_bind_tcp);
  (void)mp::landlock_restrict_self(-1, 0);

  mp::landlock_ruleset_attr_t ra{ mp::landlock_access_fs_read_file, 0, 0 };
  (void)mp::landlock_create_ruleset(&ra, mp::landlock_ruleset_size_ver2, 0);

  micron::uid_t uid{};
  u64 id = 0;
  (void)mp::setns(-1, 0);
  (void)mp::unshare(0);
  (void)mp::ns_type_of(-1);
  (void)mp::ns_parent_of(-1);
  (void)mp::ns_userns_of(-1);
  (void)mp::ns_owner_uid_of(-1, uid);
  (void)mp::ns_id_of(-1, id);
  (void)mp::ns_mntns_id_of(-1, id);

  (void)mp::getxattr("/", mp::xattr_name_selinux, nullptr, 0);
  (void)mp::lgetxattr("/", mp::xattr_name_selinux, nullptr, 0);
  (void)mp::flistxattr(-1, nullptr, 0);
  (void)mp::lremovexattr("/", mp::xattr_name_selinux);

  (void)mp::ms_shared;
  (void)mp::ms_slave;
  (void)mp::ms_unbindable;
  (void)mp::ms_nosymfollow;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the syscall groups. THIS is why the file is swept for arm32 / arm64 / i386: an `#if/#elif` with
// no `#else` leaves `calls` undeclared, and only a build for that arch says so

namespace g = micron::sec::groups;

template<typename G>
  requires micron::sec::is_syscall_group<G>
constexpr bool
__group_ok()
{
  return G::count > 0 && G::calls[0] >= 0;
}

static_assert(__group_ok<g::baseline>());
static_assert(__group_ok<g::memory>());
static_assert(__group_ok<g::io>());
static_assert(__group_ok<g::filesystem>());
static_assert(__group_ok<g::filesystem_readonly>());
static_assert(__group_ok<g::filesystem_no_mount>());
static_assert(__group_ok<g::process>());
static_assert(__group_ok<g::process_no_ns>());
static_assert(__group_ok<g::signal>());
static_assert(__group_ok<g::network>());
static_assert(__group_ok<g::time>());
static_assert(__group_ok<g::ipc>());
static_assert(__group_ok<g::capabilities>());
static_assert(__group_ok<g::io_multiplexing>());

// the groups added by the CVE audit. Each is `#if`-fanned like the others, so an arch arm with no
// `#else` leaves `calls` undeclared and only a build for THAT arch says so -- which is the whole
// reason these live here rather than in a runtime test.
static_assert(__group_ok<g::baseline_tty>());
static_assert(__group_ok<g::namespaces>());
static_assert(__group_ok<g::mount_api>());
static_assert(__group_ok<g::keyring>());
static_assert(__group_ok<g::kernel_debug>());
static_assert(__group_ok<g::uring>());
static_assert(__group_ok<g::kernel_attack_surface>());

// filesystem_readonly must not be able to WRITE, on any arch. This is a compile-time gate rather
// than a runtime one because the member lists are `#if`-selected: a mutating call reintroduced on
// arm32 alone would never be seen by an amd64 test run
constexpr bool
__readonly_is_readonly(void)
{
  for ( usize i = 0; i < g::filesystem_readonly::count; ++i ) {
    const i32 nr = g::filesystem_readonly::calls[i];
    if ( nr == SYS_linkat || nr == SYS_symlinkat || nr == SYS_unlinkat || nr == SYS_renameat2 || nr == SYS_mkdirat || nr == SYS_mknodat )
      return false;
#if !defined(__micron_arch_arm64)
    if ( nr == SYS_link || nr == SYS_symlink || nr == SYS_unlink || nr == SYS_rename || nr == SYS_renameat || nr == SYS_mkdir
         || nr == SYS_rmdir || nr == SYS_mknod )
      return false;
#endif
  }
  return true;
}

static_assert(__readonly_is_readonly(), "micron::sec::groups::filesystem_readonly allows a mutating syscall");

// A GROUP MUST NAME THE SYSCALLS MICRON ACTUALLY ISSUES, NOT THEIR SAME-NAMED TWINS.
// On i386/arm32, system.hpp selects the *32 uid/gid forms (see __sys_getuid); a group listing only
// the legacy 16-bit numbers denies micron's own getuid()/setuid() under a default-deny policy --
// including the credential VERIFICATION the sandbox does on itself. Only a 32-bit build sees this
template<typename G>
constexpr bool
__group_has(i32 nr)
{
  for ( usize i = 0; i < G::count; ++i )
    if ( G::calls[i] == nr ) return true;
  return false;
}

constexpr bool
__process_names_the_real_id_calls(void)
{
  return __group_has<g::process>(micron::posix::__sys_getuid) && __group_has<g::process>(micron::posix::__sys_geteuid)
         && __group_has<g::process>(micron::posix::__sys_getgid) && __group_has<g::process>(micron::posix::__sys_getegid)
         && __group_has<g::process>(micron::posix::__sys_setuid) && __group_has<g::process>(micron::posix::__sys_setgid)
         && __group_has<g::process_no_ns>(micron::posix::__sys_getuid) && __group_has<g::process_no_ns>(micron::posix::__sys_geteuid)
         && __group_has<g::process_no_ns>(micron::posix::__sys_getgid) && __group_has<g::process_no_ns>(micron::posix::__sys_getegid)
         && __group_has<g::capabilities>(micron::posix::__sys_setuid) && __group_has<g::capabilities>(micron::posix::__sys_setgid);
}

static_assert(__process_names_the_real_id_calls(), "a syscall group omits the uid/gid syscall micron itself issues on this arch");

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the CVE-audit invariants, on every arch
//
// Each of these is a finding from tests/adv/ restated as a compile-time fact, because the adv tier's
// runtime cells are x86_64-only (tests/adv/adv.duck) and every one of these lists is `#if`-selected.
// A regression reintroduced on arm32 alone would otherwise be invisible until somebody built for it.

// CVE-2019-10063: unfiltered ioctl in the group every policy starts from is a tty-injection
// primitive, and sandbox::stdio() is what puts the tty on fd 0.
constexpr bool
__baseline_has_no_raw_ioctl(void)
{
  return !__group_has<g::baseline>(SYS_ioctl) && !__group_has<g::io>(SYS_ioctl) && !__group_has<g::filesystem>(SYS_ioctl)
         && !__group_has<g::network>(SYS_ioctl) && !__group_has<g::ipc>(SYS_ioctl) && !__group_has<g::io_multiplexing>(SYS_ioctl);
}

static_assert(__baseline_has_no_raw_ioctl(), "a shipped allow-group grants unfiltered ioctl: that is TIOCSTI, i.e. CVE-2019-10063");

// CVE-2026-63917: clone3's flags live behind a pointer, so no seccomp rule can constrain them. A
// group whose name promises no namespaces must therefore not name it at all.
constexpr bool
__no_ns_group_omits_clone3(void)
{
  return !__group_has<g::process_no_ns>(SYS_clone3) && !__group_has<g::process_no_ns>(SYS_unshare)
         && !__group_has<g::process_no_ns>(SYS_setns) && __group_has<g::namespaces>(SYS_clone3) && __group_has<g::namespaces>(SYS_unshare)
         && __group_has<g::namespaces>(SYS_setns);
}

static_assert(__no_ns_group_omits_clone3(), "groups::process_no_ns names a namespace-creating syscall, or groups::namespaces omits one");

// CVE-2022-0185: the mount API a denylist written against mount(2) has never heard of. Both halves
// matter -- mount_api must name all seven, and `filesystem` must grant them, because micron's own
// __remount_ro issues mount_setattr.
constexpr bool
__mount_api_is_complete(void)
{
  const i32 api[] = { SYS_fsopen, SYS_fsconfig, SYS_fsmount, SYS_fspick, SYS_open_tree, SYS_move_mount, SYS_mount_setattr };
  for ( const i32 nr : api ) {
    if ( !__group_has<g::mount_api>(nr) ) return false;
    if ( !__group_has<g::filesystem>(nr) ) return false;
    if ( __group_has<g::filesystem_no_mount>(nr) ) return false;
    if ( __group_has<g::filesystem_readonly>(nr) ) return false;
  }
  return true;
}

static_assert(__mount_api_is_complete(), "groups::mount_api or groups::filesystem is missing a new-mount-API syscall, "
                                         "or a no-mount group names one");

// CVE-2024-42318: a confinement living on a credential is only as durable as every path that
// rebuilds one. No allow-group may reach the keyring; the deny group must name all three.
constexpr bool
__keyring_is_out_of_reach(void)
{
  const i32 keys[] = { SYS_keyctl, SYS_add_key, SYS_request_key };
  for ( const i32 nr : keys ) {
    if ( !__group_has<g::keyring>(nr) ) return false;
    if ( __group_has<g::baseline>(nr) || __group_has<g::io>(nr) || __group_has<g::process>(nr) || __group_has<g::process_no_ns>(nr)
         || __group_has<g::capabilities>(nr) || __group_has<g::ipc>(nr) || __group_has<g::filesystem>(nr) )
      return false;
  }
  return true;
}

static_assert(__keyring_is_out_of_reach(), "an allow-group names a keyring syscall, or groups::keyring omits one");

// and filesystem_no_mount, which is what the old filesystem_readonly actually was, must still be
// unable to reshape the mount tree
constexpr bool
__no_mount_is_no_mount(void)
{
  for ( usize i = 0; i < g::filesystem_no_mount::count; ++i ) {
    const i32 nr = g::filesystem_no_mount::calls[i];
    if ( nr == SYS_mount || nr == SYS_umount2 || nr == SYS_chroot || nr == SYS_pivot_root || nr == SYS_mknodat ) return false;
  }
  return true;
}

static_assert(__no_mount_is_no_mount());

// process_no_ns must genuinely lack the namespace primitives -- that is its whole reason to exist
constexpr bool
__no_ns_clean(void)
{
  for ( usize i = 0; i < g::process_no_ns::count; ++i )
    if ( g::process_no_ns::calls[i] == SYS_unshare || g::process_no_ns::calls[i] == SYS_setns ) return false;
  return true;
}

static_assert(__no_ns_clean());

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the seccomp builder, through both spellings

void
__sec_seccomp_surface(void)
{
  micron::sec::seccomp::filter_builder<256> fb;
  fb.require_native_arch();
  for ( usize i = 0; i < g::baseline::count; ++i ) fb.allow(g::baseline::calls[i]);
  fb.deny_errno(SYS_execve, 1);
  fb.allow_if(SYS_ioctl, micron::sec::seccomp::arg_eq(1, 0x5401));
  fb.default_errno(1);
  (void)fb.valid();

  // the beeos spelling must keep resolving
  micron::seccomp::filter_builder<64> compat;
  compat.require_native_arch_raw().default_kill();
  (void)micron::seccomp::act_errno(1);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// landlock porcelain

namespace ll = micron::sec::landlock;

static_assert(ll::bits(ll::access_fs::read_file) == 0x4uLL);
static_assert(ll::bits(ll::read_only) == 0xcuLL);
static_assert(ll::bits(ll::read_execute) == 0xduLL);
static_assert(ll::bits(ll::read_write) == 0x400euLL);
static_assert(ll::any(ll::access_fs::execute) && !ll::any(ll::access_fs::none));
static_assert(ll::bits(ll::access_fs::read_file | ll::access_fs::execute) == 0x5uLL);
static_assert(ll::bits(ll::read_write &ll::read_only) == 0xcuLL);

void
__sec_landlock_surface(void)
{
  (void)ll::available();
  (void)ll::abi_level();
  (void)ll::supported_fs();
  (void)ll::supported_net();
  (void)ll::supported_scope();

  ll::ruleset r = ll::try_ruleset(ll::read_execute, ll::access_net::bind_tcp, ll::scope::signal);
  (void)r.valid();
  (void)r.error();
  (void)r.abi();
  (void)r.handled();
  (void)r.handled_net();
  (void)r.scoped();
  (void)r.allow("/usr", ll::read_execute);
  (void)r.allow_fd(-1, ll::read_only);
  (void)r.allow_port(443, ll::access_net::connect_tcp);
  (void)r.restrict_self();

  ll::ruleset moved = micron::move(r);
  (void)moved.fd();
  moved.close();
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// namespace porcelain

namespace ns = micron::sec::ns;

static_assert(ns::bits(ns::ns_kind::mount) == 0x00020000uLL);
static_assert(ns::bits(ns::ns_kind::user) == 0x10000000uLL);
static_assert(ns::bits(ns::ns_kind::time) == 0x80uLL);
static_assert(ns::bits(ns::ns_kind::user | ns::ns_kind::mount | ns::ns_kind::pid) == 0x30020000uLL);
static_assert(ns::host_inode_of(ns::ns_kind::user) == 0xEFFFFFFDuLL);
static_assert(ns::host_inode_of(ns::ns_kind::none) == 0uLL);
static_assert(ns::name_of(ns::ns_kind::net) != nullptr);
static_assert(ns::name_of(ns::ns_kind::none) == nullptr);
static_assert(sizeof(ns::all_kinds) / sizeof(ns::all_kinds[0]) == 8);

void
__sec_ns_surface(void)
{
  ns::ns_handle h = ns::open_self(ns::ns_kind::user);
  (void)h.valid();
  (void)h.error();
  (void)h.type();
  (void)h.inode();
  (void)h.is_host();
  (void)h.enter();
  micron::posix::uid_t u{};
  (void)h.owner_uid(u);
  (void)h.owning_userns();
  (void)h.parent();

  ns::ns_handle moved = micron::move(h);
  (void)moved.fd();

  (void)ns::open_of(1, ns::ns_kind::pid);
  (void)ns::unshare(ns::ns_kind::uts);
  (void)ns::deny_setgroups();
  (void)ns::write_uid_map(0, 1000, 1);
  (void)ns::write_gid_map(0, 1000, 1);
  (void)ns::map_to_root(1000, 1000);
  (void)ns::unshare_user_as_root(ns::ns_kind::mount);

  ns::enter_scope sc{ moved };
  (void)sc.ok();
  (void)sc.error();
  (void)sc.restorable();
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// composition and the functional face
//
// These are the parts most likely to break on an untargeted arch: a type-level policy names
// syscalls through the group ladders, and every one of those has to resolve on all four ABIs

namespace sp = micron::sec;

using __ct_seccomp = sp::seccomp_policy<sp::allow<sp::groups::baseline, sp::groups::io, sp::groups::memory>, sp::deny<sp::groups::network>>;
using __ct_strict = sp::seccomp_strict_policy<sp::allow<sp::groups::baseline>>;
using __ct_caps = sp::capability_policy<micron::cap::net_bind_service, micron::cap::sys_chroot>;
using __ct_fs = sp::filesystem_policy<sp::make_private, sp::bind<"/usr", "/j/usr", true>, sp::tmpfs<"/j/tmp">, sp::proc_mount<"/j/proc">,
                                      sp::pivot_to<"/j", "/j/old">>;
using __ct_fs2 = sp::filesystem_policy<sp::chroot_to<"/j">>;
using __ct_fs3 = sp::filesystem_policy<sp::make_private, sp::pivot_to_keep_old<"/j", "/j/old">, sp::detach<"/old">>;
static_assert(sp::is_filesystem_policy<__ct_fs3>);

// the post-pivot path arithmetic, on every arch
static_assert(sp::__impl::__rel_put_old("/j", "/j/old") != nullptr);
static_assert(sp::__impl::__rel_put_old("/j", "/j/old")[0] == '/');
static_assert(sp::__impl::__rel_put_old("/jail", "/jailhouse/old") == nullptr);
static_assert(sp::__impl::__rel_put_old("/jail", "/jail") == nullptr);
// NOTE (CVE audit): ns_kind::pid is deliberately absent. namespace_policy<...pid...> is now a
// static_assert, because unshare(CLONE_NEWPID) moves the caller's CHILDREN and not the caller -- so
// the old spelling compiled, returned 0, and left the process in the host pid namespace while every
// downstream claim about pid isolation read as true. sec::sandbox does the extra fork and is the
// right tool; this policy face cannot, and now says so at compile time.
using __ct_ns = sp::namespace_policy<sp::ns::ns_kind::user, sp::ns::ns_kind::mount>;
using __ct_ll = sp::landlock_policy<sp::beneath<"/usr", sp::landlock::read_execute>>;
using __ct_rl = sp::rlimit_policy<sp::limit<micron::posix::rlimit_nofile, 64>>;

static_assert(sp::is_seccomp_policy<__ct_seccomp> && sp::is_capability_policy<__ct_caps>);
static_assert(sp::is_filesystem_policy<__ct_fs> && sp::is_namespace_policy<__ct_ns>);
static_assert(sp::is_landlock_policy<__ct_ll> && sp::is_rlimit_policy<__ct_rl>);
static_assert(__ct_ns::has_user && !sp::namespace_policy_none::has_user);
// CLONE_NEWUSER (0x10000000) | CLONE_NEWNS (0x00020000). CLONE_NEWPID (0x20000000) used to be here
// and is not expressible through this face any more -- see the note on __ct_ns above.
static_assert(sp::ns::bits(__ct_ns::kinds) == 0x10020000uLL);

// the policy must fit its builder on EVERY arch -- the group sizes differ per ABI, so this is a
// real check and not a formality
static_assert(__ct_seccomp::build(sp::seccomp::act_allow()).count > 0);
static_assert(!__ct_seccomp::build(sp::seccomp::act_allow()).overflowed,
              "the default seccomp_policy capacity no longer holds baseline+io+memory+network on this arch");
static_assert(__ct_seccomp::build(sp::seccomp::act_allow()).valid());

// a policy that does NOT fit must be refused, not truncated. The dangerous truncation is the one
// that stops a slot or two short: the seal still lands, the program is still well formed, and the
// kernel takes it -- so the builder is the only thing that can catch it
namespace
{
template<usize Max>
constexpr bool
__truncation_is_visible(void) noexcept
{
  using tiny = sp::seccomp_policy_n<Max, sp::allow<sp::groups::baseline, sp::groups::io, sp::groups::memory>>;
  constexpr auto fb = tiny::build(sp::seccomp::act_allow());
  return fb.overflowed && !fb.valid();
}

// sweep the residual classes: whichever of these the arch's group sizes land on, all must refuse
static_assert(__truncation_is_visible<16>());
static_assert(__truncation_is_visible<17>());
static_assert(__truncation_is_visible<18>());
static_assert(__truncation_is_visible<19>());
static_assert(__truncation_is_visible<20>());
static_assert(__truncation_is_visible<21>());
static_assert(__truncation_is_visible<64>());
static_assert(__truncation_is_visible<65>());
static_assert(__truncation_is_visible<66>());

// the seal always lands, so a rule can never cost the default action
template<usize Max>
constexpr bool
__always_sealed(void) noexcept
{
  using tiny = sp::seccomp_strict_policy_n<Max, sp::allow<sp::groups::baseline, sp::groups::io>>;
  constexpr auto fb = tiny::build();
  return fb.sealed && fb.count <= Max && (fb.insns[fb.count - 1].code & 0x07u) == micron::bpf::ret;
}

static_assert(__always_sealed<16>() && __always_sealed<17>() && __always_sealed<18>());
static_assert(__always_sealed<19>() && __always_sealed<20>() && __always_sealed<21>());

// the arch gate belongs at [0]. Appended after a rule it is unreachable-or-late, and a filter that
// merely CLAIMS to be gated is the fail-open case
constexpr bool
__gate_must_be_first(void) noexcept
{
  sp::seccomp::filter_builder<64> late;
  late.allow(1);
  late.require_native_arch();
  late.default_kill();
  return late.overflowed && !late.valid() && !late.arch_ok;
}

static_assert(__gate_must_be_first());

// binding a temporary builder to the sandbox left its instruction pointer dangling
template<typename B>
concept __binds = requires(sp::sandbox &box, B &&fb) { box.seccomp(static_cast<B &&>(fb)); };

static_assert(__binds<sp::seccomp::filter_builder<64> &>);
static_assert(!__binds<sp::seccomp::filter_builder<64>>);
};      // namespace

// the three faces must agree at compile time, on every arch
namespace
{
constexpr auto __fp_prog = sp::seccomp::policy<1024>() | sp::seccomp::arch_native()
                           | sp::seccomp::allow_group<sp::groups::baseline, sp::groups::io, sp::groups::memory>()
                           | sp::seccomp::deny_group<sp::groups::network>() | sp::seccomp::deny_all() | sp::seccomp::build();
constexpr auto __tl_prog = __ct_seccomp::build(sp::seccomp::act_errno(static_cast<u16>(micron::error::permissions)));

constexpr bool
__same(void) noexcept
{
  if ( __fp_prog.count != __tl_prog.count ) return false;
  for ( usize i = 0; i < __fp_prog.count; ++i )
    if ( __fp_prog.insns[i].code != __tl_prog.insns[i].code || __fp_prog.insns[i].k != __tl_prog.insns[i].k
         || __fp_prog.insns[i].jt != __tl_prog.insns[i].jt || __fp_prog.insns[i].jf != __tl_prog.insns[i].jf )
      return false;
  return true;
}
};      // namespace

static_assert(__same(), "the functional and type-level seccomp faces diverge on this architecture");

void
__sec_composition_surface(void)
{
  (void)__ct_seccomp::apply();
  (void)__ct_strict::apply();
  (void)__ct_caps::apply_pre_uid();
  (void)__ct_caps::apply_post_uid();
  (void)sp::capability_policy_none::apply();
  (void)sp::capability_policy_keep_all::apply();
  (void)__ct_fs::apply();
  (void)__ct_fs2::apply();
  (void)__ct_ns::apply();
  (void)sp::namespace_policy_none::apply();
  (void)__ct_ll::apply();
  (void)sp::landlock_policy_none::apply();
  (void)__ct_rl::apply();
  (void)sp::rlimit_policy_none::apply();
  (void)sp::filesystem_policy_none::apply();

  sp::seccomp::filter_builder<256> fb;
  fb.require_native_arch().allow(0).default_kill();

  sp::sandbox box;
  box.user()
      .mount_ns()
      .pid_ns()
      .net()
      .uts()
      .ipc()
      .cgroup()
      .root("/j", "/j/old")
      .bind("/usr", "/j/usr", true)
      .tmpfs("/j/tmp")
      .procfs("/j/proc")
      .keep_propagation()
      .landlock("/usr", sp::landlock::read_execute)
      .seccomp(fb)
      .no_new_privs()
      .undumpable()
      .drop_capabilities()
      .as_user(1000, 1000)
      .rlimit(micron::posix::rlimit_nofile, 64, 64)
      .stdio(0, 1, 2)
      .keep_fd(3)
      .close_extra_fds();
  (void)box.kinds();
  (void)box.mount_count();
  (void)box.landlock_rule_count();
  (void)box.has_seccomp();
  (void)box.landlock_handled();
  (void)box.config_fault();
  (void)box.configured();
  (void)box.landlock_handled(sp::landlock::read_only);
  (void)box.landlock_handled_all();
  (void)box.run(nullptr);
  (void)box.spawn("/bin/true", nullptr, nullptr);
  (void)box.run_to_completion(nullptr);
  (void)sp::name_of(sp::stage::seccomp);

  (void)sp::confined(fb, nullptr);
  (void)sp::seccomp::install_notif();
  (void)sp::seccomp::prog_is_arch_gated(fb.prog());
  (void)fb.rule_room();
  (void)sp::with_ruleset(sp::landlock::read_only, [](sp::landlock::ruleset &) { });
}
