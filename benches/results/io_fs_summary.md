# io filesystem cache — T1.2 before/after (wall-ns/op, median-of-5, tmpfs)

Fix: basic_filesystem key sstr<4096> (io::path_t) -> micron::string (heap-backed);
LRU recency-vector shifts use micron::move (O(1) pointer-steal, was 4096-byte copy).
Correctness: tests/fsys.cpp + tests/files.cpp PASS; compiletests/io.cpp compiles.

op                BEFORE      AFTER    speedup
lru cap64_hit64   14987 ns    748 ns   20.0x   (was 27.8x a plain hit -> 1.4x)
cache miss_open    9701 ns   4035 ns    2.4x   (no 4096 B key insert/rehash)
lru cap8_churn64   7466 ns   5030 ns    1.5x
cache hit_cached    539 ns    526 ns    ~1.0x

Query family (exists/is_regular/file_size) already ~0.40x open+fstat+close (1 syscall,
0 fd) — no change needed. rooted_filesystem still uses path_t keys (follow-up).
