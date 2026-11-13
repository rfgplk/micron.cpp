// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// Independent integer/rational oracles; host floating point never supplies an answer.
export function checkNumeric(e, memory, {seed = 0x9e3779b97f4a7c15n, iterations = 6000} = {}) {
  if (!Number.isSafeInteger(iterations) || iterations < 1 || iterations > 1000000) throw new RangeError('iterations must be in 1..1000000');
  seed = BigInt(seed);
  if (seed <= 0n || seed >= 1n << 64n) throw new RangeError('seed must be a nonzero 64-bit integer');
  const ptr = e.numeric_buffer();
  let data = new DataView(memory.buffer);
  function refresh() { if (data.buffer !== memory.buffer) data = new DataView(memory.buffer); }
  const P = 112n, I = 1n << P, S = 1n << 127n, INF = 0x7fffn << P, NAN = INF | (I >> 1n), MASK = S - 1n;
  let state = BigInt(seed);
  const random = () => { state ^= state << 13n; state ^= state >> 7n; state ^= state << 17n; return state &= (1n << 64n) - 1n; };
  const width = x => x === 0n ? 0 : x.toString(2).length;
  const quad = [113, 16383, 32767], double = [53, 1023, 2047], single = [24, 127, 255];
  function decode(x, [precision, bias, max] = quad) {
    const p = BigInt(precision - 1), implicit = 1n << p, sign = 1n << BigInt(precision - 1 + Math.log2(max + 1));
    const a = x & (sign - 1n), exp = Number(a >> p), infinity = BigInt(max) << p;
    return {s: x & sign, a, n: (a & (implicit - 1n)) | (exp ? implicit : 0n), exp: (exp || 1) - bias - (precision - 1), nan: a > infinity, inf: a === infinity};
  }
  function rounded(sign, n, d, exp, precision = 113, bias = 16383, max = 32767) {
    const p = precision - 1;
    if (n === 0n) return sign ? 1n << BigInt(p + Math.log2(max + 1)) : 0n;
    let log = width(n) - width(d);
    if (log >= 0 ? n < (d << BigInt(log)) : (n << BigInt(-log)) < d) --log;
    const k = log + exp, unit = Math.max(k - p, 1 - bias - p), shift = exp - unit;
    if (shift >= 0) n <<= BigInt(shift); else d <<= BigInt(-shift);
    let q = n / d; const r = n % d;
    if (r * 2n > d || (r * 2n === d && (q & 1n))) ++q;
    const exponent = unit + p + bias;
    let value = exponent <= 0 ? q : (BigInt(exponent - 1) << BigInt(p)) + q;
    const infinity = BigInt(max) << BigInt(p);
    if (value >= infinity) value = infinity;
    return (sign ? 1n << BigInt(p + Math.log2(max + 1)) : 0n) | value;
  }
  function add(a, b) {
    const x = decode(a), y = decode(b);
    if (x.nan || y.nan) return NAN;
    if (x.inf || y.inf) return x.inf && y.inf && x.s !== y.s ? NAN : x.inf ? a : b;
    if (!x.n && !y.n) return x.s & y.s;
    const exp = Math.min(x.exp, y.exp);
    let n = (x.s ? -x.n : x.n) << BigInt(x.exp - exp);
    n += (y.s ? -y.n : y.n) << BigInt(y.exp - exp);
    return rounded(n < 0n, n < 0n ? -n : n, 1n, exp);
  }
  function mul(a, b, c, format = quad) {
    const x = decode(a, format), y = decode(b, format), z = c === undefined ? null : decode(c, format), sign = x.s ^ y.s;
    const infinity = BigInt(format[2]) << BigInt(format[0] - 1), nan = infinity | 1n << BigInt(format[0] - 2);
    if (x.nan || y.nan || z?.nan) return nan;
    if (x.inf || y.inf) {
      if (!x.n && !x.inf || !y.n && !y.inf || z?.inf && z.s !== sign) return nan;
      return sign | infinity;
    }
    if (z?.inf) return c;
    if (!x.n || !y.n) return z ? !z.n ? sign & z.s : c : sign;
    let n = x.n * y.n, exp = x.exp + y.exp;
    if (z) {
      const unit = Math.min(exp, z.exp);
      n = ((sign ? -n : n) << BigInt(exp - unit)) + ((z.s ? -z.n : z.n) << BigInt(z.exp - unit));
      exp = unit;
      return rounded(n < 0n, n < 0n ? -n : n, 1n, exp, ...format);
    }
    return rounded(sign, n, 1n, exp, ...format);
  }
  function div(a, b) {
    const x = decode(a), y = decode(b), sign = x.s ^ y.s;
    if (x.nan || y.nan || x.inf && y.inf || !x.n && !x.inf && !y.n && !y.inf) return NAN;
    if (x.inf || !y.n && !y.inf) return sign | INF;
    if (y.inf || !x.n) return sign;
    return rounded(sign, x.n, y.n, x.exp - y.exp);
  }
  function remainder(a, b, nearest) {
    const x = decode(a), y = decode(b);
    if (x.nan || y.nan || x.inf || !y.inf && !y.n) return NAN;
    if (y.inf || !x.n) return a;
    const unit = Math.min(x.exp, y.exp);
    const numerator = x.n << BigInt(x.exp - unit), divisor = y.n << BigInt(y.exp - unit);
    let q = numerator / divisor, r = numerator % divisor;
    if (nearest && (2n * r > divisor || 2n * r === divisor && (q & 1n))) r -= divisor;
    const negative = r < 0n ? !x.s : x.s;
    return rounded(negative, r < 0n ? -r : r, 1n, unit);
  }
  function isqrt(n) { if (!n) return 0n; let r = 1n << BigInt((width(n) + 1) >> 1); for (;;) { const q = (r + n / r) >> 1n; if (q >= r) return r; r = q; } }
  function sqrt(a, format = quad) {
    const x = decode(a, format), p = format[0] - 1;
    const nan = (BigInt(format[2]) << BigInt(p)) | 1n << BigInt(p - 1);
    if (x.nan) return nan;
    if (!x.n && !x.inf) return a;
    if (x.s) return nan;
    if (x.inf) return a;
    const k = Math.floor((width(x.n) - 1 + x.exp) / 2), unit = k - p, shift = x.exp - 2 * unit;
    const n = shift >= 0 ? x.n << BigInt(shift) : x.n, d = shift < 0 ? 1n << BigInt(-shift) : 1n;
    let q = isqrt(n / d); const midpoint = d * (2n * q + 1n) ** 2n;
    if (4n * n > midpoint || 4n * n === midpoint && (q & 1n)) ++q;
    return (BigInt(k + format[1] - 1) << BigInt(p)) + q;
  }
  function sample(i) {
    const exponent = [0n, 1n, 2n, 0x3fffn, 0x3f8en, 0x7ffen, random() % 32767n][i % 7];
    return (random() & S >> 64n) << 64n | exponent << P | ((random() << 64n | random()) & (I - 1n));
  }
  function put(index, value) { refresh(); data.setBigUint64(ptr + index * 16, value & ((1n << 64n) - 1n), true); data.setBigUint64(ptr + index * 16 + 8, value >> 64n, true); }
  function get() { refresh(); return data.getBigUint64(ptr + 48, true) | data.getBigUint64(ptr + 56, true) << 64n; }
  let checks = 0;
  const operations = Array(39).fill(0);
  function check(op, a, b, c, expected, format = quad) {
    put(0, a); put(1, b); put(2, c);
    try { e.numeric_probe(op); }
    catch (cause) {
      throw new Error(`numeric probe trapped: op${op} a=${a.toString(16)} b=${b.toString(16)} c=${c.toString(16)} seed=0x${seed.toString(16)} check=${checks}`, {cause});
    }
    const actual = get();
    if (actual !== expected && !(format && decode(actual, format).nan && decode(expected, format).nan)) {
      throw new Error(`op${op}: a=${a.toString(16)} b=${b.toString(16)} c=${c.toString(16)} expected=${expected.toString(16)} actual=${actual.toString(16)}`);
    }
    ++checks; ++operations[op];
  }
  const edges = [0n, S, 1n, S | 1n, I - 1n, I, I + 1n, 0x3fffn << P, (0x3fffn << P) + 1n, INF - 1n, S | INF - 1n, INF, S | INF, NAN];
  for (const a of edges) for (const b of edges) {
    check(0, a, b, 0n, add(a, b)); check(1, a, b, 0n, add(a, b ^ S)); check(2, a, b, 0n, mul(a, b)); check(3, a, b, 0n, div(a, b));
    check(4, a, b, a ^ S, mul(a, b, a ^ S)); check(5, a, b, 0n, sqrt(a));
    check(26, a, b, 0n, remainder(a, b, false)); check(27, a, b, 0n, remainder(a, b, true));
  }
  for (let i = 0; i < iterations; ++i) {
    const a = sample(i), b = sample(i + 3), c = sample(i + 5);
    check(0, a, b, c, add(a, b)); check(1, a, b, c, add(a, b ^ S)); check(2, a, b, c, mul(a, b)); check(3, a, b, c, div(a, b));
    check(4, a, b, c, mul(a, b, c)); check(5, a & MASK, b, c, sqrt(a & MASK));
    check(26, a, b, 0n, remainder(a, b, false)); check(27, a, b, 0n, remainder(a, b, true));
    const integer = random() << 64n | random();
    check(6, integer, 0n, 0n, rounded(false, integer, 1n, 0));
    const signed = integer & S ? integer - (1n << 128n) : integer;
    check(7, integer, 0n, 0n, rounded(signed < 0n, signed < 0n ? -signed : signed, 1n, 0));
    const x = decode(a);
    check(10, a, 0n, 0n, rounded(x.s, x.n, 1n, x.exp, 53, 1023, 2047));
    check(11, a, 0n, 0n, rounded(x.s, x.n, 1n, x.exp, 24, 127, 255));
  }
  function truncInteger(a, format) {
    const x = decode(a, format);
    const n = x.exp < 0 ? x.n >> BigInt(-x.exp) : x.n << BigInt(x.exp);
    return x.s ? BigInt.asUintN(128, -n) : n;
  }
  for (const [format, fmaOp, sqrtOp, uintOp, intOp, fixUnsignedOp, fixSignedOp, extendOp] of [
    [double, 12, 13, 16, 17, 20, 21, 24], [single, 14, 15, 18, 19, 22, 23, 25]
  ]) {
    const p = BigInt(format[0] - 1), implicit = 1n << p, infinity = BigInt(format[2]) << p;
    const sign = 1n << BigInt(format[0] - 1 + Math.log2(format[2] + 1));
    const smallEdges = [0n, sign, 1n, sign | 1n, implicit - 1n, implicit, implicit + 1n,
      BigInt(format[1]) << p, (BigInt(format[1]) << p) + 1n, infinity - 1n,
      sign | infinity - 1n, infinity, sign | infinity, infinity | implicit >> 1n];
    for (const a of smallEdges) for (const b of smallEdges) {
      check(fmaOp, a, b, a ^ sign, mul(a, b, a ^ sign, format), format);
      check(sqrtOp, a, 0n, 0n, sqrt(a, format), format);
    }
    for (let i = 0; i < iterations; ++i) {
      const a = random() & (sign * 2n - 1n), b = random() & (sign * 2n - 1n), c = random() & (sign * 2n - 1n);
      check(fmaOp, a, b, c, mul(a, b, c, format), format);
      check(sqrtOp, a, 0n, 0n, sqrt(a, format), format);
      const integer = random() << 64n | random(), signed = BigInt.asIntN(128, integer);
      check(uintOp, integer, 0n, 0n, rounded(false, integer, 1n, 0, ...format), format);
      check(intOp, integer, 0n, 0n, rounded(signed < 0n, signed < 0n ? -signed : signed, 1n, 0, ...format), format);
      // Defined C++ conversion domain: finite, nonnegative for unsigned, magnitude below 2^127.
      const inRange = BigInt(Number(random() % 130n) + format[1] - 4) << p | random() & (implicit - 1n);
      check(fixUnsignedOp, inRange, 0n, 0n, truncInteger(inRange, format), null);
      const signedRange = inRange | (random() & 1n ? sign : 0n);
      check(fixSignedOp, signedRange, 0n, 0n, truncInteger(signedRange, format), null);
      const x = decode(a, format);
      const expected = x.nan ? NAN : x.inf ? (x.s ? S : 0n) | INF : rounded(x.s, x.n, 1n, x.exp);
      check(extendOp, a, 0n, 0n, expected);
    }
  }
  for (let i = 0; i < iterations; ++i) {
    const inRange = BigInt(Number(random() % 131n) + 16383 - 4) << P | ((random() << 64n | random()) & (I - 1n));
    check(8, inRange, 0n, 0n, truncInteger(inRange, quad), null);
    const signedRange = inRange | (random() & 1n ? S : 0n);
    check(9, signedRange, 0n, 0n, truncInteger(signedRange, quad), null);
  }
  // Exact integer helpers: only defined division/shift inputs reach the C++ probe.
  const wrap = n => BigInt.asUintN(128, n), signed = n => BigInt.asIntN(128, n);
  for (let i = 0; i < iterations; ++i) {
    const a = random() << 64n | random();
    const b = (i % 3 ? random() << 64n | random() : 1n << (random() % 128n)) || 1n;
    check(28, a, b, 0n, wrap(a * b), null);
    check(29, a, b, 0n, a / b, null);
    check(30, a, b, 0n, a % b, null);
    if (signed(a) !== -S || signed(b) !== -1n) {
      check(31, a, b, 0n, wrap(signed(a) / signed(b)), null);
      check(32, a, b, 0n, wrap(signed(a) % signed(b)), null);
    }
    const shift = i < 128 ? BigInt(i) : random() % 128n;
    check(33, a, shift, 0n, wrap(a << shift), null);
    check(34, a, shift, 0n, a >> shift, null);
    check(35, a, shift, 0n, wrap(signed(a) >> shift), null);
  }
  // Every valid ABI count, plus guarded source expressions whose unselected
  // shifts may be speculated by LLVM and lowered to compiler helper calls.
  const guardedCounts = [-2147483648n, -1024n, -129n, -128n, -127n, -65n, -64n, -63n, -1n,
    ...Array.from({length: 128}, (_, i) => BigInt(i)), 128n, 129n, 255n, 256n, 1024n, 2147483647n];
  const integerEdges = [0n, 1n, S - 1n, S, wrap(-1n), 0x0123456789abcdeffedcba9876543210n];
  for (const a of integerEdges) {
    for (const b of integerEdges) check(28, a, b, 0n, wrap(a * b), null);
    for (const shift of guardedCounts) {
      const valid = shift >= 0n && shift < 128n;
      const left = valid ? wrap(a << shift) : 0n, logical = valid ? a >> shift : 0n;
      const arithmetic = valid ? wrap(signed(a) >> shift) : 0n;
      check(36, a, wrap(shift), 0n, left, null);
      check(37, a, wrap(shift), 0n, logical, null);
      check(38, a, wrap(shift), 0n, arithmetic, null);
      if (valid) {
        check(33, a, shift, 0n, left, null);
        check(34, a, shift, 0n, logical, null);
        check(35, a, shift, 0n, arithmetic, null);
      }
    }
  }
  // Exact halfway quotients at both exponent limits, plus neighboring bit patterns.
  for (const exponent of [1n, 2n, 16383n, 32765n, 32766n]) {
    const y = exponent << P, halfway = y | I >> 1n;
    for (const delta of [-1n, 0n, 1n]) for (const sx of [0n, S]) for (const sy of [0n, S]) {
      const a = sx | halfway + delta, b = sy | y;
      check(27, a, b, 0n, remainder(a, b, true));
    }
  }
  if (operations.some(n => !n)) throw new Error('numeric operation coverage is incomplete');
  return {checks, seed: '0x' + seed.toString(16), iterations, operations};
}

if (typeof process !== 'undefined' && process.versions?.node &&
    import.meta.url === (await import('node:url')).pathToFileURL(process.argv[1]).href) {
  const {readFile} = await import('node:fs/promises');
  const {createLocal} = await import('../../src/bits/wasm/host.mjs');
  const option = (name, fallback) => process.argv.find(x => x.startsWith(`--${name}=`))?.slice(name.length + 3) ?? fallback;
  const threads = process.argv.includes('--threads');
  const module = await WebAssembly.compile(await readFile(process.argv[2]));
  const memory = new WebAssembly.Memory({initial: 256, maximum: 8192, shared: threads});
  const runtime = await createLocal(module, memory, {worker: threads});
  try {
    const result = checkNumeric(runtime.exports, memory, {
      seed: option('seed', '0x9e3779b97f4a7c15'), iterations: Number(option('iterations', 6000))
    });
    if (runtime.runMain() !== 1) throw new Error('directed numeric suite failed');
    console.log(JSON.stringify({status: 'PASS', ...result}));
  } finally { await runtime.shutdown(); }
}
