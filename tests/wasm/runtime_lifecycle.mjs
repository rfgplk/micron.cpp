// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
import {createMicron, MicronStateError} from '../../src/bits/wasm/host.mjs';
import {checkShutdownRace} from './runtime_host_lifecycle.mjs';

export async function checkLifecycle(runtime, threads = false) {
  const address = await runtime.call('lifecycle_finalized');
  try { await runtime.call('missing_export'); throw new Error('Missing export was accepted'); }
  catch (error) { if (!(error instanceof MicronStateError) || error.code !== -38) throw error; }
  if (threads) {
    if (await runtime.call('lifecycle_start') !== 0) throw new Error('Worker failed to start');
    try { await runtime.shutdown(); throw new Error('Shutdown accepted a live worker'); }
    catch (error) { if (!(error instanceof MicronStateError) || error.code !== -16) throw error; }
    if (await runtime.call('lifecycle_release') !== 1) throw new Error('Worker did not complete');
    try { await runtime.shutdown(); throw new Error('Shutdown accepted an unjoined worker'); }
    catch (error) { if (!(error instanceof MicronStateError) || error.code !== -16) throw error; }
    if (await runtime.call('lifecycle_finish') !== 1) throw new Error('Worker did not finish after rejected shutdown');
  }
  await checkShutdownRace(runtime);
  if (new DataView(runtime.memory.buffer).getUint32(address, true) !== 1) throw new Error('Destructor did not run once');
  if (await runtime.shutdown() !== 0) throw new Error('Repeated shutdown failed');
  try { await runtime.runMain(); throw new Error('Stopped runtime accepted main'); }
  catch (error) { if (!/has stopped/.test(error.message)) throw error; }
  return {refusals: threads ? 2 : 0, finalized: 1};
}

if (typeof process !== 'undefined' && process.versions?.node &&
    import.meta.url === (await import('node:url')).pathToFileURL(process.argv[1]).href) {
  if (!process.argv[2]) throw new Error('Usage: runtime_lifecycle.mjs module.wasm [--threads]');
  const threads = process.argv.includes('--threads');
  const runtime = await createMicron(process.argv[2], {threads});
  try {
    if (await runtime.runMain() !== 1) throw new Error('Main failed');
    await checkLifecycle(runtime, threads);
    console.log(`PASS runtime lifecycle ${threads ? 'shared' : 'single'}`);
  } finally { await runtime.terminate(); }
}
