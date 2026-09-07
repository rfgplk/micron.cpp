#!/bin/sh
#  Copyright (c) 2026 David Lucius Severus
#
#  Distributed under the Boost Software License, Version 1.0.
#  See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt

set -e
ko="${1:-micron_demo.ko}"

if [ ! -f "$ko" ]; then
  echo "check_sections: no $ko -- run make first" >&2
  exit 2
fi

dups=$(readelf -SW "$ko" | awk '
  /^ *\[ *[0-9]+\]/ {
    line = $0
    sub(/^ *\[ *[0-9]+\] */, "", line)
    n = split(line, f, /[ \t]+/)
    name = f[1]; size = f[5]; flags = ""
    for (i = 6; i <= n; i++) if (f[i] ~ /^[WAXMSILOGTpxoE]+$/) { flags = f[i]; break }
    if (name != "" && flags ~ /A/ && size ~ /[1-9a-f]/) print name
  }' | sort | uniq -d)

if [ -n "$dups" ]; then
  echo "FAIL  duplicate SHF_ALLOC section names -- insmod will report 'File exists':"
  echo "$dups" | sed 's/^/        /'
  exit 1
fi

n=$(readelf -SW "$ko" | grep '__patchable_function_entries' | grep -vc '\.rela' || true)
echo "ok    no duplicate loadable section names in $ko"
echo "      __patchable_function_entries sections: $n (a stock module has 1; this had 55)"
exit 0
