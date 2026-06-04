#!/usr/bin/env bash
#  Copyright (c) 2024- David Lucius Severus
#  Distributed under the Boost Software License, Version 1.0.
#
# check_simd_intrinsics.sh [--arch amd64|arm64|arm32|all] [SRC.cpp]
#
# Compile-probe the micron SIMD layer across amd64 / arm64 / arm32 by building a
# translation unit with the real `duck` toolchains. A successful build proves
# every SIMD intrinsic that TU references is available for that target -- which
# is far more reliable than grepping src/simd/__bits, since the intrinsics are
# macro-generated (e.g. `__neon_ldst(uint32x4_t, u32, ...)` -> vld1q_u32).
#
# Default SRC is scripts/simd_intrinsics_probe.cpp (probes the reduce.hpp set).
# Pass any other .cpp to check its SIMD usage cross-arch.
#
# Toolchains (see ~/.claude memory aarch64-build-run):
#   amd64 : `duck build <src>`
#   arm64 : `bin/main build <src> --arm64`   (patched duck; falls back to duck)
#   arm32 : `bin/main build <src> --arm`     (falls back to duck)
set -u

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$here/.." && pwd)"

arch="all"
src="$here/simd_intrinsics_probe.cpp"
while [ $# -gt 0 ]; do
  case "$1" in
    --arch) arch="$2"; shift 2 ;;
    --arch=*) arch="${1#*=}"; shift ;;
    -h|--help) sed -n '3,18p' "$0"; exit 0 ;;
    *) src="$1"; shift ;;
  esac
done

cd "$root"
host_duck="${DUCK:-duck}"
cross_duck="$root/bin/main"
[ -x "$cross_duck" ] || cross_duck="$host_duck"

fail=0
want() { [ "$arch" = "all" ] || [ "$arch" = "$1" ]; }

run() { # label, cmd...
  local label="$1"; shift
  printf '== %-6s : ' "$label"
  local out
  if out="$("$@" 2>&1)"; then
    echo "PASS"
  else
    echo "FAIL"
    printf '%s\n' "$out" | sed 's/^/      /'
    fail=1
  fi
}

echo "probe src: $src"
echo "----------------------------------------------------------------"
want amd64 && run amd64 $host_duck build "$src"
want arm64 && run arm64 $cross_duck build "$src" --arm64
want arm32 && run arm32 $cross_duck build "$src" --arm
echo "----------------------------------------------------------------"
if [ $fail -eq 0 ]; then echo "ALL SIMD INTRINSIC PROBES PASSED"; else echo "SOME SIMD INTRINSIC PROBES FAILED"; fi
exit $fail
