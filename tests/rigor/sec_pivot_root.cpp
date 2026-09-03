//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/std.hpp"

#include "../../src/sec/policy.hpp"
#include "../../src/sec/sandbox.hpp"

#include "../snowball/snowball.hpp"

namespace mc = micron;
namespace s = micron::sec;

namespace
{

constexpr const char *jail = "/var/tmp/mc_sec_jail";
constexpr const char *jail_old = "/var/tmp/mc_sec_jail/old";
constexpr const char *jail_marker = "/var/tmp/mc_sec_jail/inside.txt";
constexpr const char *host_marker = "/var/tmp/mc_sec_jail_host.txt";

// the recursive read-only bind case: src holds a submount, dst gets the whole tree
constexpr const char *ro_src = "/var/tmp/mc_sec_ro_src";
constexpr const char *ro_src_sub = "/var/tmp/mc_sec_ro_src/sub";
constexpr const char *ro_dst = "/var/tmp/mc_sec_ro_dst";
constexpr const char *ro_dst_file = "/var/tmp/mc_sec_ro_dst/top.txt";
constexpr const char *ro_dst_sub_file = "/var/tmp/mc_sec_ro_dst/sub/under.txt";

constexpr i32 bit_inside_ok = 1 << 0;
constexpr i32 bit_old_dir_gone = 1 << 1;
constexpr i32 bit_old_marker_gone = 1 << 2;
constexpr i32 bit_host_etc_gone = 1 << 3;
constexpr i32 bit_old_dotdot_gone = 1 << 4;
constexpr i32 bit_not_a_mount = 1 << 5;

bool
write_file(const char *path, const char *what)
{
  const i32 fd = static_cast<i32>(mc::posix::open(path, mc::posix::o_wronly | mc::posix::o_create | mc::posix::o_trunc, 0644));
  if ( fd < 0 ) return false;
  usize n = 0;
  while ( what[n] ) ++n;
  const bool ok = mc::posix::write(fd, what, n) == static_cast<max_t>(n);
  (void)mc::posix::close(fd);
  return ok;
}

bool
can_open(const char *path)
{
  const i32 fd = static_cast<i32>(mc::posix::open(path, mc::posix::o_rdonly, 0));
  if ( fd < 0 ) return false;
  (void)mc::posix::close(fd);
  return true;
}

bool
can_open_dir(const char *path)
{
  const i32 fd = static_cast<i32>(mc::posix::open(path, mc::posix::o_rdonly | mc::posix::o_directory, 0));
  if ( fd < 0 ) return false;
  (void)mc::posix::close(fd);
  return true;
}

bool
can_create(const char *path)
{
  const i32 fd = static_cast<i32>(mc::posix::open(path, mc::posix::o_wronly | mc::posix::o_create, 0644));
  if ( fd < 0 ) return false;
  (void)mc::posix::close(fd);
  (void)mc::posix::unlink(path);
  return true;
}

i32
escape_probe(void)
{
  i32 bits = 0;

  if ( can_open("/inside.txt") ) bits |= bit_inside_ok;

  if ( !can_open("/old/var/tmp") && !can_open_dir("/old/etc") ) bits |= bit_old_dir_gone;
  if ( !can_open("/old/var/tmp/mc_sec_jail_host.txt") ) bits |= bit_old_marker_gone;
  if ( !can_open("/etc/hostname") && !can_open("/old/etc/hostname") ) bits |= bit_host_etc_gone;

  mc::posix::stat_t root_st{};
  const bool have_root = mc::posix::stat("/", root_st) == 0;

  {
    mc::posix::stat_t up_st{};
    if ( have_root && mc::posix::stat("/old/..", up_st) == 0 ) {
      if ( up_st.st_dev == root_st.st_dev && up_st.st_ino == root_st.st_ino ) bits |= bit_old_dotdot_gone;
    } else if ( have_root ) {
      bits |= bit_old_dotdot_gone;
    }
  }

  {
    mc::posix::stat_t old_st{};
    if ( have_root && mc::posix::stat("/old", old_st) == 0 ) {
      if ( root_st.st_dev == old_st.st_dev ) bits |= bit_not_a_mount;
    } else {
      bits |= bit_not_a_mount;
    }
  }

  return bits;
}

i32
trivial_body(void)
{
  return 7;
}

i32
wait_status_of(s::sandbox::child &c)
{
  int status = 0;
  (void)mc::waitpid(c.pid, &status, 0);
  return status;
}

constexpr bool
same(const char *a, const char *b) noexcept
{
  if ( a == nullptr || b == nullptr ) return a == b;
  usize i = 0;
  while ( a[i] != '\0' && a[i] == b[i] ) ++i;
  return a[i] == '\0' && b[i] == '\0';
}

using s::__impl::__rel_put_old;

static_assert(same(__rel_put_old("/j", "/j/old"), "/old"));
static_assert(same(__rel_put_old("/j/", "/j/old"), "/old"));
static_assert(same(__rel_put_old("/j//", "/j/old"), "/old"));
static_assert(same(__rel_put_old("/var/tmp/jail", "/var/tmp/jail/old"), "/old"));
static_assert(same(__rel_put_old("/j", "/j/a/b/c"), "/a/b/c"));
static_assert(same(__rel_put_old("/", "/old"), "/old"));


static_assert(__rel_put_old("/jail", "/jailhouse/old") == nullptr);
static_assert(__rel_put_old("/jail", "/other/old") == nullptr);
static_assert(__rel_put_old("/jail", "/jail") == nullptr);      
static_assert(__rel_put_old("/jail/deep", "/jail") == nullptr);
static_assert(__rel_put_old(nullptr, "/old") == nullptr);
static_assert(__rel_put_old("/jail", nullptr) == nullptr);
static_assert(__rel_put_old("", "/old") == nullptr);

};      // namespace

int
main(void)
{
  sb::print("=== SEC PIVOT ROOT ===");

  (void)mc::posix::mkdir(jail, 0755);
  (void)mc::posix::mkdir(jail_old, 0755);
  sb::require_true(write_file(jail_marker, "inside the jail"));
  sb::require_true(write_file(host_marker, "the host filesystem"));

  sb::require_true(can_open(host_marker));

  sb::test_case("after pivot_root the old root is GONE, by every route back to it");
  {

    s::sandbox box;
    box.user().mount_ns().bind(jail, jail).root(jail, jail_old);

    sb::require_true(box.configured());
    s::sandbox::child c = box.run(escape_probe);
    if ( !c.ok() ) sb::print("  faulted at stage: ", c.fault.stage_name(), " errno ", static_cast<i64>(-c.fault.err));
    sb::require_true(c.ok());

    const i32 st = wait_status_of(c);
    sb::require_true(mc::wifexited(st));

    const i32 want = bit_inside_ok | bit_old_dir_gone | bit_old_marker_gone | bit_host_etc_gone | bit_old_dotdot_gone | bit_not_a_mount;
    const i32 got = mc::wexitstatus(st);
    if ( got != want )
      sb::print("  escape bits got=", static_cast<i64>(got), " want=", static_cast<i64>(want),
                "  STILL REACHABLE=", static_cast<i64>(want & ~got));
    sb::require(got, want);
  }
  sb::end_test_case();

  sb::test_case("a read-only bind inside the jail cannot be walked around through the old root");
  {

    s::sandbox box;
    box.user().mount_ns().bind(jail, jail).bind("/usr", "/var/tmp/mc_sec_jail/usr", true).root(jail, jail_old);

    (void)mc::posix::mkdir("/var/tmp/mc_sec_jail/usr", 0755);

    s::sandbox::child c = box.run([]() -> i32 {
      const bool ro_visible = can_open_dir("/usr");

      const bool leak = can_open_dir("/old/usr") || can_open_dir("/old/var");
      return (ro_visible && !leak) ? 31 : 32;
    });
    if ( !c.ok() ) sb::print("  faulted at stage: ", c.fault.stage_name(), " errno ", static_cast<i64>(-c.fault.err));
    sb::require_true(c.ok());
    sb::require(mc::wexitstatus(wait_status_of(c)), 31);
  }
  sb::end_test_case();

  sb::test_case("a RECURSIVE read-only bind seals the SUBMOUNTS, not only the mount it names");
  {
    // MS_REC is ignored by a MS_REMOUNT|MS_BIND remount -- the kernel applies the new flags to the
    // one mount the path names and to nothing under it. So "mount rec, then remount rdonly" seals
    // the top of the tree and leaves every submount fully writable while the API says read-only.
    // The tmpfs below is that submount, and it is mounted from inside the sandbox's own mount ns
    (void)mc::posix::mkdir(ro_src, 0755);
    (void)mc::posix::mkdir(ro_src_sub, 0755);
    (void)mc::posix::mkdir(ro_dst, 0755);

    s::sandbox box;
    box.user().mount_ns().tmpfs(ro_src_sub, "size=1m").bind(ro_src, ro_dst, true);

    s::sandbox::child c = box.run([]() -> i32 {
      const bool top_ro = !can_create(ro_dst_file);
      const bool sub_ro = !can_create(ro_dst_sub_file);
      return top_ro ? (sub_ro ? 71 : 72) : 73;
    });
    if ( !c.ok() ) sb::print("  faulted at stage: ", c.fault.stage_name(), " errno ", static_cast<i64>(-c.fault.err));
    sb::require_true(c.ok());
    const i32 got = mc::wexitstatus(wait_status_of(c));
    if ( got == 72 ) sb::print("  the top mount is read-only but the SUBMOUNT under it is writable");
    sb::require(got, 71);
  }
  sb::end_test_case();

  sb::test_case("a non-recursive read-only bind still works, for kernels without mount_setattr");
  {
    (void)mc::posix::mkdir(ro_src, 0755);
    (void)mc::posix::mkdir(ro_dst, 0755);

    s::sandbox box;
    box.user().mount_ns().bind(ro_src, ro_dst, true, false);
    s::sandbox::child c = box.run([]() -> i32 { return can_create(ro_dst_file) ? 74 : 75; });
    if ( !c.ok() ) sb::print("  faulted at stage: ", c.fault.stage_name(), " errno ", static_cast<i64>(-c.fault.err));
    sb::require_true(c.ok());
    sb::require(mc::wexitstatus(wait_status_of(c)), 75);
  }
  sb::end_test_case();

  sb::test_case("a put_old outside the new root is refused before pivot_root is ever called");
  {

    s::sandbox box;
    box.user().mount_ns().bind(jail, jail).root(jail, "/var/tmp/somewhere_else/old");

    s::sandbox::child c = box.run(trivial_body);
    sb::require_false(c.ok());
    sb::require(static_cast<i32>(c.fault.where), static_cast<i32>(s::stage::filesystem));
    sb::require(c.fault.err, -static_cast<i32>(mc::error::invalid_arg));
  }
  sb::end_test_case();

  sb::test_case("a new_root that is not a mount point still faults at the filesystem stage");
  {

    s::sandbox box;
    box.user().mount_ns().root(jail, jail_old);
    s::sandbox::child c = box.run(trivial_body);
    sb::require_false(c.ok());
    sb::require(static_cast<i32>(c.fault.where), static_cast<i32>(s::stage::filesystem));
    sb::require_true(c.fault.err < 0);
  }
  sb::end_test_case();

  sb::test_case("chroot_to still works and still says what it does not do");
  {

    s::sandbox box;
    box.user().mount_ns().chroot_to(jail);
    s::sandbox::child c = box.run([]() -> i32 { return can_open("/inside.txt") ? 41 : 42; });
    if ( !c.ok() ) sb::print("  faulted at stage: ", c.fault.stage_name(), " errno ", static_cast<i64>(-c.fault.err));
    sb::require_true(c.ok());
    sb::require(mc::wexitstatus(wait_status_of(c)), 41);
  }
  sb::end_test_case();

  sb::test_case("the sec::pivot_to policy detaches too, and pivot_to_keep_old is the one that does not");
  {

    using detaching = s::filesystem_policy<s::make_private, s::bind<"/var/tmp/mc_sec_jail", "/var/tmp/mc_sec_jail">,
                                           s::pivot_to<"/var/tmp/mc_sec_jail", "/var/tmp/mc_sec_jail/old">>;

    s::sandbox box;
    // NOTE (CVE audit): .drop_capabilities(false) is REQUIRED here now, and the reason is the point
    // of the change rather than an inconvenience. sandbox drops capabilities by default, at stages 9
    // and 12 -- both BEFORE the body runs -- so a body that mounts has no CAP_SYS_ADMIN to mount
    // with. The sandbox's OWN filesystem stage (box.bind/.root) runs at stage 3 and is unaffected;
    // what needs the opt-out is this pattern, where a filesystem_policy is applied from INSIDE the
    // body. That is exactly the case where the caller has said "I shape mounts myself".
    box.user().mount_ns().keep_propagation().drop_capabilities(false);
    s::sandbox::child c = box.run([]() -> i32 {
      if ( detaching::apply() < 0 ) return 51;
      if ( !can_open("/inside.txt") ) return 52;
      if ( can_open_dir("/old/var") ) return 53;
      if ( can_open("/old/var/tmp/mc_sec_jail_host.txt") ) return 54;
      return 55;
    });
    if ( !c.ok() ) sb::print("  faulted at stage: ", c.fault.stage_name(), " errno ", static_cast<i64>(-c.fault.err));
    sb::require_true(c.ok());
    sb::require(mc::wexitstatus(wait_status_of(c)), 55);

    using keeping = s::filesystem_policy<s::make_private, s::bind<"/var/tmp/mc_sec_jail", "/var/tmp/mc_sec_jail">,
                                         s::pivot_to_keep_old<"/var/tmp/mc_sec_jail", "/var/tmp/mc_sec_jail/old">>;
    s::sandbox box2;
    box2.user().mount_ns().keep_propagation().drop_capabilities(false);      // same reason as above
    s::sandbox::child c2 = box2.run([]() -> i32 {
      if ( keeping::apply() < 0 ) return 61;

      return can_open_dir("/old/var") ? 62 : 63;
    });
    sb::require_true(c2.ok());
    sb::require(mc::wexitstatus(wait_status_of(c2)), 62);
  }
  sb::end_test_case();

  sb::test_case("the runner's own filesystem is exactly as it was");
  {
    sb::require_true(can_open(host_marker));
    sb::require_true(can_open(jail_marker));
    sb::require_true(can_open_dir("/etc"));
    sb::require_true(can_open_dir(jail_old));
    sb::require_true(mc::posix::geteuid() != 0);
  }
  sb::end_test_case();

  sb::print("=== SEC PIVOT ROOT PASSED ===");
  return 1;
}
