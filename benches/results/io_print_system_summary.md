# io print-as-a-system — 20000 mixed lines -> /dev/null (non-tty)

mode       write(2)   ns/line   note
println      963        69      buffered global stdout; T1.3 batches when non-tty
echofd     20001       579      echo(fd) fd_sink flushes per CALL (no cross-call buffer)
rawline    20001       575      one posix::write per line (worst case)
rawbatch     195        31      hand-batched 4096 buffer (reference floor)

T1.3 effect on println: was 20000 write(2) (1/line) before the isatty gate; now 963 (~21x fewer),
and 69 ns/line vs 575 for the per-line paths (~8x faster). Finding: fd_sink (echo-to-fd) does not
batch across calls -> the global stdout buffer is the efficient high-volume path post-T1.3.
