#!/usr/bin/env python3
"""Build real Micron modules with Duck and grade Node's 1/6 exit contract."""
import argparse
import concurrent.futures
import hashlib
import itertools
import json
import os
from pathlib import Path
import re
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
PROFILES = {
    ('simd' if simd else 'scalar') + ('-threads' if threads else '') + ('-eh' if eh else ''):
        dict(simd=simd, threads=threads, eh=eh)
    for simd, threads, eh in itertools.product((False, True), repeat=3)
}
TESTS = [
    dict(name=name, source=f'tests/wasm/{name}.cpp') for name in
    ('arch', 'allocator', 'allocator_cabi', 'allocator_new', 'simd', 'numeric', 'math', 'math_edges', 'format_wide', 'runtime', 'runtime_main_args', 'host_probe')
] + [
    dict(name=name, source=f'tests/wasm/{name}.cpp', fuzz=True, minCases=1, timeout=120,
         maximumPages=1024 if name == 'fuzz_memory' else 8192)
    for name in ('fuzz_memory', 'fuzz_simd', 'fuzz_chrono')
] + [
    dict(name='runtime_lifecycle', source='tests/wasm/runtime_lifecycle.cpp', lifecycle=True, timeout=120),
    dict(name='allocator_oom', source='tests/wasm/allocator_oom.cpp', maximumPages=1024),
    dict(name='runtime_eh', source='tests/wasm/runtime_eh.cpp', eh=True),
    dict(name='fuzz_eh', source='tests/wasm/fuzz_eh.cpp', eh=True, fuzz=True, minCases=1, timeout=120),
    dict(name='allocator_eh', source='tests/wasm/allocator_eh.cpp', eh=True, maximumPages=1024),
    dict(name='runtime_guard', source='tests/wasm/runtime_guard.cpp', eh=True),
    dict(name='runtime_ctors', source='tests/wasm/runtime_ctors.cpp', threads=True),
    dict(name='runtime_futex', source='tests/wasm/runtime_futex.cpp', threads=True),
    dict(name='runtime_threads', source='tests/wasm/runtime_threads.cpp', threads=True),
    dict(name='runtime_handles', source='tests/wasm/runtime_handles.cpp', threads=True),
    dict(name='runtime_cancel', source='tests/wasm/runtime_cancel.cpp', threads=True, timeout=60),
    dict(name='runtime_cancel_eh', source='tests/wasm/runtime_cancel_eh.cpp', threads=True, eh=True, minCases=1, timeout=120),
    dict(name='runtime_join_reuse', source='tests/wasm/runtime_join_reuse.cpp', threads=True, timeout=60),
    dict(name='runtime_thread_fuzz', source='tests/wasm/runtime_thread_fuzz.cpp', threads=True, fuzz=True, minCases=1, timeout=120),
    dict(name='runtime_multitu', source='tests/wasm/runtime_multitu/main.cpp',
         objects=['tests/wasm/runtime_multitu/a.cpp', 'tests/wasm/runtime_multitu/b.cpp']),
    dict(name='allocator_shared', source='tests/wasm/allocator_shared.cpp', threads=True),
    dict(name='vector_zero', source='tests/rigor/vector_zero.cpp', minCases=4),
    dict(name='tuple_exhaustive', source='tests/rigor/tuple_exhaustive.cpp', minCases=20),
    dict(name='hash_table_property', source='tests/rigor/hash_table_property.cpp', minCases=1),
    dict(name='format_values', source='tests/rigor/format_values.cpp', minCases=50, eh=True),
    dict(name='rigor_lz_laziness', source='tests/rigor/rigor_lz_laziness.cpp', minCases=1, eh=True),
    dict(name='itable', source='tests/rigor/itable.cpp', minCases=83, eh=True),
    dict(name='parray', source='tests/rigor/parray.cpp', minCases=86, eh=True),
    dict(name='rope', source='tests/rigor/rope.cpp', minCases=55, eh=True),
    dict(name='rigor_pvector', source='tests/rigor/rigor_pvector.cpp', minCases=49, eh=True, timeout=120),
]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profiles', default=','.join(PROFILES))
    parser.add_argument('--select', help='comma-separated test names; default all')
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--optimization', choices=('O0', 'O2', 'O3'), default='O2')
    parser.add_argument('--no-lto', action='store_true')
    parser.add_argument('--ubsan-traps', action='store_true', help='actual wasm32 UBSan; no external sanitizer runtime')
    parser.add_argument('--define', action='append', default=[], metavar='NAME=VALUE')
    parser.add_argument('--oracles', action='store_true', help='run exact numeric and seeded transcendental oracles')
    parser.add_argument('--oracle-iterations', type=int, default=6000)
    parser.add_argument('--oracle-seed', default='0x9e3779b97f4a7c15')
    parser.add_argument('--output', default='bin/wasm-tests')
    parser.add_argument('--duck', default='bin/duck')
    args = parser.parse_args()
    os.chdir(ROOT)
    out = Path(args.output)
    out.mkdir(parents=True, exist_ok=True)
    profiles = args.profiles.split(',')
    unknown = set(profiles) - PROFILES.keys()
    if args.select is not None and not args.select.strip():
        parser.error('--select must not be empty')
    selected = args.select.split(',') if args.select is not None else [x['name'] for x in TESTS]
    if len(profiles) != len(set(profiles)):
        parser.error('duplicate profiles are not allowed')
    if len(selected) != len(set(selected)):
        parser.error('duplicate test names are not allowed')
    unknown |= set(selected) - {x['name'] for x in TESTS}
    if unknown:
        parser.error('unknown names: ' + ','.join(sorted(unknown)))
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    if not 1 <= args.oracle_iterations <= 1000000:
        parser.error('--oracle-iterations must be in 1..1000000')
    try:
        if not 0 < int(args.oracle_seed, 0) < 2 ** 64:
            raise ValueError()
    except ValueError:
        parser.error('--oracle-seed must be a nonzero 64-bit integer')
    if any(not re.fullmatch(r'[A-Za-z_]\w*(?:=[A-Za-z0-9_xX.+-]+)?', value) for value in args.define):
        parser.error('--define expects NAME or NAME=VALUE')
    cells = [(p, t) for p in profiles for t in TESTS if t['name'] in selected
             and (not t.get('eh') or PROFILES[p]['eh'])
             and (not t.get('threads') or PROFILES[p]['threads'])]
    if not cells:
        parser.error('no eligible tests selected')

    def run(cell):
        profile, test = cell
        settings = PROFILES[profile]
        dest = out / profile
        dest.mkdir(parents=True, exist_ok=True)
        module = dest / (test['name'] + '.wasm')
        module.unlink(missing_ok=True)
        command = [args.duck, 'build', test['source'], '--wasm32', '--std', 'c++23', '--start', 'start',
                   '-i', '.', '-i', 'src', '-' + args.optimization, '-o', str(dest), '-f']
        if settings['simd']:
            command.append('--wasm-simd128')
        if settings['threads']:
            command.append('--wasm-threads')
        if settings['eh']:
            command.append('-ke')
        if args.no_lto:
            command.append('--no-lto')
        if args.ubsan_traps:
            command.append('--wasm-ubsan-traps')
        for definition in args.define:
            command += ['--def', definition]
        maximum = test.get('maximumPages', 8192)
        timeout = test.get('timeout', 30)
        command += ['--wasm-max-memory', str(maximum * 65536)]
        record = dict(name=profile + '/' + test['name'], file=str(module), threads=settings['threads'],
                      simd=settings['simd'], eh=settings['eh'], minCases=test.get('minCases', 0),
                      maximumPages=maximum, timeout=timeout, hostProbe=test['name'] == 'host_probe',
                      lifecycle=test.get('lifecycle', False), fuzz=test.get('fuzz', False),
                      numericOracle=args.oracles and test['name'] == 'numeric',
                      mathOracle=args.oracles and test['name'] == 'math',
                      oracleIterations=args.oracle_iterations, oracleSeed=args.oracle_seed, command=command)
        started = time.monotonic()
        log = ''
        try:
            if test.get('objects'):
                object_dir = dest / (test['name'] + '_objects')
                object_dir.mkdir(exist_ok=True)
                record['prerequisites'] = []
                objects = []
                for source in test['objects']:
                    object_command = command.copy()
                    object_command[2] = source
                    object_command[object_command.index('-o') + 1] = str(object_dir)
                    object_command.append('--obj')
                    record['prerequisites'].append(object_command)
                    obj = object_dir / (Path(source).stem + '.o')
                    obj.unlink(missing_ok=True)
                    built = subprocess.run(object_command, text=True, stdout=subprocess.PIPE,
                                           stderr=subprocess.STDOUT, timeout=120)
                    log += built.stdout
                    if built.returncode or not obj.is_file():
                        raise RuntimeError('translation unit did not compile: ' + source)
                    objects.append(str(obj))
                command += [test['name'] + '.wasm'] + objects
            compiled = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120)
            log += compiled.stdout
            if compiled.returncode or not module.is_file():
                raise RuntimeError('module did not compile/link')
            disassembly = subprocess.run(['llvm-objdump', '-d', str(module)], text=True, stdout=subprocess.PIPE,
                                         stderr=subprocess.STDOUT, timeout=30, check=True).stdout
            if not settings['simd'] and re.search(r'\b(?:v128|i(?:8x16|16x8|32x4|64x2)|f(?:32x4|64x2))\.', disassembly):
                raise RuntimeError('scalar profile emitted SIMD instructions')
            if not settings['threads'] and '.atomic.' in disassembly:
                raise RuntimeError('single-thread profile emitted atomics')
            if settings['eh'] and 'try_table' not in disassembly:
                raise RuntimeError('native standardized exception instructions were not emitted')
            if not settings['eh'] and re.search(r'\btry_table\b|\bcatch_all\b', disassembly):
                raise RuntimeError('no-EH profile emitted exception instructions')
            runtime = ['node', 'src/bits/wasm/node.mjs', str(module), f'--timeout={timeout}', f'--maximum-pages={maximum}']
            if settings['threads']:
                runtime.append('--threads')
            if record['minCases']:
                runtime.append('--min-cases=' + str(record['minCases']))
            executed = subprocess.run(runtime, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout + 5)
            log += '\n' + executed.stdout
            record['exit'] = executed.returncode
            if executed.returncode != 1:
                raise RuntimeError(f'test returned {executed.returncode}; expected PASS sentinel 1')
            if record['hostProbe']:
                utf8 = ['node', 'tests/wasm/runtime_utf8.mjs', str(module)]
                if settings['threads']:
                    utf8.append('--threads')
                checked = subprocess.run(utf8, text=True, stdout=subprocess.PIPE,
                                         stderr=subprocess.STDOUT, timeout=timeout + 5)
                log += '\n' + checked.stdout
                if checked.returncode != 0:
                    raise RuntimeError('host UTF-8 stream checks failed')
            if record['lifecycle']:
                lifecycle = ['node', 'tests/wasm/runtime_lifecycle.mjs', str(module)]
                if settings['threads']:
                    lifecycle.append('--threads')
                checked = subprocess.run(lifecycle, text=True, stdout=subprocess.PIPE,
                                         stderr=subprocess.STDOUT, timeout=timeout + 5)
                log += '\n' + checked.stdout
                if checked.returncode != 0:
                    raise RuntimeError('host lifecycle checks failed')
                fuzz = ['node', 'tests/wasm/runtime_host_lifecycle.mjs', str(module)]
                if settings['threads']:
                    fuzz.append('--threads')
                checked = subprocess.run(fuzz, text=True, stdout=subprocess.PIPE,
                                         stderr=subprocess.STDOUT, timeout=timeout + 5)
                log += '\n' + checked.stdout
                if checked.returncode != 0:
                    raise RuntimeError('host lifecycle fuzz failed')
            oracle_runs = []
            if record['numericOracle']:
                oracle_runs.append(['node', 'tests/wasm/numeric_oracle.mjs', str(module),
                                    f'--iterations={args.oracle_iterations}', f'--seed={args.oracle_seed}'])
            if record['mathOracle']:
                oracle_runs += [['node', 'tests/wasm/math_oracle.mjs', str(module), f'--reference=tests/wasm/{corpus}']
                                for corpus in ('math_reference.json', 'fuzz_math_reference.json')]
            record['oracles'] = []
            for oracle in oracle_runs:
                if settings['threads']:
                    oracle.append('--threads')
                checked = subprocess.run(oracle, text=True, stdout=subprocess.PIPE,
                                         stderr=subprocess.STDOUT, timeout=180)
                log += '\n' + checked.stdout
                if checked.returncode != 0:
                    raise RuntimeError('numeric/math oracle failed')
                record['oracles'].append(json.loads(checked.stdout))
            record['bytes'] = module.stat().st_size
            record['sha256'] = hashlib.sha256(module.read_bytes()).hexdigest()
            record['status'] = 'PASS'
        except Exception as error:
            record['status'] = 'FAIL'
            record['error'] = str(error)
            log += '\n' + str(error) + '\n'
        record['seconds'] = round(time.monotonic() - started, 3)
        (dest / (test['name'] + '.log')).write_text(log)
        print(record['status'], record['name'], record.get('error', ''), flush=True)
        return record

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(run, cells))
    failed = sum(x['status'] != 'PASS' for x in results)
    report = dict(failed=failed, optimization=args.optimization, lto=not args.no_lto,
                  ubsan_traps=args.ubsan_traps, definitions=args.define, results=results)
    (out / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    (out / 'modules.json').write_text(json.dumps(dict(modules=[r for r in results if r['status'] == 'PASS']), indent=2) + '\n')
    print(f'{len(results) - failed}/{len(results)} cells passed', flush=True)
    return bool(failed)


if __name__ == '__main__':
    raise SystemExit(main())
