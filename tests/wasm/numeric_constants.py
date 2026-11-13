#!/usr/bin/env python3
"""Recompute every Payne-Hanek limb with two Decimal precisions."""
# Copyright (c) 2026- David Lucius Severus
# Distributed under the Boost Software License, Version 1.0.
from decimal import Decimal, localcontext
from pathlib import Path
import re

BITS = 16896
ROOT = Path(__file__).resolve().parents[2]


def two_over_pi(digits):
    with localcontext() as context:
        context.prec = digits
        coefficient, linear, power, k = 1, 13591409, 1, 6
        total = Decimal(linear)
        for i in range(1, digits // 14 + 10):
            coefficient = coefficient * (k * k * k - 16 * k) // (i * i * i)
            linear += 545140134
            power *= -262537412640768000
            total += Decimal(coefficient * linear) / power
            k += 12
        pi = 426880 * Decimal(10005).sqrt() / total
        return int((Decimal(2) / pi) * (1 << BITS))


expected = two_over_pi(5300)
assert expected == two_over_pi(5500), 'precision cross-check failed'
source = (ROOT / 'src/bits/wasm/math/two_over_pi.hpp').read_text()
limbs = [int(x, 16) for x in re.findall(r'0x([0-9a-f]{8})u', source)]
assert len(limbs) == BITS // 32
assert sum(value << (32 * index) for index, value in enumerate(limbs)) == expected
print(f'PASS all {len(limbs)} Payne-Hanek limbs at 5300 and 5500 decimal digits')
