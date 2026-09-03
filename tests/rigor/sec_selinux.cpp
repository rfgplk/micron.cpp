//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// micron::sec::selinux against the live policy.
//
// The parser is the part that can silently be wrong, so it is checked against a CORPUS rather than
// a handful of literals: every context the running policy ships in
// /sys/fs/selinux/initial_contexts, plus the real security.selinux label of every file in /bin.
// For each one the requirement is byte-exact round-tripping and a field split that reassembles
// into the original -- which is what catches the obvious mistake of treating a context as exactly
// four colon-separated fields when an MLS range contains colons of its own
// (unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023 has FIVE).
//
// Nothing here changes any label or any process context: the suite reads.

#include "../../src/std.hpp"

#include "../../src/io/ftw.hpp"
#include "../../src/sec/selinux.hpp"

#include "../snowball/snowball.hpp"

namespace mc = micron;
namespace sl = micron::sec::selinux;

namespace
{

usize corpus_seen = 0;

bool
same(sl::context::part p, const char *lit)
{
  usize n = 0;
  while ( lit[n] ) ++n;
  if ( p.size != n ) return false;
  for ( usize i = 0; i < n; ++i )
    if ( p.data[i] != lit[i] ) return false;
  return true;
}

// reassemble user:role:type[:range] from the parsed fields and require it equals the input
bool
round_trips(const sl::context &c)
{
  if ( !c.valid() ) return false;

  char rebuilt[sl::context::max_len];
  usize n = 0;
  auto put = [&](sl::context::part p) {
    for ( usize i = 0; i < p.size && n + 1 < sizeof(rebuilt); ++i ) rebuilt[n++] = p.data[i];
  };
  put(c.user());
  rebuilt[n++] = ':';
  put(c.role());
  rebuilt[n++] = ':';
  put(c.type());
  if ( c.has_range() ) {
    rebuilt[n++] = ':';
    put(c.range());
  }
  rebuilt[n] = '\0';

  if ( n != c.size() ) return false;
  for ( usize i = 0; i < n; ++i )
    if ( rebuilt[i] != c.str()[i] ) return false;
  return true;
}

// read a file whole into buf, returning the byte count or -errno
max_t
slurp(const char *path, char *buf, usize cap)
{
  const i32 fd = static_cast<i32>(mc::posix::open(path, mc::posix::o_rdonly, 0));
  if ( fd < 0 ) return fd;
  const max_t n = mc::posix::read(fd, buf, cap - 1);
  (void)mc::posix::close(fd);
  if ( n >= 0 ) buf[n] = '\0';
  return n;
}

void
check_corpus_entry(const char *raw, usize len, const char *whence)
{
  sl::context c = sl::context::parse(raw, len);
  if ( !c.valid() ) {
    sb::print("  unparsable context from ", whence, ": ", raw);
    sb::require_true(false);
  }
  if ( !round_trips(c) ) {
    sb::print("  field split does not reassemble: ", c.str(), "  (from ", whence, ")");
    sb::require_true(false);
  }
  // the type field is what a policy decision keys on; it must never come back empty
  sb::require_true(!c.type().empty());
  sb::require_true(!c.user().empty());
  sb::require_true(!c.role().empty());
  ++corpus_seen;
}

};      // namespace

int
main(void)
{
  sb::print("=== SEC SELINUX ===");

  // ---------------------------------------------------------------- //
  sb::test_case("the policy state is read from selinuxfs and matches the raw files");
  {
    const sl::status_t st = sl::status();
    sb::require_true(st.present);
    sb::print("  enforcing=", static_cast<u64>(st.enforcing()), " policyvers=", static_cast<i64>(st.policyvers),
              " mls=", static_cast<u64>(st.mls), " deny_unknown=", static_cast<u64>(st.deny_unknown));

    // cross-check every field against the pseudo-file it came from, read independently
    char buf[64];
    sb::require_true(slurp("/sys/fs/selinux/enforce", buf, sizeof(buf)) > 0);
    sb::require(st.enforcing(), buf[0] == '1');

    sb::require_true(slurp("/sys/fs/selinux/policyvers", buf, sizeof(buf)) > 0);
    i32 vers = 0;
    for ( usize i = 0; buf[i] >= '0' && buf[i] <= '9'; ++i ) vers = vers * 10 + (buf[i] - '0');
    sb::require(st.policyvers, vers);

    sb::require_true(slurp("/sys/fs/selinux/mls", buf, sizeof(buf)) > 0);
    sb::require(st.mls, buf[0] == '1');

    sb::require(sl::present(), true);
    sb::require(sl::enforcing(), st.enforcing());
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("self() is byte-identical to the raw bytes of /proc/self/attr/current");
  {
    char raw[512];
    const max_t n = slurp("/proc/self/attr/current", raw, sizeof(raw));
    sb::require_true(n > 0);

    // the file yields a NUL-terminated string and counts the NUL; the parser must trim it
    usize trimmed = static_cast<usize>(n);
    while ( trimmed > 0 && (raw[trimmed - 1] == '\0' || raw[trimmed - 1] == '\n') ) --trimmed;

    mc::sec::result<sl::context> me = sl::self();
    sb::require_true(me.is_first());      // NOT has_value(): that is true for both branches
    const sl::context c = me.cast<sl::context>();

    sb::require_true(c.valid());
    sb::require(c.size(), trimmed);
    for ( usize i = 0; i < trimmed; ++i ) sb::require(c.str()[i], raw[i]);
    sb::print("  self: ", c.str());
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a context with a colon-bearing MLS range keeps the range whole");
  {
    // THE parse trap: this has five colon-separated pieces, not four. Splitting on every ':'
    // truncates the range and silently changes what the context means
    const sl::context c = sl::context::parse("unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023");
    sb::require_true(c.valid());
    sb::require_true(c.has_range());
    sb::require_true(same(c.user(), "unconfined_u"));
    sb::require_true(same(c.role(), "unconfined_r"));
    sb::require_true(same(c.type(), "unconfined_t"));
    sb::require_true(same(c.range(), "s0-s0:c0.c1023"));
    sb::require_true(round_trips(c));

    // and one with no range at all
    const sl::context nr = sl::context::parse("system_u:object_r:bin_t");
    sb::require_true(nr.valid());
    sb::require_false(nr.has_range());
    sb::require_true(same(nr.type(), "bin_t"));
    sb::require_true(nr.range().empty());
    sb::require_true(round_trips(nr));
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("malformed input is rejected rather than half-parsed");
  {
    sb::require_false(sl::context::parse("").valid());
    sb::require_false(sl::context::parse(nullptr).valid());
    sb::require_false(sl::context::parse("only_one_field").valid());
    sb::require_false(sl::context::parse("two:fields").valid());
    sb::require_true(sl::context::parse("a:b:c").valid());

    // a trailing NUL and newline are trimmed, not carried into the value
    const sl::context t = sl::context::parse("system_u:object_r:bin_t\0", 24);
    sb::require_true(t.valid());
    sb::require(t.size(), usize(23));
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("CORPUS: every initial context the running policy ships round-trips byte-exactly");
  {
    const usize before = corpus_seen;

    mc::io::ftw_files(mc::io::path{ "/sys/fs/selinux/initial_contexts" }, [](const mc::io::path_t &p) {
      char buf[512];
      const max_t n = slurp(p.c_str(), buf, sizeof(buf));
      if ( n <= 0 ) return true;
      check_corpus_entry(buf, static_cast<usize>(n), p.c_str());
      return true;
    });

    sb::print("  initial contexts checked: ", static_cast<u64>(corpus_seen - before));
    sb::require_true(corpus_seen > before);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("CORPUS: the real label of every file in /bin parses and round-trips");
  {
    const usize before = corpus_seen;

    mc::io::ftw_files(mc::io::path{ "/bin" }, [](const mc::io::path_t &p) {
      mc::sec::result<sl::context> lbl = sl::label_of_link(p.c_str());
      if ( !lbl.is_first() ) return true;      // an unlabelled file is legal; skip it
      const sl::context c = lbl.cast<sl::context>();
      if ( c.size() == 0 ) return true;

      check_corpus_entry(c.str(), c.size(), p.c_str());

      // and the porcelain must agree with a direct lgetxattr, minus the counted NUL
      char raw[512];
      const max_t n = mc::posix::lgetxattr(p.c_str(), mc::posix::xattr_name_selinux, raw, sizeof(raw));
      if ( n > 0 ) {
        usize trimmed = static_cast<usize>(n);
        while ( trimmed > 0 && raw[trimmed - 1] == '\0' ) --trimmed;
        sb::require(c.size(), trimmed);
        for ( usize i = 0; i < trimmed; ++i ) sb::require(c.str()[i], raw[i]);
      }
      return true;
    });

    sb::print("  file labels checked: ", static_cast<u64>(corpus_seen - before));
    sb::require_true(corpus_seen - before > 10);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("booleans and policy capabilities read as the raw files do");
  {
    // pick a boolean the running policy actually has, rather than hardcoding a name
    usize checked = 0;

    mc::io::ftw_files(mc::io::path{ "/sys/fs/selinux/booleans" }, [&checked](const mc::io::path_t &p) {
      if ( checked >= 12 ) return true;

      const char *name = p.c_str();
      for ( const char *q = p.c_str(); *q; ++q )
        if ( *q == '/' ) name = q + 1;

      mc::sec::result<bool> b = mc::sec::selinux::boolean(name);
      sb::require_true(b.is_first());

      char buf[32];
      sb::require_true(slurp(p.c_str(), buf, sizeof(buf)) > 0);
      sb::require(b.cast<bool>(), buf[0] == '1');
      ++checked;
      return true;
    });
    sb::print("  booleans checked: ", static_cast<u64>(checked));
    sb::require_true(checked > 0);

    // this box's policy has nnp_nosuid_transition; whatever the answer, the call must succeed
    mc::sec::result<bool> cap = sl::policy_capability("nnp_nosuid_transition");
    sb::require_true(cap.is_first());

    // and an absent name is an error, not a false
    sb::require_true(sl::boolean("mc_sec_definitely_not_a_boolean").is_second());
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("every /proc/self/attr slot is reachable and prev/current agree on shape");
  {
    for ( sl::attr a : { sl::attr::current, sl::attr::exec, sl::attr::fscreate, sl::attr::keycreate,
                         sl::attr::sockcreate, sl::attr::prev } ) {
      mc::sec::result<sl::context> r = sl::get_attr(a);
      // an UNSET slot reads empty, which parses as invalid -- that is a legal answer, not an error
      sb::require_true(r.is_first());
      const sl::context c = r.cast<sl::context>();
      if ( c.valid() ) sb::require_true(round_trips(c));
    }
    sb::require_true(sl::name_of(sl::attr::exec) != nullptr);
    sb::require(sl::get_attr(sl::attr::current).cast<sl::context>(), sl::self().cast<sl::context>());
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("setting an invalid context is refused before it ever reaches the kernel");
  {
    const sl::context junk = sl::context::parse("not_a_context");
    sb::require_false(junk.valid());
    sb::require(sl::set_attr(sl::attr::exec, junk), -22);      // -EINVAL, from our own guard
    sb::require(sl::set_label("/var/tmp", junk), -22);

    // and nothing above changed our own context
    sb::require_true(sl::self().cast<sl::context>().valid());
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a context that does not FIT is flagged, and two that do not fit are not EQUAL");
  {
    // an MLS context with a wide category set runs well past 512 bytes. Clipping it to the first
    // 512 would hand back a well-formed context that is not the one on disk; yielding an empty one
    // and calling it a success is no better, because label_of(a) == label_of(b) then reads TRUE for
    // two labels nobody managed to read
    char big[sl::context::max_len + 64];
    usize i = 0;
    const char *head = "staff_u:staff_r:staff_t:s0-s15:";
    while ( head[i] ) {
      big[i] = head[i];
      ++i;
    }
    while ( i < sizeof(big) - 1 ) big[i++] = 'c';
    big[i] = '\0';

    const sl::context over = sl::context::parse(big, i);
    sb::require_true(over.truncated());
    sb::require_false(over.valid());
    sb::require(over.size(), usize(0));

    // a DIFFERENT over-long label parses to the same empty state, and must still not compare equal
    big[3] = 'X';
    const sl::context other = sl::context::parse(big, i);
    sb::require_true(other.truncated());
    sb::require_false(over == other);
    sb::require_false(over == sl::context{});
    sb::require_false(sl::context{} == sl::context{});

    // the longest label that DOES fit is unaffected
    const usize fits = sl::context::max_len - 1;
    const sl::context edge = sl::context::parse(big, fits);
    sb::require_false(edge.truncated());
    sb::require_true(edge.valid());
    sb::require(edge.size(), fits);
    sb::require_true(edge == sl::context::parse(big, fits));
  }
  sb::end_test_case();

  sb::print("  total corpus entries: ", static_cast<u64>(corpus_seen));
  sb::print("=== SEC SELINUX PASSED ===");
  return 1;
}
