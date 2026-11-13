// Copyright (c) 2026 David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
import {checkMath} from './math_oracle.mjs';
import {checkNumeric} from './numeric_oracle.mjs';

// Test the graders independently of the implementation being graded.
const memory = new WebAssembly.Memory({initial: 1}), data = new DataView(memory.buffer);
const one = 0x3fffn << 112n, infinity = 0x7fffn << 112n, nan = infinity | 1n;
const put = (offset, value) => {
  data.setBigUint64(offset, BigInt.asUintN(64, value), true);
  data.setBigUint64(offset + 8, value >> 64n, true);
};
let assertions = 0;
function require(value, message) { ++assertions; if (!value) throw new Error(message); }
function fixture(expected = one, actual = expected) {
  return {
    reference: {limit_ulp: 128, cases: Array.from({length: 30}, (_, op) =>
      [op, '0x0', '0x0', '0x' + (op ? one : expected).toString(16)])},
    exports: {math_buffer: () => 0, math_probe: op => put(32, op ? one : actual)}
  };
}
function grade(expected, actual) {
  const x = fixture(expected, actual);
  return checkMath(x.exports, memory, x.reference);
}
require(grade(one, one).failed === 0, 'math positive control failed');
require(grade(one, one + 128n).failed === 0, 'inclusive ULP boundary rejected');
require(grade(one, one + 129n).failed === 1, 'finite error escaped ULP grading');
require(grade(infinity, infinity - 1n).failed === 1, 'finite maximum accepted as infinity');
require(grade(infinity - 1n, infinity).failed === 1, 'infinity accepted as finite maximum');
require(grade(infinity, nan).failed === 1, 'NaN accepted as infinity');
require(grade(nan, nan + 1n).failed === 0, 'valid NaN payload difference rejected');
let rejected = false;
try { checkMath(fixture().exports, memory, {cases: []}); } catch { rejected = true; }
require(rejected, 'empty math corpus accepted');
rejected = false;
try {
  checkNumeric({numeric_buffer: () => 0, numeric_probe: () => put(48, 1n)}, memory, {iterations: 1});
} catch (error) { rejected = /op0:.*expected=0 actual=1/.test(error.message); }
require(rejected, 'one-bit arithmetic error escaped exact grading');
console.log(JSON.stringify({status: 'PASS', assertions, negativeControls: 6}));
