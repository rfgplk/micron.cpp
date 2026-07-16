# io echo/format engine — T4 coalescing before/after (ns/op, lower better)

Fix: echo.hpp __put_fill_run (batch padding fill <=64B/put) + format_to_sink literal-run
coalescing (one s.put per run of non-brace bytes). Output byte-identical (tests/echo.cpp PASS).

case             BEFORE   AFTER   speedup
literal-heavy      145      81      1.8x
pad{:>40}          224      48      4.7x

Baseline finding (unchanged, informational): container/map formatters ~58-77x raw (per-element
scalar conversion), Ryu float ~16x — inherent formatter cost, not coalescing-addressable.
