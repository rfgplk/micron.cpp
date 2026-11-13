#!/usr/bin/env python3
"""Replay deterministic Wasm backend fuzzing through Duck, Node and optional browsers."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_TESTS = ','.join((
    'fuzz_memory', 'fuzz_simd', 'fuzz_chrono', 'fuzz_eh', 'numeric', 'math', 'math_edges',
    'runtime_lifecycle', 'runtime_cancel', 'runtime_cancel_eh', 'runtime_join_reuse', 'runtime_thread_fuzz',
))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profiles', default='scalar,scalar-eh,scalar-threads,scalar-threads-eh,simd,simd-eh,simd-threads,simd-threads-eh')
    parser.add_argument('--seeds', default='0x243f6a88,0x9e3779b9,0xb7e15162')
    parser.add_argument('--select', default=DEFAULT_TESTS)
    parser.add_argument('--steps', type=int, help='override each C++ suite\'s domain-specific default')
    parser.add_argument('--iterations', type=int, default=6000, help='random iterations per exact numeric operation')
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--optimization', choices=('O0', 'O2', 'O3'), default='O2')
    parser.add_argument('--no-lto', action='store_true')
    parser.add_argument('--ubsan-traps', action='store_true')
    parser.add_argument('--browsers', action='store_true')
    parser.add_argument('--engines', default='chromium,firefox,webkit')
    parser.add_argument('--output', default='bin/wasm-fuzz')
    parser.add_argument('--duck', default='bin/duck')
    args = parser.parse_args()
    try:
        seeds = [int(seed, 0) for seed in args.seeds.split(',')]
        if not seeds or len(set(seeds)) != len(seeds) or any(not 0 < seed <= 0xffffffff for seed in seeds):
            raise ValueError()
    except ValueError:
        parser.error('--seeds requires distinct nonzero 32-bit integers')
    if args.steps is not None and not 1 <= args.steps <= 1000000:
        parser.error('--steps must be in 1..1000000')
    if not 1 <= args.iterations <= 1000000 or args.jobs < 1:
        parser.error('--iterations must be in 1..1000000 and --jobs must be positive')
    os.chdir(ROOT)
    output = Path(args.output)
    output.mkdir(parents=True, exist_ok=True)
    results = []
    started = time.monotonic()

    def run(command, log):
        print('RUN', ' '.join(command), flush=True)
        with log.open('w') as stream:
            # suite.py has a deadline for each compiler/runtime; browser.mjs for each cell.
            completed = subprocess.run(command, stdout=stream, stderr=subprocess.STDOUT)
        return completed.returncode

    controls = ['node', 'tests/wasm/oracle_controls.mjs']
    control_code = run(controls, output / 'oracle-controls.log')
    for seed in seeds:
        dest = output / f'{seed:08x}'
        dest.mkdir(parents=True, exist_ok=True)
        command = [sys.executable, 'tests/wasm/suite.py', '--profiles', args.profiles,
                   '--select', args.select, '--jobs', str(args.jobs), '--output', str(dest),
                   '--optimization', args.optimization, '--duck', args.duck, '--oracles',
                   '--oracle-seed', hex(seed), '--oracle-iterations', str(args.iterations),
                   '--define', f'MICRON_WASM_FUZZ_SEED=0x{seed:08x}u']
        if args.steps is not None:
            command += ['--define', f'MICRON_WASM_FUZZ_STEPS={args.steps}']
        if args.no_lto:
            command.append('--no-lto')
        if args.ubsan_traps:
            command.append('--ubsan-traps')
        code = run(command, dest / 'suite.log')
        result = dict(seed=hex(seed), command=command, exit=code)
        report_file = dest / 'results.json'
        if report_file.is_file():
            report = json.loads(report_file.read_text())
            result.update(cells=len(report['results']), failed=report['failed'])
        else:
            result.update(cells=0, failed=1)
        if args.browsers and code == 0 and result['cells']:
            browser = ['node', 'tests/wasm/browser.mjs', f'--manifest={dest / "modules.json"}',
                       f'--output={dest / "browsers.json"}', f'--engines={args.engines}']
            result['browserCommand'] = browser
            result['browserExit'] = run(browser, dest / 'browsers.log')
        results.append(result)
        print(json.dumps(result), flush=True)
    failed = bool(control_code or any(r['exit'] or r['failed'] or not r['cells'] or r.get('browserExit', 0) for r in results))
    report = dict(status='FAIL' if failed else 'PASS', seconds=round(time.monotonic() - started, 3),
                  controlsExit=control_code, results=results)
    (output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    print(report['status'], 'fuzz matrix', flush=True)
    return failed


if __name__ == '__main__':
    raise SystemExit(main())
