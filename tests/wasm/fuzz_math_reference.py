#!/usr/bin/env python3
# Copyright (c) 2026- David Lucius Severus
# Distributed under the Boost Software License, Version 1.0.
"""Seeded binary128 transcendental oracle corpus. mpmath is test-only."""
import argparse
import json
from pathlib import Path
import mpmath as mp
from math_reference import encode, decode, funcs, SIGN, INF


def generate(seed=0x6d617468, iterations=64):
    state = seed
    def random():
        nonlocal state
        state ^= (state << 13) & 0xffffffff
        state ^= state >> 17
        state ^= (state << 5) & 0xffffffff
        return state

    def fraction():
        return mp.mpf((random() << 96) | (random() << 64) | (random() << 32) | random()) / mp.mpf(1 << 128)

    rows = []
    def append(op, x, y=mp.mpf('1.25')):
        a, b = encode(mp.mpf(x)), encode(mp.mpf(y))
        x, y = decode(a), decode(b)
        if op == 25:
            result = mp.re(mp.loggamma(x))
        elif op == 24 and x > 2000:
            result = mp.inf
        else:
            result = funcs[op](x, y) if op in (19, 20, 21) else funcs[op](x)
        rows.append([op, hex(a), hex(b), hex(encode(result))])

    with mp.workprec(256):
        for op in range(30):
            for _ in range(iterations):
                u, v = fraction(), fraction()
                y = (v * 2 - 1) * mp.power(2, int(random() % 201) - 100)
                if op in (0, 1, 12, 13):
                    x = (u * 2 - 1) * (16000 if op == 1 else 11350)
                elif op in (2, 5):
                    x = u * 2 - (mp.mpf('0.99999999999999999999999999999999') if op == 5 else 1)
                elif op in (3, 4, 18, 20):
                    x = (1 + u) * mp.power(2, int(random() % 32800) - 16420)
                    if op == 18 and random() & 1:
                        x = -x
                elif op in (10, 11, 17):
                    x = (u * 2 - 1) * (1 - mp.power(2, -112))
                elif op in (9, 14, 15, 21):
                    x = (u * 2 - 1) * mp.power(2, int(random() % 32800) - 16420)
                elif op == 16:
                    x = 1 + u * mp.power(2, int(random() % 16383))
                elif op == 19:
                    x, y = mp.mpf('0.125') + 8*u, (v * 2 - 1) * 100
                elif op in (22, 23):
                    x = (u * 2 - 1) * (110 if op == 23 else 8)
                elif op in (24, 25):
                    x = mp.mpf('0.0625') + u * 1800
                else:
                    x = mp.mpf('0.0001') + u * 128
                    if op in (6, 7, 8, 26, 27) and random() & 1:
                        x = -x
                append(op, x, y)
        maximum = decode(INF - 1)
        for divisor in (1, 2, 11340, 11346, 11347, 11348, 11356, 11400, 65536):
            for op in (24, 25):
                append(op, maximum / divisor)
        # Exercise both sides of the lgamma scaling seam, poles and reflection.
        for exponent in (8191, 8192, 8193, 16369, 16370, 16371, 16382, 16383):
            append(25, (1 + fraction()) * mp.power(2, exponent))
        for k in range(1, 17):
            for delta in (-mp.power(2, -100), mp.power(2, -100), mp.mpf('0.125')):
                for op in (24, 25):
                    append(op, -k + delta)
        # Cancellation near the zeros of lgamma and log1p/exp paired boundaries.
        for exponent in (1, 16, 64, 100, 112):
            for sign in (-1, 1):
                for center in (1, 2):
                    append(25, center + sign * mp.power(2, -exponent))
        # Bessel and gamma limits include subnormal division/logarithm seams.
        for exponent in (-16494, -16493, -16400, -16385, -16384, -16382, -16300, -10000, -1000, -128, -65, -64, -63):
            for significand in (1, mp.mpf('1.25'), mp.mpf('1.75')):
                for op in (24, 25, 26, 27, 28, 29):
                    append(op, significand * mp.power(2, exponent))
                    if op in (24, 25):
                        append(op, -significand * mp.power(2, exponent))
        threshold = encode((2 / mp.pi) / maximum)
        for delta in range(-2, 3):
            append(29, decode(threshold + delta))
        # Smooth finite powers expose accumulated error in repeated squaring.
        for power in (31, 32, 33, 255, 256, 65535, 65536, 2147483647):
            for target in (-100, -1, 1, 100):
                base = mp.exp(mp.mpf(target) / power)
                for sign in (-1, 1):
                    append(19, base, sign * power)
                    append(19, -base, sign * power)

    with mp.workprec(17500):
        for exponent in (113, 114, 1024, 8192, 16383):
            for _ in range(4):
                x = (1 + fraction()) * mp.power(2, exponent)
                for op in (6, 7, 8):
                    append(op, x)
    return {'generator': 'mpmath ' + mp.__version__, 'seed': hex(seed),
            'iterations': iterations, 'precision': '256 bits; 17500 for large trig',
            'limit_ulp': 128, 'cases': rows}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=lambda x: int(x, 0))
    parser.add_argument('--iterations', type=int)
    parser.add_argument('--output', type=Path, default=Path('tests/wasm/fuzz_math_reference.json'))
    args = parser.parse_args()
    if args.seed is not None and not 0 < args.seed <= 0xffffffff or args.iterations is not None and not 1 <= args.iterations <= 100000:
        parser.error('seed must be nonzero uint32; iterations must be in 1..100000')
    batches = [(args.seed or 0x6d617468, args.iterations or 64)] if args.seed is not None or args.iterations is not None else [
        (0x6d617468, 64), (0x9e3779b9, 128), (0xb7e15162, 128)]
    generated = [generate(seed, count) for seed, count in batches]
    report = dict(generator=generated[0]['generator'], precision=generated[0]['precision'], limit_ulp=128,
                  seeds=[hex(seed) for seed, _ in batches], batches=[dict(seed=hex(seed), iterations=count) for seed, count in batches],
                  cases=[case for corpus in generated for case in corpus['cases']])
    args.output.write_text(json.dumps(report, separators=(',', ':')) + '\n')
