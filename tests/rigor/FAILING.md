# Known failing rigor tests

Baseline as of 2026-09-06, amd64 hosted, **branch `barebones` after the Phase 4 prune**.

    duck test tests/rigor/ --timeout 300 -j 2

## Failing

- `rigor_snowball_fuzz` — needs a c++26 snowball (`'reflect' is not a member of`). **Pre-existing**:
  fails identically at a clean `bdd0f3d` worktree.

## Not failures, but do not be surprised

- `abcmalloc_soak_rand`, `abcmalloc_soak_serial_bulk` — soak tests. Minutes each; `soak_serial_bulk`
  wants >= 16 GB free. A `--timeout 300` reports them as 124 (timeout), which is the timeout doing
  its job, not a defect.
- `robin_exhaustive` — highly sensitive to the hash in use; pathological collisions otherwise.
- Certain heavy abc tests need `vm.overcommit_memory=0|1` or they fail at RUNTIME with
  `critical_error` (mmap refused).

## Removed by Phase 4, not failing

`gl_user_api`, `memcmp` and `memory` were on the previous baseline. `gl_user_api` went with `gfx/`;
`memcmp`/`memory` did not link and are gone with the io/thread-dependent set. The arm32-only entries
(`simd_arith_arm32`, `simd_neon_math_mask`, `simd_shifts_arm32`) survive and are unaffected.
