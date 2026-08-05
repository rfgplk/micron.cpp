# Known failing rigor tests

Baseline as of 2026-08-04, amd64 hosted 
duck test tests/rigor/ --timeout 300

- gl_user_api
- memcmp
- memory

# ARM32 only tests

- simd_arith_arm32
- simd_neon_math_mask
- simd_shifts_arm32

# Special mentions

- rigor_snowball_fuzz (requires cpp26 snowball)
- robin_exhaustive (HIGHLY dependent on the underlying hash used -- pathological hash collisions occur otherwise)
- abcmalloc_soak (takes a long time depending on the machine; 3 to 8 minutes)
- abcmalloc_soak_serial_bulk (requires at least 16GB of free mem, otherwise OOMs)
