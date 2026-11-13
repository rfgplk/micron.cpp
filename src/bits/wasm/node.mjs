// Copyright (c) 2026 micron contributors.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt.

import { Worker, isMainThread, parentPort, workerData } from 'node:worker_threads';
import { readFile } from 'node:fs/promises';

if (isMainThread) {
  const [file, ...args] = process.argv.slice(2);
  const options = { file, threads: false, initialPages: 256, maximumPages: 8192, timeout: 30, minCases: 0 };
  try {
    if (!file || file.startsWith('-')) throw new Error('usage: node node.mjs module.wasm [--threads] [--timeout=seconds]');
    for (const arg of args) {
      if (arg === '--threads') { options.threads = true; continue; }
      const match = /^--(initial-pages|maximum-pages|timeout|min-cases)=([0-9]+)$/.exec(arg);
      if (!match) throw new Error(`unknown option: ${arg}`);
      const value = Number(match[2]);
      if (!Number.isSafeInteger(value) || value <= 0) throw new Error(`invalid option: ${arg}`);
      options[{ 'initial-pages': 'initialPages', 'maximum-pages': 'maximumPages', timeout: 'timeout', 'min-cases': 'minCases' }[match[1]]] = value;
    }
    const worker = new Worker(new URL(import.meta.url), { workerData: options });
    const timer = setTimeout(() => {
      process.stderr.write(`Micron WASM timed out after ${options.timeout}s\n`);
      process.exit(124);
    }, options.timeout * 1000);
    let completed = false;
    const finish = (code) => {
      if (completed) return;
      completed = true;
      clearTimeout(timer);
      Promise.all([process.stdout, process.stderr].map(stream => new Promise(resolve => stream.write('', resolve))))
        .then(() => process.exit(code));
    };
    worker.on('message', message => {
      if (message.type === 'write') {
        (message.stream === 2 ? process.stderr : process.stdout).write(message.text);
        return;
      }
      if (message.error) process.stderr.write(`${message.error}\n`);
      finish(message.code);
    });
    worker.on('error', error => { process.stderr.write(`${error.stack}\n`); finish(70); });
    worker.on('exit', code => { if (!completed) finish(code || 70); });
  } catch (error) {
    process.stderr.write(`${error.message}\n`);
    process.exit(64);
  }
} else {
  let runtime;
  try {
    const { createMicron } = await import('./host.mjs');
    runtime = await createMicron(await readFile(workerData.file), {
      ...workerData, onWrite: (text, stream) => parentPort.postMessage({type: 'write', text, stream})
    });
    const code = await runtime.runMain();
    if (workerData.minCases && code === 1) {
      const count = async name => {
        const value = await runtime.call(name);
        if (!Number.isInteger(value) || value < -0x80000000 || value > 0xffffffff)
          throw new Error(`invalid unsigned test coverage export ${name}: ${String(value)}`);
        return value >>> 0;
      };
      const cases = await count('__micron_wasm_test_cases');
      const skips = await count('__micron_wasm_test_skips');
      if (cases < workerData.minCases || skips !== 0)
        throw new Error(`Micron test coverage failed: ${cases} cases, ${skips} skips; expected >=${workerData.minCases} cases and no skips`);
    }
    await runtime.shutdown();
    if (!Number.isInteger(code) || code < 0 || code > 255) throw new Error(`invalid Micron exit code: ${code}`);
    parentPort.postMessage({ code });
  } catch (error) {
    try { await runtime?.terminate(); } catch {}
    parentPort.postMessage({ code: error.name === 'MicronExit' && Number.isInteger(error.code) ? error.code : 70,
                             error: error.stack || String(error) });
  }
}
