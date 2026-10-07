#!/usr/bin/env python3
# Copyright (c) 2026 David Lucius Severus
# Distributed under the Boost Software License, Version 1.0.
"""Build, pin and retain five repetitions of the portable compact GEMM baseline."""
import hashlib
import json
import math
import os
from pathlib import Path
import statistics
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'bin/compact/bench'
OUT.mkdir(parents=True, exist_ok=True)
BINARY = OUT / 'compact_bench'
BUILD = ['duck', 'build', 'benches/compact_bench.cpp', '-i', str(ROOT), '--std', 'c++23', '-O3', '--no-lto', '--perf',
         '--def', 'MICRON_ABC_STATS=1', '--def', 'MICRON_ABC_ZERO_ON_ALLOC=true', '--def', 'MICRON_ABC_ENFORCE_PROVENANCE=true',
         '-o', str(OUT)]

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

sources = [ROOT / p for p in ['src/math/float16.hpp', 'src/math/blas/mixed.hpp', 'src/math/matrix/pack.hpp',
                              'benches/compact_bench.cpp', 'benches/compact.py']]
result = {'build': BUILD, 'source_sha256': {str(p.relative_to(ROOT)): digest(p) for p in sources},
          'micron_base': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
          'compiler': subprocess.check_output(['g++', '--version'], text=True),
          'cpu_model': next(s for s in Path('/proc/cpuinfo').read_text().splitlines() if s.startswith('model name')),
          'cpu': min(os.sched_getaffinity(0)), 'milliseconds_per_case': 100, 'runs': []}
with (OUT / 'build.log').open('w') as stream:
    subprocess.run(BUILD, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT, check=True)
result['binary_sha256'] = digest(BINARY)
for repeat in range(5):
    command = ['taskset', '-c', str(result['cpu']), str(BINARY), '100']
    def competing():
        return [s for s in subprocess.check_output(['ps', '-eo', 'pid,comm'], text=True).splitlines()
                if any(x in s for x in ['cc1plus', 'clang', 'qemu'])]
    before = competing()
    run = subprocess.run(command, capture_output=True, text=True, timeout=90)
    after = competing()
    assert run.returncode == 0, run.stdout + run.stderr
    samples = []
    for line in run.stdout.splitlines():
        row = dict(field.split('=', 1) for field in line.split())
        for field in ['rows', 'calls', 'setup_ns', 'storage_bytes', 'allocs', 'frees']:
            row[field] = int(row[field])
        for field in ['ns_per_call', 'checksum']:
            row[field] = float(row[field])
            assert math.isfinite(row[field])
        assert row['allocs'] == row['frees'] == 0 and row['calls'] > 0
        samples.append(row)
    assert len(samples) == 15
    result['runs'].append({'repetition': repeat, 'command': command, 'time_unix': time.time(),
                           'competing_before': before, 'competing_after': after,
                           'stdout': run.stdout, 'stderr': run.stderr, 'samples': samples})
    print('PASS repetition', repeat + 1, flush=True)
result['summary'] = []
for rows in [1, 32, 512]:
    for kind in ['f32', 'f16', 'bf16', 'i8-i32', 'i8-i64']:
        samples = [s for r in result['runs'] for s in r['samples'] if s['rows'] == rows and s['type'] == kind]
        result['summary'].append({'rows': rows, 'type': kind, 'median_ns': statistics.median(s['ns_per_call'] for s in samples),
                                  'min_ns': min(s['ns_per_call'] for s in samples), 'max_ns': max(s['ns_per_call'] for s in samples),
                                  'weight_bytes': samples[0]['storage_bytes']})
assert result['source_sha256'] == {str(p.relative_to(ROOT)): digest(p) for p in sources}
result['limitations'] = ['Physical amd64 shared desktop; single pinned core, no perf counters or isolation claim.',
                        'Integer workload has different numerical values and outputs; it is not a model-quality comparison.',
                        'Half storage uses generic mixed arithmetic; f32 uses existing prepared GEMM.',
                        'Storage bytes count weight payload only; floating setup also constructs prepared panels.',
                        'ARM emulation is correctness evidence only; RAM model quality and inference measurement follow in Pond #33-35.']
path = ROOT / 'benches/results/compact.json'
path.parent.mkdir(parents=True, exist_ok=True)
path.write_text(json.dumps(result, indent=2) + '\n')
print(path)
