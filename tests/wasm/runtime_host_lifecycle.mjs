// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
import {createMicron as defaultCreateMicron} from '../../src/bits/wasm/host.mjs';

const defaultSeeds = [0x726a8b31, 0x9e3779b9, 0x1badf00d];

export function deadline(promise, milliseconds = 3000, label = 'host operation') {
  let timer;
  return Promise.race([promise, new Promise((_, reject) => {
    timer = setTimeout(() => reject(new Error(`${label} timed out`)), milliseconds);
  })]).finally(() => clearTimeout(timer));
}

function observe(operation) {
  try {
    return Promise.resolve(operation()).then(value => ({status: 'fulfilled', value}), reason => ({status: 'rejected', reason}));
  } catch (reason) { return Promise.resolve({status: 'rejected', reason}); }
}

function random(seed) {
  let state = seed >>> 0;
  return () => {
    state ^= state << 13; state ^= state >>> 17; state ^= state << 5;
    return state >>> 0;
  };
}

export async function checkShutdownRace(runtime, milliseconds = 3000) {
  const first = observe(() => runtime.shutdown());
  const second = observe(() => runtime.shutdown());
  const late = observe(() => runtime.runMain());
  const rows = await deadline(Promise.all([first, second, late]), milliseconds, 'concurrent shutdown');
  if (rows[0].status !== 'fulfilled' || rows[0].value !== 0 || rows[1].status !== 'fulfilled' || rows[1].value !== 0)
    throw new Error('Concurrent shutdown did not succeed');
  if (rows[2].status !== 'rejected' || !/shutting down|has stopped/.test(rows[2].reason.message))
    throw new Error('Call issued after shutdown began was accepted');
  return {shutdowns: 2, refusedCalls: 1};
}

export async function checkHostLifecycle(source, options = {}) {
  const createMicron = options.createMicron ?? defaultCreateMicron;
  const threads = options.threads ?? false;
  const milliseconds = options.deadlineMs ?? 3000;
  const rounds = options.rounds ?? 12;
  const seeds = [...(options.seeds ?? defaultSeeds)];
  if (!Number.isInteger(rounds) || rounds < 1) throw new Error('rounds must be a positive integer');
  if (!seeds.length || seeds.some(seed => !Number.isInteger(seed) || seed < 1 || seed > 0xffffffff))
    throw new Error('seeds must be nonzero uint32 values');
  if (!Number.isFinite(milliseconds) || milliseconds <= 0) throw new Error('deadlineMs must be positive');
  const selected = options.cases ? new Set(options.cases) : null;
  const result = {threads, assertions: 0, operations: 0, runtimes: 0, seededRounds: 0, negativeControls: 0,
    cancellationRequests: 0, cancelledJoins: 0, cleanupFaults: 0, seeds, scenarios: []};
  let context = '';
  function require(condition, message) {
    ++result.assertions;
    if (!condition) throw new Error(`${context}: ${message}`);
  }
  async function settled(promises, label) { return deadline(Promise.all(promises), milliseconds, `${context} ${label}`); }
  function success(row, value, label) {
    require(row.status === 'fulfilled' && row.value === value,
      `${label}: ${row.status === 'fulfilled' ? row.value : row.reason}`);
  }
  function refused(row, label, code) {
    require(row.status === 'rejected' && (code === undefined || row.reason.code === code),
      `${label}: ${row.status === 'fulfilled' ? `accepted (${row.value})` : row.reason}`);
  }
  async function run(name, operation, applicable = true) {
    if (!applicable || (selected && !selected.has(name))) return;
    context = name;
    await operation();
    result.scenarios.push(name);
  }
  async function withRuntime(operation, extra = {}) {
    const runtime = await deadline(createMicron(source, {threads, poolSize: 2, onWrite() {}, ...extra}), milliseconds, `${context} create`);
    ++result.runtimes;
    try {
      require(await deadline(Promise.resolve(runtime.runMain()), milliseconds, `${context} main`) === 1, 'main failed');
      return await operation(runtime);
    } finally { await deadline(Promise.resolve(runtime.terminate()), milliseconds, `${context} cleanup`); }
  }
  async function stopped(runtime) {
    const rows = await settled([observe(() => runtime.call('lifecycle_step', 1)), observe(() => runtime.runMain())], 'stopped calls');
    for (const row of rows) refused(row, 'Stopped runtime call');
    require(await runtime.shutdown() === 0 && await runtime.shutdown() === 0, 'Stopped shutdown is not idempotent');
  }
  async function finalizedOnce(runtime, address) {
    require(new DataView(runtime.memory.buffer).getUint32(address, true) === 1, 'Destructor did not run exactly once');
  }
  async function panicStatus(runtime) {
    const eh = await runtime.call('lifecycle_eh');
    require(eh === 0 || eh === 1, 'Invalid lifecycle EH profile');
    // EH cleanup crosses a noexcept boundary before returning to the host.
    return eh ? 134 : 57;
  }
  async function waitEntered(runtime, address, word = 0) {
    const until = performance.now() + milliseconds;
    while (Atomics.load(new Int32Array(runtime.memory.buffer, address + word * 4, 1), 0) !== 1) {
      if (performance.now() >= until) throw new Error(`${context}: primary worker did not enter wait`);
      await new Promise(resolve => setTimeout(resolve, 0));
    }
  }

  await run('controls', async () => {
    const timed = await observe(() => deadline(new Promise(() => {}), 5, 'negative control'));
    require(timed.status === 'rejected' && /negative control timed out/.test(timed.reason.message), 'Deadline negative control did not fail');
    require(await deadline(Promise.resolve(19), milliseconds) === 19, 'Deadline positive control failed');
    ++result.negativeControls;
    const mismatch = await observe(() => success({status: 'fulfilled', value: 2}, 1, 'oracle negative control'));
    require(mismatch.status === 'rejected' && /oracle negative control/.test(mismatch.reason.message), 'Result oracle accepted corruption');
    ++result.negativeControls;
  });
  for (const [name, mode] of [['shutdown-trap', 1], ['shutdown-panic', 2]]) await run(name, () => withRuntime(async runtime => {
    const address = await runtime.call('lifecycle_finalized');
    const expectedPanic = mode === 2 ? await panicStatus(runtime) : 0;
    await runtime.call('lifecycle_shutdown_fault', mode);
    const close = await deadline(observe(() => runtime.shutdown()), milliseconds, name);
    refused(close, 'Faulting shutdown');
    if (mode === 2) require(close.reason.name === 'MicronExit' && close.reason.code === expectedPanic, 'Unexpected panic status');
    await stopped(runtime);
    await finalizedOnce(runtime, address);
  }));
  await run('shutdown-race', () => withRuntime(async runtime => {
    const address = await runtime.call('lifecycle_finalized');
    const rows = [];
    let checksum = 0;
    for (let i = 0; i < 64; ++i) {
      checksum = (Math.imul(checksum, 1664525) + i + 1013904223) >>> 0;
      rows.push({expected: checksum, promise: observe(() => runtime.call('lifecycle_step', i))});
    }
    const closing = observe(() => checkShutdownRace(runtime, milliseconds));
    const replies = await settled(rows.map(row => row.promise), 'queued calls');
    replies.forEach((reply, i) => success({...reply, value: reply.value >>> 0}, rows[i].expected, 'Call queued before shutdown'));
    const closed = await closing;
    require(closed.status === 'fulfilled', `Concurrent shutdown failed: ${closed.reason}`);
    result.assertions += 3;
    await stopped(runtime);
    await finalizedOnce(runtime, address);
  }));
  await run('busy-retry', () => withRuntime(async runtime => {
    for (let i = 0; i < 8; ++i) {
      require(await runtime.call('lifecycle_start') === 0, 'Worker start failed');
      const closes = [observe(() => runtime.shutdown()), observe(() => runtime.shutdown())];
      const late = observe(() => runtime.runMain());
      for (const row of await settled(closes, 'busy shutdowns')) refused(row, 'Live worker shutdown', -16);
      refused(await late, 'Call during busy shutdown', -16);
      require(await runtime.call('lifecycle_release') === 1, 'Worker release failed after refused shutdown');
      refused(await observe(() => runtime.shutdown()), 'Unjoined worker shutdown', -16);
      require(await runtime.call('lifecycle_finish') === 1, 'Worker join failed after refused shutdown');
      refused(await observe(() => runtime.call('missing_export')), 'Missing export', -38);
    }
    await checkShutdownRace(runtime, milliseconds);
    result.assertions += 3;
  }), threads);
  await run('terminate-pending', () => withRuntime(async runtime => {
    const address = await runtime.call('lifecycle_control');
    const calls = [observe(() => runtime.call('lifecycle_block'))];
    await waitEntered(runtime, address);
    for (let i = 0; i < 64; ++i) calls.push(observe(() => runtime.call('lifecycle_step', i)));
    calls.push(observe(() => runtime.runMain()), observe(() => runtime.shutdown()), observe(() => runtime.shutdown()));
    const late = observe(() => runtime.call('lifecycle_step', 99));
    const terms = [observe(() => runtime.terminate()), observe(() => runtime.terminate())];
    for (const row of await settled(calls, 'terminated pending requests')) refused(row, 'Pending request survived termination');
    refused(await late, 'Call during shutdown', -16);
    for (const row of await settled(terms, 'repeated termination')) success(row, undefined, 'Terminate');
    await stopped(runtime);
  }), threads);
  await run('terminate-shutdown', () => withRuntime(async runtime => {
    const closing = observe(() => runtime.shutdown());
    const ending = [observe(() => runtime.terminate()), observe(() => runtime.terminate())];
    const row = await deadline(closing, milliseconds, 'shutdown interrupted by terminate');
    if (threads) refused(row, 'Interrupted shutdown'); else success(row, 0, 'Local shutdown');
    for (const term of await settled(ending, 'concurrent termination')) success(term, undefined, 'Terminate');
    await stopped(runtime);
  }));
  await run('trap-pending', () => withRuntime(async runtime => {
    const calls = [observe(() => runtime.call('lifecycle_trap'))];
    for (let i = 0; i < 64; ++i) calls.push(observe(() => runtime.call('lifecycle_step', i)));
    calls.push(observe(() => runtime.runMain()), observe(() => runtime.shutdown()));
    const rows = await settled(calls, 'trapped pending requests');
    rows.slice(0, -1).forEach(row => refused(row, 'Call survived trap'));
    if (threads) refused(rows.at(-1), 'Queued shutdown survived trap'); else success(rows.at(-1), 0, 'Stopped local shutdown');
    await stopped(runtime);
  }));
  await run('flush-failure', () => withRuntime(async runtime => {
    const address = await runtime.call('lifecycle_finalized');
    require(await runtime.call('lifecycle_write_byte', 0xe2, 1) === 1, 'Partial UTF-8 write failed');
    const row = await deadline(observe(() => runtime.shutdown()), milliseconds, 'flush error shutdown');
    refused(row, 'Output callback failure');
    require(/deliberate flush failure/.test(row.reason.message), 'Output callback error was lost');
    await stopped(runtime);
    await finalizedOnce(runtime, address);
  }, {onWrite() { throw new Error('deliberate flush failure'); }}));
  await run('reentrant-shutdown', async () => {
    let current;
    const callbacks = [];
    await withRuntime(async runtime => {
      current = runtime;
      await runtime.call('lifecycle_shutdown_fault', 3);
      require(await runtime.shutdown() === 0, 'Outer shutdown failed');
      require(callbacks.length === 3, 'Destructor output callback was not observed');
      const rows = await settled(callbacks, 'reentrant calls');
      refused(rows[0], 'Reentrant export');
      refused(rows[1], 'Reentrant main');
      if (threads) success(rows[2], 0, 'Reentrant shared shutdown'); else refused(rows[2], 'Reentrant local shutdown', -16);
    }, {onWrite() {
      callbacks.push(observe(() => current.call('lifecycle_step', 77)), observe(() => current.runMain()), observe(() => current.shutdown()));
    }});
  });
  await run('clone-failure', () => withRuntime(async runtime => {
    const bad = Array.from({length: 64}, () => observe(() => runtime.call('lifecycle_step', () => 1)));
    for (const row of await settled(bad, 'invalid RPC arguments')) {
      refused(row, 'Non-cloneable RPC argument');
      require(row.reason.name === 'DataCloneError', 'Unexpected clone failure');
    }
    require((await runtime.call('lifecycle_step', 0) >>> 0) === 1013904223, 'Rejected RPC executed or poisoned the runtime');
  }), threads);
  await run('cancellation-reuse', () => withRuntime(async runtime => {
    const address = await runtime.call('lifecycle_cancel_control');
    for (let i = 0; i < 16; ++i) {
      require(await runtime.call('lifecycle_cancel_start', 0) === 0, 'Cancellation worker start');
      await waitEntered(runtime, address);
      require(await runtime.call('lifecycle_cancel_request') === 0, 'Cancellation request');
      ++result.cancellationRequests;
      refused(await observe(() => runtime.shutdown()), 'Shutdown accepted a cancelled but unjoined worker', -16);
      require(await deadline(Promise.resolve(runtime.call('lifecycle_cancel_finish')), milliseconds, 'cancellation handoff') === 1,
        'Cancellation cleanup or result failed');
      ++result.cancelledJoins;
      require(Atomics.load(new Int32Array(runtime.memory.buffer, address, 4), 3) === 1, 'Cancellation TLS was reset or destroyed twice');
      require(await runtime.call('lifecycle_start') === 0 && await runtime.call('lifecycle_finish') === 1, 'Worker pool did not recover after cancellation');
    }
    await checkShutdownRace(runtime, milliseconds); result.assertions += 3;
  }, {poolSize: 1}), threads);
  for (const [name, mode] of [['cancellation-cleanup-trap', 1], ['cancellation-cleanup-panic', 2]])
    await run(name, () => withRuntime(async runtime => {
      const address = await runtime.call('lifecycle_cancel_control');
      const expectedPanic = mode === 2 ? await panicStatus(runtime) : 0;
      require(await runtime.call('lifecycle_cancel_start', mode) === 0, 'Cancellation worker start');
      await waitEntered(runtime, address);
      require(await runtime.call('lifecycle_cancel_request') === 0, 'Cancellation request');
      ++result.cancellationRequests;
      const pending = [observe(() => runtime.call('lifecycle_cancel_finish'))];
      for (let i = 0; i < 64; ++i) pending.push(observe(() => runtime.call('lifecycle_step', i)));
      pending.push(observe(() => runtime.shutdown()));
      const rows = await settled(pending, 'failed replacement cleanup');
      for (const row of rows) refused(row, 'Request survived cleanup failure');
      if (mode === 2) require(rows[0].reason.name === 'MicronExit' && rows[0].reason.code === expectedPanic, 'Unexpected cleanup panic status');
      require(Atomics.load(new Int32Array(runtime.memory.buffer, address, 4), 3) === 1, 'Replacement lost preserved TLS');
      ++result.cleanupFaults;
      await stopped(runtime);
    }, {poolSize: 1}), threads);
  await run('terminate-cleanup', () => withRuntime(async runtime => {
    const address = await runtime.call('lifecycle_cancel_control');
    require(await runtime.call('lifecycle_cancel_start', 3) === 0, 'Cancellation worker start');
    await waitEntered(runtime, address);
    require(await runtime.call('lifecycle_cancel_request') === 0, 'Cancellation request');
    ++result.cancellationRequests;
    const pending = [observe(() => runtime.call('lifecycle_cancel_finish'))];
    await waitEntered(runtime, address, 2);
    for (let i = 0; i < 64; ++i) pending.push(observe(() => runtime.call('lifecycle_step', i)));
    pending.push(observe(() => runtime.shutdown()));
    const terms = [observe(() => runtime.terminate()), observe(() => runtime.terminate())];
    for (const row of await settled(pending, 'termination during replacement cleanup')) refused(row, 'Request survived cleanup termination');
    for (const row of await settled(terms, 'replacement termination')) success(row, undefined, 'Terminate');
    await stopped(runtime);
  }, {poolSize: 1}), threads);

  await run('seeded-fuzz', async () => {
    for (const seed of seeds) {
      const next = random(seed);
      for (let round = 0; round < rounds; ++round) {
        context = `seeded-fuzz seed=0x${seed.toString(16)} round=${round}`;
        const output = [], expected = new Map(), decoders = new Map();
        let checksum = 0, worker = 0;
        await withRuntime(async runtime => {
          const steps = 64 + next() % 128;
          const cancelAddress = threads ? await runtime.call('lifecycle_cancel_control') : 0;
          for (let step = 0; step < steps; ++step) {
            ++result.operations;
            const action = next() % 8;
            if (action < 3) {
              const batch = [];
              for (let i = 1 + next() % 8; i; --i) {
                const value = next();
                checksum = (Math.imul(checksum, 1664525) + value + 1013904223) >>> 0;
                batch.push({value: checksum, reply: observe(() => runtime.call('lifecycle_step', value))});
              }
              const rows = await settled(batch.map(item => item.reply), 'checksum batch');
              rows.forEach((row, i) => success({...row, value: row.value >>> 0}, batch[i].value, 'Checksum oracle'));
            } else if (action === 3) {
              refused(await observe(() => runtime.call('missing_export')), 'Missing export', -38);
            } else if (action === 4) {
              require(await runtime.runMain() === 1, 'Main changed before shutdown');
            } else if (action === 5 && threads) {
              if (worker === 0) {
                if ((next() & 3) === 0) {
                  require(await runtime.call('lifecycle_cancel_start', 0) === 0, 'Cancellation worker start');
                  await waitEntered(runtime, cancelAddress); worker = 3;
                } else { require(await runtime.call('lifecycle_start') === 0, 'Worker start'); worker = 1; }
              }
              else if (worker === 1) { require(await runtime.call('lifecycle_release') === 1, 'Worker completion'); worker = 2; }
              else if (worker === 2) { require(await runtime.call('lifecycle_finish') === 1, 'Worker join'); worker = 0; }
              else if (worker === 3) {
                require(await runtime.call('lifecycle_cancel_request') === 0, 'Cancellation request');
                ++result.cancellationRequests; worker = 4;
              }
              else {
                require(await runtime.call('lifecycle_cancel_finish') === 1, 'Cancellation join');
                ++result.cancelledJoins;
                require(Atomics.load(new Int32Array(runtime.memory.buffer, cancelAddress, 4), 3) === 1, 'Cancellation TLS oracle');
                worker = 0;
              }
            } else if (action === 6 && worker) {
              const closes = [observe(() => runtime.shutdown()), observe(() => runtime.shutdown())];
              const late = observe(() => runtime.call('lifecycle_step', next()));
              for (const row of await settled(closes, 'retryable fuzz shutdown')) refused(row, 'Busy shutdown', -16);
              refused(await late, 'Call during shutdown', -16);
            } else {
              const byte = next() & 255, stream = 1 + (next() & 1);
              if (!decoders.has(stream)) decoders.set(stream, new TextDecoder());
              expected.set(stream, (expected.get(stream) ?? '') + decoders.get(stream).decode(new Uint8Array([byte]), {stream: true}));
              require(await runtime.call('lifecycle_write_byte', byte, stream) === 1, 'UTF-8 byte write');
            }
          }
          for (const [stream, text] of expected)
            require(output.filter(item => item.stream === stream).map(item => item.text).join('') === text, 'Streaming UTF-8 oracle');
          const ending = next() % 4;
          if (ending < 2 && worker) {
            if (worker < 3) require(await runtime.call('lifecycle_finish') === 1, 'Final worker join');
            else {
              if (worker === 3) {
                require(await runtime.call('lifecycle_cancel_request') === 0, 'Final cancellation request');
                ++result.cancellationRequests;
              }
              require(await runtime.call('lifecycle_cancel_finish') === 1, 'Final cancellation join');
              ++result.cancelledJoins;
            }
          }
          if (ending === 0) {
            await checkShutdownRace(runtime, milliseconds); result.assertions += 3;
            for (const [stream, decoder] of decoders) {
              const text = (expected.get(stream) ?? '') + decoder.decode();
              require(output.filter(item => item.stream === stream).map(item => item.text).join('') === text, 'Final UTF-8 oracle');
            }
          } else if (ending === 1) {
            await runtime.call('lifecycle_shutdown_fault', 1);
            refused(await deadline(observe(() => runtime.shutdown()), milliseconds, 'fuzz teardown trap'), 'Trapping teardown');
          } else if (ending === 2) {
            const pending = [observe(() => runtime.call('lifecycle_trap'))];
            for (let i = 0; i < 8; ++i) pending.push(observe(() => runtime.call('lifecycle_step', next())));
            for (const row of await settled(pending, 'fuzz trapped requests')) refused(row, 'Request survived trap');
          } else {
            const terms = [observe(() => runtime.terminate()), observe(() => runtime.terminate())];
            for (const row of await settled(terms, 'fuzz repeated terminate')) success(row, undefined, 'Terminate');
          }
          await stopped(runtime);
        }, {poolSize: 1 + (next() & 1), onWrite: (text, stream) => output.push({text, stream})});
        ++result.seededRounds;
      }
    }
  });
  if (selected) for (const name of selected) require(result.scenarios.includes(name), `Unknown or unavailable case: ${name}`);
  return result;
}

if (typeof process !== 'undefined' && process.versions?.node && process.argv[1] &&
    import.meta.url === (await import('node:url')).pathToFileURL(process.argv[1]).href) {
  const [source, ...args] = process.argv.slice(2);
  if (!source) throw new Error('Usage: runtime_host_lifecycle.mjs runtime_lifecycle.wasm [--threads] [--rounds=N] [--seeds=hex,...] [--cases=name,...] [--host=URL]');
  const options = {threads: false};
  for (const arg of args) {
    if (arg === '--threads') options.threads = true;
    else if (arg.startsWith('--rounds=')) options.rounds = Number(arg.slice(9));
    else if (arg.startsWith('--seeds=')) options.seeds = arg.slice(8).split(',').map(value => Number(value));
    else if (arg.startsWith('--cases=')) options.cases = arg.slice(8).split(',');
    else if (arg.startsWith('--host=')) options.createMicron = (await import(new URL(arg.slice(7), import.meta.url))).createMicron;
    else throw new Error(`Unknown option: ${arg}`);
  }
  const {readFile} = await import('node:fs/promises');
  const module = await WebAssembly.compile(await readFile(source));
  console.log(JSON.stringify(await checkHostLifecycle(module, options)));
}
