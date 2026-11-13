#!/usr/bin/env python3
"""Positive and negative controls for compiler selection, Duck routing and reference integrity."""
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
BASE = ['clang++-22', '--no-default-config', '--target=wasm32-unknown-unknown', '-std=c++23',
        '-ffreestanding', '-fno-exceptions', '-fno-rtti', '-nostdinc', '-nostdinc++', '-I.', '-Isrc', '-fsyntax-only', '-x', 'c++', '-']
failures = []
count = 0


def check(name, command, source='', diagnostic=None):
    global count
    count += 1
    result = subprocess.run(command, input=source, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, timeout=30, cwd=ROOT)
    ok = result.returncode == 0 if diagnostic is None else result.returncode != 0 and diagnostic in result.stdout
    print('PASS' if ok else 'FAIL', name)
    if not ok:
        failures.append((name, result.stdout))
    return result.stdout


foundation = '#include <micron/types.hpp>\n#include <micron/bits/__ctasserts.hpp>\nstatic_assert(__ct_type_checker::check());\n'
check('wasm32 ILP32 positive control', BASE, foundation)
for name, flags, diagnostic in [
    ('wasm64', ['-D__wasm64__'], 'wasm64 is not supported'),
    ('wasi', ['-D__wasi__'], 'native wasm32-unknown-unknown'),
    ('emscripten', ['-D__EMSCRIPTEN__'], 'native wasm32-unknown-unknown'),
    ('threads without atomics', ['-DMICRON_WASM_THREADS'], 'requires -matomics'),
    ('workers without TLS', ['-DMICRON_WASM_THREADS', '-matomics', '-mbulk-memory', '-DMICRON_NO_TLS'], 'require thread-local storage'),
    ('legacy exception selector', ['-D__micron_eh'], 'require -fwasm-exceptions'),
    ('fast math', ['-ffast-math'], 'requires strict floating-point'),
    ('ELF runtime', ['-DMICRON_ATTACH_MODULE'], 'cannot use a Linux, metal or ELF'),
]:
    check(name, BASE[:-1] + flags + ['-'], foundation, diagnostic)
for header, diagnostic in [
    ('syscall.hpp', 'Linux syscalls are unavailable'),
    ('io.hpp', 'native filesystem I/O is unavailable'),
    ('vector/reserved_vector.hpp', 'requires virtual memory reservation'),
    ('tasks/coroutine/fiber.hpp', 'stack switching is unavailable'),
    ('parallel/engine.hpp', 'task-based parallel engine is unavailable'),
    ('elf.hpp', 'ELF loading is unavailable'),
    ('dynamic.hpp', 'native dynamic loading is unavailable'),
    ('sec.hpp', 'Linux security controls are unavailable'),
]:
    check('unsupported ' + header, BASE, f'#include <micron/{header}>\n', diagnostic)

duck = ['bin/duck', 'splat', 'build', 'tests/wasm/arch.cpp', '--wasm32', '--start', 'start', '-i', '.', '-i', 'src']
for flags in [[], ['--wasm-simd128'], ['--wasm-threads'], ['--wasm-ubsan-traps'], ['-ke'], ['-ke', '--wasm-threads'], ['--raw-obj'], ['--asm'], ['--pp']]:
    output = check('Duck ' + ' '.join(flags), duck + flags)
    if '--target=wasm32-unknown-unknown' not in output or '-march=native' in output or '--allow-undefined' in output:
        failures.append(('Duck routing', output))
    if any(x in flags for x in ('--raw-obj', '--asm', '--pp')) and ('start/start.cpp' in output or '-Wl,' in output):
        failures.append(('compile-only links startup', output))
    if '--wasm-ubsan-traps' in flags and not all(x in output for x in ('-fsanitize=undefined', '-fsanitize-trap=all', '-fno-sanitize=vptr')):
        failures.append(('target trap sanitizer missing', output))
check('native rejects Wasm sanitizer flag', ['bin/duck', 'splat', 'build', 'tests/wasm/arch.cpp', '--wasm-ubsan-traps'],
      diagnostic='--wasm-* options require --wasm32')
for flags, diagnostic in [
    (['--gcc'], 'cannot be combined'), (['--isa', 'base'], 'cannot be combined'), (['--arm'], 'cannot be combined'),
    (['--direct'], 'cannot use native entry points'), (['-Ofast'], 'requires precise math'),
    (['--wasm-max-memory', '65537'], 'multiples of 65536'),
    (['--wasm-stack-size', '16777216'], 'stack < initial memory'),
    (['--wasm-stack-size', '16'], '65536 <= stack'),
    (['--wasm-max-memory', '99999999999999999'], 'no larger than 2147483648'),
    (['-ke', '--no-eh'], 'requires exceptions and RTTI'),
]:
    check('Duck rejects ' + ' '.join(flags), duck + flags, diagnostic=diagnostic)
for mode in ([], ['parallel']):
    check('missing runner fails ' + ' '.join(mode), ['bin/duck', 'test'] + mode +
          ['tests/wasm/arch.cpp', '--wasm32', '--wasm-runner', '/nonexistent/micron-wasm-runner.mjs'],
          diagnostic='missing /nonexistent/micron-wasm-runner.mjs')

manifest = json.loads((ROOT / 'references/wasm/manifest.json').read_text())
for entry in manifest['sources']:
    count += 1
    path = ROOT / 'references/wasm' / entry['file']
    ok = hashlib.sha256(path.read_bytes()).hexdigest() == entry['sha256']
    print('PASS' if ok else 'FAIL', 'reference ' + entry['file'])
    if not ok:
        failures.append(('reference digest', entry['file']))
check('abcmalloc unchanged', ['git', 'diff', '--exit-code', '--', 'src/memory/allocation/abcmalloc'])
check('wasmalloc provenance', ['python3', 'scripts/sync_wasmalloc.py', '--check'])
for name, output in failures:
    print('\nFAIL', name, '\n', output)
print(f'{count} controls checked, {len(failures)} failures')
raise SystemExit(bool(failures))
