#!/usr/bin/env python3
"""Check selection validation and unsigned Wasm coverage-export grading."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', default='bin/wasm-grading-controls')
    parser.add_argument('--duck', default='bin/duck')
    parser.add_argument('--suite', default='tests/wasm/suite.py')
    parser.add_argument('--node', default='src/bits/wasm/node.mjs')
    parser.add_argument('--browsers', action='store_true')
    parser.add_argument('--engines', default='chromium')
    args = parser.parse_args()
    os.chdir(ROOT)
    out = Path(args.output)
    out.mkdir(parents=True, exist_ok=True)
    results = []

    def run(name, command, expected, timeout=15):
        completed = subprocess.run(command, text=True, stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, timeout=timeout)
        log = out / (name + '.log')
        log.write_text(completed.stdout)
        record = dict(name=name, command=command, expected=expected,
                      exit=completed.returncode, passed=completed.returncode == expected,
                      log=str(log))
        results.append(record)
        print('PASS' if record['passed'] else 'FAIL', name, 'exit', completed.returncode, flush=True)
        return record

    invalid = [
        ('empty-selection', ['--select', '']),
        ('blank-selection', ['--select', ' ']),
        ('duplicate-profile', ['--profiles', 'scalar,scalar']),
        ('duplicate-test', ['--select', 'arch,arch']),
        ('empty-list-member', ['--select', 'arch,']),
        ('ineligible-selection', ['--select', 'runtime_eh']),
    ]
    for name, options in invalid:
        # The nonexistent Duck also proves argument rejection precedes compilation.
        command = [sys.executable, args.suite, '--profiles', 'scalar', '--select', 'arch',
                   '--duck', str(out / 'must-not-execute-duck'), '--output', str(out / name)] + options
        run(name, command, 2)

    modules = {}
    for mode in range(11):
        dest = out / f'coverage-{mode}'
        module = dest / 'coverage_control.wasm'
        dest.mkdir(exist_ok=True)
        module.unlink(missing_ok=True)
        command = [args.duck, 'build', 'tests/wasm/fixtures/coverage_control.cpp', '--wasm32',
                   '--start', 'start', '-i', '.', '-i', 'src', '-O2', '-o', str(dest),
                   '--def', f'MICRON_COVERAGE_CONTROL={mode}', '-f']
        built = run(f'build-{mode}', command, 0, 120)
        if not built['passed'] or not module.is_file():
            if built['passed']:
                built['passed'] = False
                built['error'] = 'compiler produced no module'
            continue
        modules[mode] = module
        runtime = ['node', args.node, str(module), '--min-cases=1', '--timeout=5']
        record = run(f'coverage-{mode}', runtime, 1 if mode in (1, 2, 3) else 70)
        if mode not in (1, 2, 3):
            expected_error = 'Micron test coverage failed:' if mode in (0, 4) else 'invalid unsigned test coverage export'
            if expected_error not in Path(record['log']).read_text():
                record['passed'] = False
                record['error'] = 'wrong failure reason'

    if args.browsers:
        for name, modes, expected in [('browser-high-counts', [2, 3], 0),
                                      ('browser-nonzero-skips', [4], 1),
                                      ('browser-invalid-count', [5], 1)]:
            if any(mode not in modules for mode in modes):
                results.append(dict(name=name, passed=False, error='missing fixture modules'))
                continue
            manifest = out / (name + '.json')
            manifest.write_text(json.dumps(dict(modules=[dict(
                name=f'coverage-{mode}', file=str(modules[mode].resolve().relative_to(ROOT)), threads=False,
                minCases=1, exit=1, timeout=10) for mode in modes]), indent=2) + '\n')
            command = ['node', 'tests/wasm/browser.mjs', f'--manifest={manifest}',
                       f'--output={out / (name + "-results.json")}', f'--engines={args.engines}']
            record = run(name, command, expected, 120)
            report = json.loads((out / (name + '-results.json')).read_text())
            wanted = len(args.engines.split(',')) * len(modes)
            statuses = report['results']
            if len(statuses) != wanted or any(item['status'] != ('PASS' if expected == 0 else 'FAIL') for item in statuses):
                record['passed'] = False
                record['error'] = 'browser result set does not match the grading control'
            if expected and any(('cases=' if modes == [4] else 'invalid unsigned test coverage export') not in item.get('error', '') for item in statuses):
                record['passed'] = False
                record['error'] = 'wrong browser failure reason'
    failed = sum(not result['passed'] for result in results)
    (out / 'results.json').write_text(json.dumps(dict(failed=failed, results=results), indent=2) + '\n')
    return bool(failed)


if __name__ == '__main__':
    raise SystemExit(main())
