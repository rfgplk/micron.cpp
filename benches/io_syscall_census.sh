#!/bin/sh
set -u
N="${1:-100000}"
OUT="benches/results"
mkdir -p "$OUT"
PB="bin/io_print_system_bench/io_print_system_bench"

echo "=== io print system: write(2) count per mode, $N lines -> /dev/null (non-tty) ==="
if [ -x "$PB" ]; then
  for m in println echofd rawline rawbatch; do
    log="$OUT/io_syscalls_print_${m}.txt"
    strace -f -e trace=write -c -o "$log" sh -c "$PB --mode=$m --n=$N >/dev/null" 2>/dev/null
    calls=$(awk '/ write$/{print $4}' "$log")
    printf "  %-9s write(2)=%s\n" "$m" "${calls:-?}"
  done
else
  echo "  (build first: ./bin/duck build benches/io_print_system_bench.cpp -O3 --isa v3 -o bin/io_print_system_bench)"
fi

# optional: file / fs benches, if built — full per-syscall trace for inspection
for name in io_file_bench io_fs_bench; do
  bin="bin/$name/$name"
  if [ -x "$bin" ]; then
    echo "=== $name: full syscall census ==="
    strace -f -e trace=openat,read,write,pread64,pwrite64,fstat,newfstatat,statx,lseek,fcntl,fsync,fdatasync,close \
      -c -o "$OUT/io_syscalls_${name}.txt" "$bin" >/dev/null 2>&1
    grep -E ' (openat|fstat|newfstatat|statx|pread64|pwrite64|lseek|fcntl|write|read)$' "$OUT/io_syscalls_${name}.txt" | head -20
  fi
done
echo "done -> $OUT/io_syscalls_*.txt"
