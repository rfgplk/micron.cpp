#!/bin/sh
# usage: sh scripts/check_metal_image.sh <image.elf> [i386|amd64|amd64-lm|arm32|arm64|cortexm]
set -eu

img="${1:-}"
arch="${2:-i386}"
[ -n "$img" ] || { echo "usage: $0 <image.elf> [i386|amd64|arm32|arm64]" >&2; exit 2; }
[ -f "$img" ] || { echo "no such image: $img" >&2; exit 2; }

case "$arch" in
arm32|cortexm) OBJDUMP=${OBJDUMP:-/usr/gcc-linaro/bin/arm-none-linux-gnueabihf-objdump}
       READELF=${READELF:-/usr/gcc-linaro/bin/arm-none-linux-gnueabihf-readelf}
       NM=${NM:-/usr/gcc-linaro/bin/arm-none-linux-gnueabihf-nm} ;;
arm64) OBJDUMP=${OBJDUMP:-/usr/gcc-linaro-aarch64/bin/aarch64-none-linux-gnu-objdump}
       READELF=${READELF:-/usr/gcc-linaro-aarch64/bin/aarch64-none-linux-gnu-readelf}
       NM=${NM:-/usr/gcc-linaro-aarch64/bin/aarch64-none-linux-gnu-nm} ;;
*)     OBJDUMP=${OBJDUMP:-objdump} READELF=${READELF:-readelf} NM=${NM:-nm} ;;
esac
command -v "$OBJDUMP" >/dev/null 2>&1 || { echo "  skip  $arch toolchain absent ($OBJDUMP)"; exit 0; }

pass=0
fail=0
ok() { pass=$((pass + 1)); printf '  ok    %s\n' "$1"; }
no() { fail=$((fail + 1)); printf '  FAIL  %s\n' "$1"; }

echo "checking $img ($arch)"

mb32=$("$READELF" -SW "$img" 2>/dev/null | grep -c '\.mb32' || true)
if [ "$mb32" -gt 0 ]; then
  jflags=$("$READELF" -SW "$img" | sed -n 's/^ *\[ *[0-9]*\] \(\.[^ ]*\) *PROGBITS.*/\1/p' | grep -v '^\.mb32$' \
           | while read -r sec; do printf -- '-j %s ' "$sec"; done)
  dis=$("$OBJDUMP" -d $jflags "$img" 2>/dev/null; "$OBJDUMP" -d -m i386 -j .mb32 "$img" 2>/dev/null)
else
  dis=$("$OBJDUMP" -d "$img" 2>/dev/null)
fi

n=$(printf '%s\n' "$dis" \
    | grep -cE '[[:space:]](syscall|sysenter)[[:space:]]|int[[:space:]]+\$0x80|[[:space:]]svc[[:space:]]+(#?0x?0+|#?0)([[:space:]]|$)' || true)
[ "$n" -eq 0 ] && ok "no syscall instruction" || {
  no "$n syscall instructions -- a facet fell through to the linux backend"
  printf '%s\n' "$dis" | grep -E '[[:space:]](syscall|sysenter)[[:space:]]|int[[:space:]]+\$0x80|[[:space:]]svc[[:space:]]+(#?0x?0+|#?0)([[:space:]]|$)' | sed 's/^/          /' | head -8
}

n=$(printf '%s\n' "$dis" | grep -cE '%xmm|%ymm|%zmm|%st\(|%mm[0-7]|[[:space:]][qd][0-9]+,' || true)
[ "$n" -eq 0 ] && ok "no vector or x87 register" || no "$n vector/x87 register uses"

n=$("$READELF" -sW "$img" 2>/dev/null | awk '$4 == "TLS"' | wc -l)
[ "$n" -eq 0 ] && ok "no TLS symbol" || { no "$n TLS symbols -- MICRON_NO_TLS did not reach everything"; "$READELF" -sW "$img" | awk '$4 == "TLS" { print "          " $8 }'; }

u=$("$NM" -u "$img" 2>/dev/null | wc -l)
[ "$u" -eq 0 ] && ok "no undefined symbol" || { no "$u undefined symbols"; "$NM" -u "$img" | sed 's/^/          /'; }

if [ "$arch" = "cortexm" ]; then entsym=Reset_Handler; else entsym=_start; fi
entry=$("$READELF" -hW "$img" | awk '/Entry point address/ { print $NF }')
start=$("$NM" "$img" 2>/dev/null | awk -v s="$entsym" '$3 == s { print "0x"$1 }' | head -1)
if [ -n "$start" ] && [ "$(( $((entry)) & ~1 ))" -eq "$(( $((start)) & ~1 ))" ]; then
  ok "$entsym is the ELF entry ($entry)"
else
  no "ELF entry $entry is not $entsym ($start) -- the board would run the wrong first instruction"
fi

case "$arch" in
i386)
  if "$OBJDUMP" -s -j .text "$img" 2>/dev/null | grep -A1 -m1 '^ *[0-9a-f]* ' | grep -qi '02b0ad1b'; then
    ok "multiboot magic present near the image start"
  else
    no "no multiboot magic in the first words of .text -- qemu -kernel will refuse this image"
  fi
  ;;
amd64-lm)
  if "$READELF" -SW "$img" 2>/dev/null | grep -q '\.note\.Xen'; then
    no "a PVH note is present on the long-mode entry -- PVH hands over in 32-bit mode and this stub starts with movabsq"
  elif "$OBJDUMP" -s "$img" 2>/dev/null | grep -qi '02b0ad1b'; then
    no "a multiboot header is present on the long-mode entry -- multiboot also hands over in 32-bit mode"
  else
    ok "no boot header (--metal-entry lm is entered in long mode by the board's first stage)"
  fi
  ;;
amd64)
  if ! "$READELF" -SW "$img" 2>/dev/null | grep -q '\.note\.Xen .*NOTE'; then
    no ".note.Xen is not a NOTE section -- it was folded into another output section and no PT_NOTE exists"
  elif ! "$READELF" -lW "$img" 2>/dev/null | grep -q '^ *NOTE'; then
    no "no PT_NOTE segment -- the loader has nowhere to look for the PVH note"
  else
    pvh=$("$OBJDUMP" -s -j .note.Xen "$img" 2>/dev/null | sed -n 's/^ *[0-9a-f]* \([0-9a-f ]*\) .*/\1/p' | tr -d ' \n')
    # namesz=4 descsz=4 type=12 "Xen\0" then the entry, all little-endian
    want_hdr='040000000400000012000000586e6500'
    got_hdr=$(printf '%s' "$pvh" | cut -c1-24)
    entry=$("$READELF" -hW "$img" | awk '/Entry point address/ { print $NF }')
    # the descriptor is the 8 hex chars after the 16-byte header, byte-swapped
    d=$(printf '%s' "$pvh" | cut -c33-40)
    dle=$(printf '0x%s%s%s%s' "$(printf '%s' "$d" | cut -c7-8)" "$(printf '%s' "$d" | cut -c5-6)" \
                              "$(printf '%s' "$d" | cut -c3-4)" "$(printf '%s' "$d" | cut -c1-2)")
    if [ "$got_hdr" != "$(printf '%s' "$want_hdr" | cut -c1-24)" ]; then
      no "PVH note header is $got_hdr, want $(printf '%s' "$want_hdr" | cut -c1-24) (namesz/descsz/XEN_ELFNOTE_PHYS32_ENTRY)"
    elif [ "$((dle))" -ne "$((entry))" ]; then
      no "PVH note points at $dle but the ELF entry is $entry -- the loader would jump to the wrong address"
    else
      ok "PVH note present and points at the ELF entry ($dle)"
    fi
  fi
  if "$OBJDUMP" -s "$img" 2>/dev/null | grep -qi '02b0ad1b'; then
    no "a multiboot header is present -- qemu takes the ELF32-only multiboot path on sight of it and refuses a 64-bit image"
  else
    ok "no multiboot header (it would suppress PVH)"
  fi
  ;;
cortexm)
  vaddr=$("$READELF" -SW "$img" 2>/dev/null | sed -n 's/^ *\[ *[0-9]*\] \.isr_vector *PROGBITS *\([0-9a-f]*\).*/\1/p')
  if [ -z "$vaddr" ]; then
    no "no .isr_vector section -- the core has no table to boot from"
  else
    w=$("$OBJDUMP" -s -j .isr_vector "$img" 2>/dev/null | sed -n 's/^ *[0-9a-f]* \([0-9a-f]*\) \([0-9a-f]*\) .*/\1 \2/p' | head -1)
    w0=$(printf '%s' "$w" | cut -d' ' -f1)
    w1=$(printf '%s' "$w" | cut -d' ' -f2)
    # objdump -s prints bytes in file order; these are little-endian words
    swap() { printf '0x%s%s%s%s' "$(printf %s "$1"|cut -c7-8)" "$(printf %s "$1"|cut -c5-6)" \
                                 "$(printf %s "$1"|cut -c3-4)" "$(printf %s "$1"|cut -c1-2)"; }
    sp=$(swap "$w0"); pc=$(swap "$w1")
    if [ "$((0x$vaddr))" -ne "$((0x08000000))" ]; then
      no ".isr_vector is at 0x$vaddr, not at 0x08000000 -- the core reads its table from the start of flash"
    else
      ok ".isr_vector is at the flash base (0x$vaddr)"
    fi
    if [ "$(( $((sp)) >> 24 ))" -eq "$(( 0x20 ))" ]; then
      ok "vector word 0 (initial SP $sp) points into SRAM"
    else
      no "vector word 0 is $sp, which is not in SRAM -- the first push faults"
    fi
    if [ "$(( $((pc)) & 1 ))" -eq 1 ] && [ "$(( $((pc)) & ~1 ))" -eq "$(( $((entry)) & ~1 ))" ]; then
      ok "vector word 1 (reset PC $pc) is the entry, with the Thumb bit set"
    elif [ "$(( $((pc)) & 1 ))" -ne 1 ]; then
      no "vector word 1 is $pc -- bit 0 is CLEAR, which is a HardFault at reset. The reset symbol has neither .thumb_func nor .type %function."
    else
      no "vector word 1 is $pc but the ELF entry is $entry -- they must name the same handler"
    fi
  fi
  ;;
esac

echo "  $pass ok, $fail failed"
[ "$fail" -eq 0 ]
