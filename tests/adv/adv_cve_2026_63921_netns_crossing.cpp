//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// CVE-2026-63921  --  Linux IPv6 VTI, CVSS 8.8, CWE-843 (type confusion)
//
// "IPv6 VTI network-namespace confusion reachable from an unprivileged user namespace could cross
//  namespace or tenant boundaries on container hosts."
//
// THE SHAPE
//
// Its sibling CVE-2026-63917 is about the primitive that makes the bug reachable: a nested user
// namespace, which adv_cve_2026_63917_nested_userns.cpp covers. This one is about the SECOND half,
// which is a separate question with a separate answer: given that a process is in a network
// namespace it has CAP_NET_ADMIN in, what does it need to reach the VTI6 code?
//
// It needs to configure a tunnel device, and the only way to do that is a netlink socket:
//
//     socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)   then RTM_NEWLINK with an ip_vti6 link kind
//
// So the containment question is not "can it create a netns" alone. It is also "can it open an
// AF_NETLINK socket", and those are two different rules on two different syscalls.
//
// MICRON'S ANALOGUE
//
// groups::network (groups.hpp:294-310) grants socket(2) unfiltered:
//
//     struct network {
//       static constexpr i32 calls[] = {
//         SYS_socket, SYS_connect, SYS_accept4, SYS_bind, SYS_listen, ...
//         ^^^^^^^^^^
//
// A group named "network" granting the network syscalls is not obviously wrong -- but socket(2)'s
// first argument is the ADDRESS FAMILY, and AF_NETLINK and AF_PACKET are not networking in the sense
// anybody means when they allowlist a group called network. AF_NETLINK is the kernel's configuration
// interface; AF_PACKET is raw link-layer access. Granting them is granting a different capability
// than the one the group's name describes.
//
// And filtering them is exactly the CVE-2019-10063 problem again: socket(2) is
//
//     SYSCALL_DEFINE3(socket, int, family, int, type, int, protocol)
//                             ^^^^^^^^^^
//
// an `int`, narrowed from the 64-bit register. So `arg_eq(0, AF_NETLINK)` is bypassable with
// AF_NETLINK | (1<<32) and the rule has to be masked. That is not a coincidence -- it is the same
// ABI seam, and it is why this file and adv_cve_2019_10063_tiocsti.cpp both want the same helper.
//
// WHAT THIS PINS
//   1  a filter can express "no AF_NETLINK" and it survives the high-bit decoration
//   2  groups::network does not hand out AF_NETLINK / AF_PACKET unfiltered
//   3  a sandbox with its own netns is the containment for the tunnel half, and it works
//   4  ... and the netns is real: no host interface is visible inside it
//   5  live: under the composed policy, AF_NETLINK is refused
//
// POLARITY: inverted. Contract 2 FAILS on the tree as it stands. Contracts 1, 3, 4 pass and are the
// guards: 1 pins the masked comparator against the bypass, 3-4 pin that box.net() is a real boundary
// so a fix for 2 is not the only thing standing between the sandbox and the kernel.
//
// NEGATIVE CONTROL: contract 1 runs the unmasked rule alongside the masked one on the decorated
// argument and requires them to DISAGREE, in the same run. The bypass is demonstrated, not asserted.
//
// CONTROL (ungated): an ordinary AF_INET/AF_UNIX socket must still be creatable under a policy that
// denies AF_NETLINK. A sandbox that cannot open a TCP connection is not a fix for a tunnel bug.
//
// Build:
//   duck test tests/adv/adv_cve_2026_63921_netns_crossing.cpp -o bin/adv --timeout 120 -f

#include "../../src/std.hpp"

#include "../../src/linux/io/sys.hpp"
#include "../../src/sec/groups.hpp"
#include "../../src/sec/policy.hpp"
#include "../../src/sec/sandbox.hpp"
#include "../../src/sec/seccomp.hpp"

#include "../snowball/snowball.hpp"
#include "../support/adv_kit.hpp"

namespace mc = micron;
namespace sc = micron::sec::seccomp;
namespace g = micron::sec::groups;
namespace s = micron::sec;
namespace ns = micron::sec::ns;

namespace
{

constexpr u16 eperm = static_cast<u16>(mc::error::permissions);
constexpr u64 narrow_mask = 0xFFFF'FFFFull;

// address families. Deliberately spelled out rather than pulled from a header: the point of the
// contract is the NUMBER the kernel reads, and an alias could drift.
constexpr u64 af_unix = 1;
constexpr u64 af_inet = 2;
constexpr u64 af_inet6 = 10;
constexpr u64 af_netlink = 16;
constexpr u64 af_packet = 17;

constexpr u64 sock_raw = 3;
constexpr u64 sock_dgram = 2;
constexpr u64 netlink_route = 0;

constexpr i32 netlink_open = 111;
constexpr i32 host_iface_visible = 112;

}      // namespace

int
main(void)
{
  sb::print("=== ADV CVE-2026-63921 (netlink as the second half of a netns crossing) ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 1  the rule, and its bypass -- side by side
  //
  // socket(2)'s family is an `int`. Whatever the register held above bit 31 is discarded before the
  // kernel dispatches, so a 64-bit compare and the kernel disagree about what "AF_NETLINK" means.

  u32 plain_hi = 0;
  u32 masked_hi = 0;
  {
    sb::test_case("a no-AF_NETLINK rule must survive high-bit decoration of the family argument");
    const u64 netlink_decorated = af_netlink | (u64(1) << 32);

    // the naive spelling
    sc::filter_builder<64> naive;
    naive.require_native_arch();
    naive.deny_if_errno(SYS_socket, sc::arg_eq(0, af_netlink), eperm);
    naive.default_allow();
    sb::require_true(naive.valid());
    sb::require(adv::filter_action(naive, SYS_socket, af_netlink, sock_raw, netlink_route), sc::act_errno(eperm));
    plain_hi = adv::filter_action(naive, SYS_socket, netlink_decorated, sock_raw, netlink_route);

    // the correct one
    sc::filter_builder<64> masked;
    masked.require_native_arch();
    masked.deny_if_errno(SYS_socket, sc::arg_masked(0, narrow_mask, af_netlink), eperm);
    masked.default_allow();
    sb::require_true(masked.valid());
    sb::require(adv::filter_action(masked, SYS_socket, af_netlink, sock_raw, netlink_route), sc::act_errno(eperm));
    masked_hi = adv::filter_action(masked, SYS_socket, netlink_decorated, sock_raw, netlink_route);

    sb::print("  socket(AF_NETLINK,            ...) -> both rules deny");
    sb::print("  socket(AF_NETLINK | 1<<32,    ...) -> unmasked ", plain_hi == sc::act_allow() ? "ALLOWS" : "denies", ", masked ",
              masked_hi == sc::act_allow() ? "ALLOWS" : "denies");

    // NEGATIVE CONTROL: the two must disagree, or neither is observing anything
    sb::require_distinct(plain_hi, masked_hi);
    sb::require(plain_hi, sc::act_allow());            // the bypass, reproduced
    sb::require(masked_hi, sc::act_errno(eperm));      // and closed
  }

  {
    sb::test_case("... and AF_PACKET too, by the same rule shape");
    sc::filter_builder<64> fb;
    fb.require_native_arch();
    fb.deny_if_errno(SYS_socket, sc::arg_masked(0, narrow_mask, af_netlink), eperm);
    fb.deny_if_errno(SYS_socket, sc::arg_masked(0, narrow_mask, af_packet), eperm);
    fb.default_allow();
    sb::require_true(fb.valid());
    for ( u32 sh = 32; sh < 64; ++sh ) {
      sb::require(adv::filter_action(fb, SYS_socket, af_netlink | (u64(1) << sh), sock_raw, 0), sc::act_errno(eperm));
      sb::require(adv::filter_action(fb, SYS_socket, af_packet | (u64(1) << sh), sock_raw, 0), sc::act_errno(eperm));
    }
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 2  the shipped group

  {
    sb::test_case("a network policy must be able to exclude AF_NETLINK / AF_PACKET");

    // THE GROUP ALONE CANNOT DO IT, and saying so is half the contract. groups::network is a flat
    // list of syscall numbers; socket(2)'s family is an ARGUMENT, so no number list can distinguish
    // socket(AF_INET) from socket(AF_NETLINK). This half is required to keep granting -- it pins the
    // limitation permanently rather than letting a reader assume the group's name covers it.
    using group_only = s::seccomp_policy<s::allow<g::baseline, g::network>>;
    auto bare = group_only::build(sc::act_errno(eperm));
    sb::require_true(bare.valid());
    const u32 bare_nl = adv::filter_action(bare, SYS_socket, af_netlink, sock_raw, netlink_route);
    sb::print("  allow<baseline,network> alone       -> AF_NETLINK ",
              bare_nl == sc::act_allow() ? "ALLOW (a number list cannot filter the family argument)" : "denied");

    // and the composed policy, which is what a caller is supposed to write
    using netp = s::seccomp_policy<s::no_raw_socket_families<>, s::allow<g::baseline, g::network>>;
    auto fb = netp::build(sc::act_errno(eperm));
    sb::require_true(fb.valid());

    const u32 nl = adv::filter_action(fb, SYS_socket, af_netlink, sock_raw, netlink_route);
    const u32 pk = adv::filter_action(fb, SYS_socket, af_packet, sock_raw, 0);
    const u32 nl_hi = adv::filter_action(fb, SYS_socket, af_netlink | (u64(1) << 32), sock_raw, netlink_route);
    sb::print("  + no_raw_socket_families<>          -> AF_NETLINK ", nl == sc::act_allow() ? "ALLOW" : "denied", ", AF_PACKET ",
              pk == sc::act_allow() ? "ALLOW" : "denied");

    sb::require_distinct(nl, sc::act_allow());
    sb::require_distinct(pk, sc::act_allow());
    sb::require_distinct(nl_hi, sc::act_allow());      // and the decorated spelling too
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // CONTROL -- ungated
  //
  // The over-correction is obvious and tempting: drop SYS_socket from groups::network. That satisfies
  // contract 2 and makes the group useless.

  {
    sb::test_case("control: ordinary sockets must still be creatable");
    sc::filter_builder<128> fb;
    fb.require_native_arch();
    fb.deny_if_errno(SYS_socket, sc::arg_masked(0, narrow_mask, af_netlink), eperm);
    fb.deny_if_errno(SYS_socket, sc::arg_masked(0, narrow_mask, af_packet), eperm);
    for ( usize i = 0; i < g::network::count; ++i ) fb.allow(g::network::calls[i]);
    fb.default_errno(eperm);
    sb::require_true(fb.valid());

    sb::require(adv::filter_action(fb, SYS_socket, af_inet, sock_dgram, 0), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_socket, af_inet6, sock_dgram, 0), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_socket, af_unix, sock_dgram, 0), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_connect, 3, 0, 0), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_sendto, 3, 0, 0), sc::act_allow());
    sb::require(adv::filter_action(fb, SYS_socketpair, af_unix, sock_dgram, 0), sc::act_allow());
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 3 + 4  the other containment: a network namespace of its own
  //
  // The syscall rule is one layer. The other is that a netns with no interfaces has nothing to
  // confuse -- the VTI6 pair are about state shared BETWEEN namespaces, so a sandbox that never
  // shares one is not exposed regardless of what socket(2) it can open.

  if ( !adv::have_userns() ) {
    sb::test_case("network namespace cases");
    sb::skip("this kernel refuses an unprivileged user namespace, so an unprivileged netns cannot be "
             "created either");
    sb::print("=== ADV CVE-2026-63921 PASSED (netns half skipped) ===");
    return 1;
  }

  {
    sb::test_case("a sandbox with its own netns sees no host interface");
    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::net);
    sb::require_true(box.configured());

    const auto r = box.run_to_completion([]() -> i32 {
      // A fresh netns has exactly one interface: loopback, down. If the host's interfaces are
      // visible we are not in a namespace of our own, and every contract about isolation is void.
      const i32 fd = static_cast<i32>(mc::posix::open("/proc/net/dev", mc::posix::o_rdonly, 0));
      if ( fd < 0 ) return adv::ok_code;      // no procfs here; nothing to read, nothing leaked
      char buf[4096]{};
      const auto n = mc::posix::read(fd, buf, sizeof(buf) - 1);
      (void)mc::posix::close(fd);
      if ( n <= 0 ) return adv::ok_code;

      // count interface lines: the header is two lines, each interface adds one
      usize lines = 0;
      for ( i64 i = 0; i < static_cast<i64>(n); ++i )
        if ( buf[i] == '\n' ) ++lines;
      // 2 header lines + loopback = 3. Anything more is the host's.
      return lines > 3 ? host_iface_visible : adv::ok_code;
    });
    sb::require_true(r.is_first());
    const i32 code = r.cast<s::sandbox::exit_status>().code();
    if ( code == host_iface_visible ) sb::print("  host network interfaces are visible inside box.net()");
    sb::require(code, adv::ok_code);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 5  live

  {
    sb::test_case("live: AF_NETLINK is refused under the composed policy");
    const adv::child_result r = adv::run_child([]() -> i32 {
      sc::filter_builder<512> fb;
      fb.require_native_arch();
      fb.deny_if_errno(SYS_socket, sc::arg_masked(0, narrow_mask, af_netlink), eperm);
      fb.deny_if_errno(SYS_socket, sc::arg_masked(0, narrow_mask, af_packet), eperm);
      for ( usize i = 0; i < g::baseline::count; ++i ) fb.allow(g::baseline::calls[i]);
      for ( usize i = 0; i < g::signal::count; ++i ) fb.allow(g::signal::calls[i]);
      for ( usize i = 0; i < g::network::count; ++i ) fb.allow(g::network::calls[i]);
      fb.default_errno(eperm);
      if ( !fb.valid() ) return adv::setup_failed;
      if ( sc::load(fb, true) < 0 ) return adv::setup_failed;

      // both spellings; the decorated one is the seam
      if ( mc::syscall(SYS_socket, af_netlink, sock_raw, netlink_route) >= 0 ) return netlink_open;
      if ( mc::syscall(SYS_socket, af_netlink | (u64(1) << 32), sock_raw, netlink_route) >= 0 ) return netlink_open;
      if ( mc::syscall(SYS_socket, af_packet, sock_raw, 0) >= 0 ) return netlink_open;

      // and the control, in the same child: an ordinary socket must still open
      const long ok = mc::syscall(SYS_socket, af_inet, sock_dgram, 0);
      if ( ok < 0 ) return adv::bad_code;
      (void)mc::posix::close(static_cast<i32>(ok));
      return adv::ok_code;
    });
    if ( r.code == netlink_open ) sb::print("  a netlink or packet socket opened under the policy");
    sb::require_true(r.ok());
  }

  sb::print("=== ADV CVE-2026-63921 PASSED ===");
  return 1;
}
