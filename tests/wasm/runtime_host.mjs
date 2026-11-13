// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
import {createMicron, bytes} from '../../src/bits/wasm/host.mjs';

const encode = new TextEncoder();
const section = (id, payload) => [id, payload.length, ...payload];
const name = text => [text.length, ...encode.encode(text)];
const moduleWithVersion = version => new Uint8Array([
  0, 97, 115, 109, 1, 0, 0, 0,
  ...section(1, [1, 0x60, 0, 1, 0x7f]),
  ...section(3, [1, 0]),
  ...section(7, [1, ...name('__micron_wasm_abi_version'), 0, 0]),
  ...section(10, [1, 4, 0, 0x41, version, 0x0b])
]);
let assertions = 0;
async function rejects(operation, pattern) {
  try { await operation(); }
  catch (error) {
    if (!pattern.test(error.message)) throw error;
    ++assertions;
    return;
  }
  throw new Error(`Expected rejection matching ${pattern}`);
}

await rejects(() => createMicron(new Uint8Array([0, 97, 115, 109, 1, 0, 0, 0])), /missing.*ABI version/);
await rejects(() => createMicron(moduleWithVersion(2)), /version 2 is unsupported/);
await rejects(() => createMicron(moduleWithVersion(1)), /missing.*startup exports/);
const memory = new WebAssembly.Memory({initial: 1, maximum: 2});
await rejects(() => createMicron(moduleWithVersion(1), {threads: true, memory}), /shared-memory profile/);
await rejects(() => bytes(memory, -1, 4), /outside linear memory/);
await rejects(() => bytes(memory, 65535, 2), /outside linear memory/);
await rejects(() => bytes(memory, 0.5, 1), /outside linear memory/);
await rejects(() => bytes(memory, 0, NaN), /outside linear memory/);
if (bytes(memory, 65536, 0).length !== 0) throw new Error('Empty end-of-memory view rejected');
console.log(`PASS runtime host ABI and bounds: ${assertions + 1} assertions`);
