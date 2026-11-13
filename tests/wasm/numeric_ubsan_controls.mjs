// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
import {createLocal} from '../../src/bits/wasm/host.mjs';

export async function checkNumericUbsanControls(source, {threads = false} = {}) {
  const module = source instanceof WebAssembly.Module ? source : await WebAssembly.compile(source);
  const report = {status: 'PASS', threads, checks: 0, validCases: 0, guardedCases: 0, trapCases: 0};
  const wrap = value => BigInt.asUintN(128, value);
  const sign = 1n << 127n, mask64 = (1n << 64n) - 1n;
  const patterns = [0n, 1n, sign - 1n, sign, wrap(-1n), 0x0123456789abcdeffedcba9876543210n];
  const invalid = [-2147483648n, -1024n, -129n, -128n, -127n, -65n, -64n, -63n, -1n,
    128n, 129n, 255n, 256n, 1024n, 2147483647n];
  function require(condition, message) {
    ++report.checks;
    if (!condition) throw new Error(message);
  }
  async function instance() {
    const memory = new WebAssembly.Memory({initial: 256, maximum: 8192, shared: threads});
    const runtime = await createLocal(module, memory, {worker: threads});
    const e = runtime.exports, pointer = e.numeric_buffer();
    function probe(op, a, shift) {
      // A previous call can grow memory; acquire a current view for each probe.
      const data = new DataView(memory.buffer), count = wrap(shift);
      data.setBigUint64(pointer, a & mask64, true);
      data.setBigUint64(pointer + 8, a >> 64n, true);
      data.setBigUint64(pointer + 16, count & mask64, true);
      data.setBigUint64(pointer + 24, count >> 64n, true);
      e.numeric_probe(op);
      const output = new DataView(memory.buffer);
      return output.getBigUint64(pointer + 48, true) | output.getBigUint64(pointer + 56, true) << 64n;
    }
    return {runtime, probe};
  }
  try {
    const {runtime, probe} = await instance();
    try {
      for (const a of patterns) {
        for (const shift of [...Array.from({length: 128}, (_, i) => BigInt(i)), ...invalid]) {
          const valid = shift >= 0n && shift < 128n;
          const expected = valid ? [wrap(a << shift), a >> shift, wrap(BigInt.asIntN(128, a) >> shift)] : [0n, 0n, 0n];
          for (let kind = 0; kind < 3; ++kind) {
            if (valid) {
              const op = 33 + kind;
              require(probe(op, a, shift) === expected[kind], `valid op${op} a=${a.toString(16)} shift=${shift}`);
              ++report.validCases;
            }
            const op = 36 + kind;
            require(probe(op, a, shift) === expected[kind], `guarded op${op} a=${a.toString(16)} shift=${shift}`);
            ++report.guardedCases;
          }
        }
      }
      require(runtime.runMain() === 1, 'directed numeric suite did not return the PASS sentinel');
      await runtime.shutdown();
    } finally { await runtime.terminate(); }

    // A trap abandons the linear stack pointer. Every negative control gets a
    // fresh instance, and its stopped runtime is never reentered for cleanup.
    for (const op of [33, 34, 35]) for (const shift of [-2147483648n, -1n, 128n, 255n]) {
      const {runtime, probe} = await instance();
      let trapped;
      try {
        try { probe(op, 0x8123456789abcdeffedcba9876543210n, shift); }
        catch (error) { trapped = error; }
        require(trapped instanceof WebAssembly.RuntimeError && /unreachable/i.test(trapped.message),
          `op${op} shift=${shift} must trigger source UBSan; compile with --wasm-ubsan-traps`);
        ++report.trapCases;
      } finally { await runtime.terminate(); }
    }
    require(report.validCases === 2304 && report.guardedCases === 2574 && report.trapCases === 12,
      'numeric UBSan control coverage is incomplete');
    return report;
  } catch (error) {
    error.numericUbsanReport = {...report, status: 'FAIL'};
    throw error;
  }
}

if (typeof process !== 'undefined' && process.versions?.node && process.argv[1] &&
    import.meta.url === (await import('node:url')).pathToFileURL(process.argv[1]).href) {
  try {
    const {readFile} = await import('node:fs/promises');
    const [file, ...args] = process.argv.slice(2);
    if (!file || file.startsWith('-') || args.some(arg => arg !== '--threads') || args.length > 1)
      throw new Error('usage: node numeric_ubsan_controls.mjs module.wasm [--threads]');
    console.log(JSON.stringify(await checkNumericUbsanControls(await readFile(file), {threads: args.includes('--threads')})));
  } catch (error) {
    console.error(JSON.stringify({...error.numericUbsanReport, status: 'FAIL', error: error.message,
      cause: error.cause?.message}));
    process.exitCode = 1;
  }
}
