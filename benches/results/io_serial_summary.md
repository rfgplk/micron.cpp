# io_serial (MFR1) — T1.1 before/after

Metric: x-memcpy ratio within each run (robust to run-to-run noise). Lower is better.
Fix: __serial_core.hpp __put_bytes/__get_bytes byte-loop -> micron::bytecpy;
     unframe blit_container per-element push_back -> single bulk copy (via __get_bytes) + set_size.
Correctness: tests/mfr.cpp ALL PASS on --isa v3 and --isa base (SSE2 floor).

op            elems   BEFORE   AFTER
frame_into    256     3.06x    1.33x
frame_into    65536   1.29x    1.18x
unframe_from  256    30.13x    4.06x   (7.4x faster)
unframe_from  4096   27.08x    3.88x   (7.0x faster)
unframe_from  65536  13.91x    9.91x   (memory-bound)

vector<f64> unframe_from 256: 17.45x -> 3.81x; 4096: 6.10x -> 2.85x.
