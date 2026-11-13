// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
export function checkMath(e, memory, reference) {
  const ptr = e.math_buffer(), worst = Array(30).fill(0n), operations = Array(30).fill(0);
  const sign = 1n << 127n, mask = sign - 1n, inf = 0x7fffn << 112n;
  const ordered = x => x & sign ? sign - (x & mask) : sign + x;
  const limit = BigInt(reference.limit_ulp ?? 128), failures = [];
  let checks = 0;
  for (const [op, ax, bx, rx] of reference.cases) {
    const data = new DataView(memory.buffer), a = BigInt(ax), b = BigInt(bx), expected = BigInt(rx);
    data.setBigUint64(ptr, a & ((1n << 64n) - 1n), true); data.setBigUint64(ptr + 8, a >> 64n, true);
    data.setBigUint64(ptr + 16, b & ((1n << 64n) - 1n), true); data.setBigUint64(ptr + 24, b >> 64n, true);
    e.math_probe(op);
    const result = new DataView(memory.buffer);
    const actual = result.getBigUint64(ptr + 32, true) | result.getBigUint64(ptr + 40, true) << 64n;
    ++checks; ++operations[op];
    if ((actual & mask) > inf && (expected & mask) > inf) continue;
    let distance = ordered(actual) - ordered(expected);
    if (distance < 0n) distance = -distance;
    if (distance > worst[op]) worst[op] = distance;
    const classificationMismatch = ((actual & mask) >= inf || (expected & mask) >= inf) && actual !== expected;
    if (classificationMismatch || distance > limit)
      failures.push({op, a: ax, b: bx, expected: rx, actual: '0x' + actual.toString(16), ulp: String(distance), classificationMismatch});
  }
  if (!checks || operations.some(n => !n)) throw new Error('math operation coverage is incomplete');
  return {checks, seed: reference.seed, seeds: reference.seeds, iterations: reference.iterations, limit: String(limit),
    operations, worst: worst.map(String), failed: failures.length, failures};
}

if (typeof process !== 'undefined' && process.versions?.node &&
    import.meta.url === (await import('node:url')).pathToFileURL(process.argv[1]).href) {
  const {readFile} = await import('node:fs/promises');
  const {createLocal} = await import('../../src/bits/wasm/host.mjs');
  const option = (name, fallback) => process.argv.find(x => x.startsWith(`--${name}=`))?.slice(name.length + 3) ?? fallback;
  const module = await WebAssembly.compile(await readFile(process.argv[2]));
  const threads = process.argv.includes('--threads');
  const memory = new WebAssembly.Memory({initial: 256, maximum: 8192, shared: threads});
  const runtime = await createLocal(module, memory, {worker: threads});
  const reference = JSON.parse(await readFile(option('reference', new URL('./math_reference.json', import.meta.url)), 'utf8'));
  try {
    const result = checkMath(runtime.exports, memory, reference);
    if (runtime.runMain() !== 1) throw new Error('math directed tests failed');
    console.log(JSON.stringify({status: result.failed ? 'FAIL' : 'PASS', ...result}));
    process.exitCode = result.failed ? 1 : 0;
  } finally { await runtime.shutdown(); }
}
