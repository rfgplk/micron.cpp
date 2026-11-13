// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
import {createMicron} from '../../src/bits/wasm/host.mjs';

export async function checkUtf8(runtime, output, threads = false) {
  let start = output.length;
  const collected = stream => output.slice(start).filter(x => x.stream === stream).map(x => x.text).join('');
  const expect = (stream, expected) => {
    const actual = collected(stream);
    if (actual !== expected) throw new Error(`UTF-8 stream ${stream}: ${JSON.stringify(actual)} != ${JSON.stringify(expected)}`);
  };
  const write = async (byte, stream) => {
    if (await runtime.call('test_write_byte', byte, stream) !== 1) throw new Error('Byte write failed');
  };
  await write(0xe2, 1);
  await write(0xf0, 2);
  await write(0x82, 1);
  await write(0x9f, 2);
  expect(1, ''); expect(2, '');
  await write(0xac, 1);
  await write(0x98, 2);
  expect(1, '€'); expect(2, '');
  await write(0x80, 2);
  expect(1, '€'); expect(2, '😀');
  if (threads) {
    start = output.length;
    if (await runtime.call('test_utf8_workers') !== 1) throw new Error('Worker UTF-8 writes failed');
    expect(1, '€�J���K');
    expect(2, '���');
  }
  start = output.length;
  await write(0xe2, 1);
  await write(0xf0, 2);
  expect(1, ''); expect(2, '');
  await runtime.shutdown();
  expect(1, '�'); expect(2, '�');
  const flushed = output.length;
  await runtime.shutdown();
  if (output.length !== flushed) throw new Error('Repeated shutdown flushed UTF-8 twice');
  return {streams: 2, workerReuse: threads, shutdownFlush: true};
}

if (typeof process !== 'undefined' && process.versions?.node &&
    import.meta.url === (await import('node:url')).pathToFileURL(process.argv[1]).href) {
  if (!process.argv[2]) throw new Error('Usage: runtime_utf8.mjs host_probe.wasm [--threads]');
  const threads = process.argv.includes('--threads');
  const output = [];
  const runtime = await createMicron(process.argv[2], {threads, onWrite: (text, stream) => output.push({text, stream})});
  try {
    if (await runtime.runMain() !== 1) throw new Error('Main failed');
    await checkUtf8(runtime, output, threads);
    console.log(`PASS UTF-8 streaming ${threads ? 'shared' : 'single'}`);
  } finally { await runtime.terminate(); }
}
