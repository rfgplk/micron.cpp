#!/usr/bin/env python3
# check_syscall_tables.py — verify src/bits/__syscall_codes_<arch>.hpp against the kernel uapi
# headers installed on this box.
#
# Three checks per table:
#   1. alias sync   every `nr` member has a matching SYS_ alias, in the same order, and vice versa.
#                   Needs no kernel header, so it runs everywhere — and on its own it catches the
#                   commonest way these tables rot (a number added to one half and not the other).
#   2. numbers      every entry agrees with the authority header, and the authority holds nothing
#                   the table is missing. The intentional omissions are whitelisted below, each
#                   with the reason it is intentional.
#   3. monotonic    numbers strictly ascending within a table.
#
# Authorities:
#   amd64  /usr/include/asm/unistd_64.h
#   i386   /usr/include/asm/unistd_32.h
#   arm64  /usr/include/asm-generic/unistd.h   (minus the 32-bit-only and __ARCH_WANT_* entries)
#   arm32  the linaro sysroot's asm/unistd-eabi.h for its arch-specific numbering, and
#          asm-generic above that header's high-water mark, where numbering is unified
#
# Usage:  python3 scripts/check_syscall_tables.py [-v]
#   -v  list every entry checked, not just the drift
#
# Exit status: 0 clean, 1 drift found.

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BITS = ROOT / "src/bits"

ASM_64 = Path("/usr/include/asm/unistd_64.h")
ASM_32 = Path("/usr/include/asm/unistd_32.h")
GENERIC = Path("/usr/include/asm-generic/unistd.h")
ARM_EABI = Path("/usr/gcc-linaro/arm-none-linux-gnueabihf/libc/usr/include/asm/unistd-eabi.h")

OMITTED = {
    "amd64": {
        "uselib": "not implemented on amd64",
        "create_module": "removed",
        "get_kernel_syms": "removed",
        "query_module": "removed",
        "getpmsg": "unimplemented",
        "putpmsg": "unimplemented",
        "afs_syscall": "unimplemented",
        "tuxcall": "unimplemented",
        "security": "unimplemented",
        "set_thread_area": "x86-32 only",
        "get_thread_area": "x86-32 only",
        "epoll_ctl_old": "removed",
        "epoll_wait_old": "removed",
        "vserver": "unimplemented",
    },
    "i386": {
        "break": "C++ keyword, cannot be a member name (legacy sys_break)",
    },
    "arm32": {
        "memfd_secret": "not wired on ARM",
        "map_shadow_stack": "x86 only",
    },
    "arm64": {
        "renameat": "__ARCH_WANT_RENAMEAT; arm64 has renameat2 only",
        "sync_file_range2": "__ARCH_WANT_SYNC_FILE_RANGE2; arm64 uses sync_file_range",
        "arch_specific_syscall": "reserved-range placeholder, not a syscall",
    },
}

# names the table legitimately defines that its authority header does not spell
EXTRA = {
    "arm32": {
        # arm's asm/unistd.h, not unistd-eabi.h, carries this one; micron aliases it to
        # arm_sync_file_range (341), which is what the kernel does
        "sync_file_range2",
    },
}

# asm-generic gates these on __BITS_PER_LONG == 32, so a 64-bit arch must not carry them.
# Not `_time64$` — clock_gettime64 / timer_settime64 and friends have no separating underscore
TIME64 = re.compile(r"time64$")


def parse_table(path):
    """-> (entries, aliases, bases). entries is [(name, number)] in file order; aliases is the
    SYS_ names in file order; bases are the __nr_Base-relative anchors, which are not syscalls."""
    entries, aliases, bases = [], [], {}
    for raw in path.read_text().splitlines():
        line = re.sub(r"\s*//.*$", "", raw)
        m = re.match(r"\s*static constexpr __nr_t (\w+) = (.+);\s*$", line)
        if m:
            name, rhs = m.group(1), m.group(2).strip()
            if rhs == "__nr_Base":
                bases[name] = 0
                continue
            if re.fullmatch(r"\w+", rhs):      # a same-number alias of an earlier entry
                prior = dict(entries)
                if rhs not in prior:
                    sys.exit(f"{path.name}: {name} aliases unknown {rhs}")
                entries.append((name, prior[rhs]))
                continue
            b = re.match(r"(\w+) \+ (0x[0-9a-fA-F]+|\d+)$", rhs)
            if not b:
                sys.exit(f"{path.name}: cannot parse RHS {rhs!r} for {name}")
            anchor, off = b.group(1), int(b.group(2), 0)
            if anchor == "__nr_Base" and name.endswith("_base"):
                bases[name] = off
                continue
            entries.append((name, bases.get(anchor, 0) + off))
            continue
        m = re.match(r"inline constexpr __nr_t SYS_(\w+) = \w+::nr::(\w+);\s*$", line)
        if m:
            if m.group(1) != m.group(2):
                sys.exit(f"{path.name}: SYS_{m.group(1)} aliases nr::{m.group(2)}")
            aliases.append(m.group(1))
    return entries, aliases, bases


def parse_uapi(path):
    """-> (required, known) from a kernel unistd header.

    `required` is the numeric defines — a name with a hard number the table must carry. `known` is
    those plus the symbolic ones (`#define __NR_fcntl __NR3264_fcntl`), which the header spells
    that way precisely because they are arch-conditional: whether a given arch gets `fcntl` or
    `fcntl64` out of __NR3264_fcntl depends on __BITS_PER_LONG and the __ARCH_WANT_* set. Resolving
    them would need a real preprocessor, so they are accepted as legitimate names without being
    demanded of the table."""
    required, known = {}, set()
    for line in path.read_text().splitlines():
        m = re.match(r"#define __NR_(\w+)\s+\(?(?:__NR_SYSCALL_BASE \+ )?(\d+)\)?\s*$", line)
        if m and m.group(1) != "syscalls":
            required[m.group(1)] = int(m.group(2))
            known.add(m.group(1))
            continue
        m = re.match(r"#define __NR_(\w+)\s+__NR", line)
        if m:
            known.add(m.group(1))
    return required, known


def authority(arch):
    """-> (required, known, note), or (None, None, path) when the header is unavailable."""
    if arch == "amd64":
        if not ASM_64.exists():
            return None, None, str(ASM_64)
        return (*parse_uapi(ASM_64), str(ASM_64))
    if arch == "i386":
        if not ASM_32.exists():
            return None, None, str(ASM_32)
        return (*parse_uapi(ASM_32), str(ASM_32))
    if arch == "arm64":
        if not GENERIC.exists():
            return None, None, str(GENERIC)
        req, known = parse_uapi(GENERIC)
        # the *time64 block is gated on __BITS_PER_LONG == 32; a 64-bit arch must not carry it
        req = {k: v for k, v in req.items() if not TIME64.search(k)}
        return req, known, f"{GENERIC} (64-bit subset)"
    if arch == "arm32":
        if not ARM_EABI.exists() or not GENERIC.exists():
            return None, None, f"{ARM_EABI} + {GENERIC}"
        req, known = parse_uapi(ARM_EABI)
        # the sysroot header lags; above its high-water mark the numbering is unified, so
        # asm-generic is the authority there
        top = max(req.values())
        greq, gknown = parse_uapi(GENERIC)
        known |= gknown
        for k, v in greq.items():
            if v > top:
                req[k] = v
        return req, known, f"{ARM_EABI} + {GENERIC} above {top}"
    raise AssertionError(arch)


def check(arch, verbose):
    path = BITS / f"__syscall_codes_{arch}.hpp"
    entries, aliases, bases = parse_table(path)
    table = dict(entries)
    fails = []

    # 1. alias sync, order included
    names = [n for n, _ in entries]
    if names != aliases:
        only_nr = [n for n in names if n not in set(aliases)]
        only_al = [n for n in aliases if n not in set(names)]
        for n in only_nr:
            fails.append(f"nr::{n} has no SYS_{n} alias")
        for n in only_al:
            fails.append(f"SYS_{n} has no nr::{n} member")
        if not only_nr and not only_al:
            fails.append("SYS_ alias block is out of order with the nr block")

    # 2. non-decreasing (equal is legal: arm32's sync_file_range2 aliases arm_sync_file_range)
    prev_name, prev = None, -1
    for name, num in entries:
        if num < prev:
            fails.append(f"{name} = {num} is below {prev_name} = {prev}")
        prev_name, prev = name, num

    # 3. numbers vs the authority
    required, known, note = authority(arch)
    if required is None:
        note = f"SKIP numbers - no header at {note}"
    else:
        omitted = OMITTED.get(arch, {})
        allowed = known | EXTRA.get(arch, set())
        private = {n for n, v in table.items() if v >= 0x0F0000}
        for name, num in sorted(required.items(), key=lambda kv: kv[1]):
            if name in table:
                if table[name] != num:
                    fails.append(f"{name} = {table[name]}, kernel says {num}")
            elif name not in omitted:
                fails.append(f"missing {name} = {num}")
        for name, num in entries:
            if name not in allowed and name not in private:
                fails.append(f"{name} = {num} is not in the authority header")

    n = len(entries)
    if fails:
        print(f"FAIL  {arch:6} {n} entries   {note}")
        for f in fails:
            print(f"        {f}")
    else:
        print(f"ok    {arch:6} {n} entries   {note}")
    if verbose:
        for name, num in entries:
            print(f"        {num:7} {name}")
    return len(fails)


def main():
    verbose = "-v" in sys.argv
    bad = sum(check(a, verbose) for a in ("amd64", "i386", "arm32", "arm64"))
    print()
    if bad:
        print(f"{bad} problem(s) found")
        return 1
    print("4 tables, no drift")
    return 0


if __name__ == "__main__":
    sys.exit(main())
