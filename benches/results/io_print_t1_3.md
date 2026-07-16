# io print buffering — T1.3 before/after (strace write(2) census)

Workload: 1000 println("log line number ", i, " some payload text") to a regular FILE (non-tty).
Fix: __std.hpp __stdout_line_buffered flag (posix::isatty(1)); io.hpp fwrite gates the
     newline-flush on it. TTY -> line-buffered (unchanged); pipe/file -> fully buffered.

                 write(2) calls
BEFORE (always line-buffered)   1000
AFTER  (isatty-gated)             46     (~22x fewer)
output content: byte-identical (diff clean)
tests/printing.cpp + tests/echo.cpp: PASS (v3 -O2); compiletests/io.cpp: PASS (base -O2 SSE2 floor)
