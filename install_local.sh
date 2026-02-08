#!/bin/bash

SRC="./src" 
DEST="/usr/include/micron"


# Requires root permissions
if [ "$(id -u)" -ne 0 ]; then
    echo "Must be root" >&2
    exit 1
fi

mkdir -p /usr/include/micron


find "$SRC" -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.c' -o -name '*.cpp' -o -name 'initializer_list' -o -name 'index_sequence'  \) -print0 |
while IFS= read -r -d '' file; do
    rel_path="${file#$SRC/}"
    dest_path="$DEST/$rel_path"
    mkdir -p "$(dirname "$dest_path")"
    cp -f "$file" "$dest_path"
    chmod 644 "$dest_path"
done
