#!/usr/bin/env python3
"""The runner must distinguish success, missing sentinel, assertion, trap and timeout."""
from pathlib import Path
from itertools import product
import subprocess

ROOT = Path(__file__).resolve().parents[2]
failures = 0
for shared, exceptions in product((False, True), repeat=2):
    profile = ('shared' if shared else 'single') + ('-eh' if exceptions else '')
    for expected in (1, 0, 6, 70, 124):
        directory = Path('bin/wasm-runner-controls') / profile / str(expected)
        module = directory / 'runner_control.wasm'
        (ROOT / directory).mkdir(parents=True, exist_ok=True)
        (ROOT / module).unlink(missing_ok=True)
        command = ['bin/duck', 'build', 'tests/wasm/fixtures/runner_control.cpp', '--wasm32', '--start', 'start',
                   '-i', '.', '-i', 'src', '-O2', '-o', str(directory), '--def', f'MICRON_RUNNER_CONTROL={expected}']
        if shared:
            command.append('--wasm-threads')
        if exceptions:
            command.append('-ke')
        compiled = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
        if compiled.returncode or not (ROOT / module).is_file():
            print('FAIL build', profile, expected, compiled.stdout)
            failures += 1
            continue
        run = ['node', 'src/bits/wasm/node.mjs', str(module), '--timeout=2'] + (['--threads'] if shared else [])
        result = subprocess.run(run, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=10)
        ok = result.returncode == expected
        print('PASS' if ok else 'FAIL', profile, 'expected', expected, 'observed', result.returncode)
        if not ok:
            print(result.stdout)
            failures += 1
        if expected == 1:
            result = subprocess.run(run + ['--min-cases=1'], cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=10)
            if result.returncode != 70:
                print('FAIL empty coverage accepted')
                failures += 1
            else:
                print('PASS empty coverage negative control')
raise SystemExit(bool(failures))
