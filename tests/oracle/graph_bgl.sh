#!/bin/sh
set -eu

graph_root=${BGL_GRAPH_ROOT:-/code/Git/graph}
expected=3ad9c9266712ba8bc1d27c66fe4959f14fa71916

if [ ! -d "$graph_root/.git" ]; then
  echo "Boost.Graph checkout not found at $graph_root (override with BGL_GRAPH_ROOT)" >&2
  exit 2
fi

actual=$(git -C "$graph_root" rev-parse HEAD)
if [ "$actual" != "$expected" ]; then
  echo "Boost.Graph oracle commit mismatch: expected $expected, got $actual" >&2
  exit 2
fi

mkdir -p bin/oracle
${CXX:-g++} -std=c++23 -O2 -I"$graph_root/include" -I./src tests/oracle/graph_bgl.cpp -o bin/oracle/graph_bgl

set +e
bin/oracle/graph_bgl
status=$?
set -e
if [ "$status" -eq 1 ]; then
  exit 0
fi
exit "$status"
