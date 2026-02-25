#!/bin/sh

SRC="./src"
DEFAULT_DEST="/usr/include/micron"

if [ "$#" -ge 1 ]; then
    DEST="$1"
else
    DEST="$DEFAULT_DEST"
fi

if [ ! -d "$DEST" ]; then
    echo "location doesnt exist" >&2
    exit 1
fi

if [ "$(id -u)" -ne 0 ]; then
    echo "Must be root" >&2
    exit 1
fi

find "$SRC" -type f | while IFS= read -r file; do
    base=$(basename "$file")
    case "$base" in
        *.h|*.hh|*.hpp|*.c|*.cc|*.cpp|initializer_list|index_sequence|pthread)
            rel="${file#$SRC/}"
            dest_file="$DEST/$rel"
            dest_dir=$(dirname "$dest_file")
            mkdir -p "$dest_dir" || exit 1
            cp -p "$file" "$dest_file" || exit 1
            chmod 644 "$dest_file" || exit 1
            ;;
    esac
done
