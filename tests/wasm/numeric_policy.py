#!/usr/bin/env python3
"""Binary128 approximation policy diagnostics and native header controls."""
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
COMMON = ['-std=c++23', '-I.', '-Isrc', '-fsyntax-only', '-x', 'c++', '-']
WASM = ['clang++-22', '--no-default-config', '--target=wasm32-unknown-unknown',
        '-fhosted', '-fno-builtin', '-D__micron_freestanding=1', '-fno-exceptions',
        '-fno-rtti', '-nostdinc', '-nostdinc++']
HEADER = '#include <micron/math/mk.hpp>\n'
DIAGNOSTIC = 'binary128 strict transcendental policies are unavailable'
checks = 0


def check(name, command, source, diagnostic=None):
    global checks
    result = subprocess.run(command + COMMON, input=HEADER + source, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, cwd=ROOT, timeout=30)
    valid = result.returncode == 0 if diagnostic is None else result.returncode != 0 and diagnostic in result.stdout
    if not valid:
        raise RuntimeError(name + '\n' + result.stdout)
    checks += 1
    print('PASS', name)


for simd in ['-mno-simd128', '-msimd128']:
    command = WASM + [simd]
    check(simd + ' default and explicit extended', command, '''
long double probe(long double x) {
  return micron::math::mk::trig::sin(x) + micron::math::mk::exp_ns::exp(x, micron::math::policy::extended)
       + micron::math::mk::pow_ns::sqrt(x) + micron::math::mk::fused::fma(x, x, x);
}
''')
    for policy in ['ieee', 'cr', 'faithful', 'fast']:
        check(simd + ' rejects binary128 ' + policy, command,
              f'long double probe(long double x) {{ return micron::math::mk::trig::sin(x, micron::math::policy::{policy}); }}\n',
              DIAGNOSTIC)
        check(simd + ' retains binary64 ' + policy, command,
              f'double probe(double x) {{ return micron::math::mk::trig::sin(x, micron::math::policy::{policy}); }}\n')
    check(simd + ' rejects direct binary128 CR', command,
          'long double probe(long double x) { return micron::math::cr::sin(x); }\n',
          'certified binary128 transcendental rounding is unavailable')

for compiler in ['clang++-22', 'g++']:
    check(compiler + ' native policy/header control', [compiler, '-march=x86-64'], '''
double probe(double x) {
  return micron::math::mk::trig::sin(x, micron::math::policy::faithful)
       + micron::math::mk::pow_ns::sqrt(x) + micron::math::mk::fused::fma(x, x, x);
}
''')
print(f'PASS {checks} policy/header controls')
