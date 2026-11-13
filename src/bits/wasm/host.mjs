// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
const node = typeof process !== "undefined" && Boolean(process.versions?.node);
const workerURL = new URL("./worker.mjs", import.meta.url);
const nodeWorkers = node ? await import("node:worker_threads") : null;
const WorkerClass = node ? nodeWorkers.Worker : globalThis.Worker;

export class MicronExit extends Error {
  constructor(code) { super(`Micron stopped with status ${code}`); this.name = "MicronExit"; this.code = code; }
}
export class MicronStateError extends Error {
  constructor(message, code) { super(message); this.name = "MicronStateError"; this.code = code; }
}
export function bytes(memory, offset = 0, length = memory.buffer.byteLength - offset) {
  if (!Number.isInteger(offset) || !Number.isInteger(length) || offset < 0 || length < 0 ||
      offset > memory.buffer.byteLength || length > memory.buffer.byteLength - offset)
    throw new RangeError("Micron memory range is outside linear memory");
  return new Uint8Array(memory.buffer, offset, length);
}
function listen(worker, fn) {
  if (node) worker.on("message", fn); else worker.addEventListener("message", e => fn(e.data));
}
function failure(worker, fn) {
  if (node) worker.on("error", fn); else worker.addEventListener("error", fn);
}
function newWorker() {
  return new WorkerClass(workerURL, {type: "module", ...(node ? {execArgv: []} : {})});
}
async function compile(source) {
  if (source instanceof WebAssembly.Module) return source;
  if (source instanceof Uint8Array || source instanceof ArrayBuffer) return WebAssembly.compile(source);
  if (typeof Response !== "undefined" && source instanceof Response)
    return WebAssembly.compile(await source.arrayBuffer());
  if (node && (source instanceof URL ? source.protocol === "file:" : !/^https?:/.test(String(source)))) {
    const fs = await import("node:fs/promises");
    return WebAssembly.compile(await fs.readFile(source));
  }
  const response = await fetch(source);
  if (!response.ok) throw new Error(`Micron fetch failed: ${response.status}`);
  return WebAssembly.compile(await response.arrayBuffer());
}

// Every callback builds a current view: memory.grow invalidates ordinary views and
// older shared views retain their old length.
export async function createLocal(module, memory, options = {}) {
  const children = new Set();
  const output = options.onWrite ?? ((text, stream) => {
    if (node) (stream === 2 ? process.stderr : process.stdout).write(text);
    else (stream === 2 ? console.error : console.log)(text);
  });
  const decoders = new Map();
  function flushOutput() {
    for (const [stream, decoder] of decoders) {
      const text = decoder.decode();
      if (text) output(text, stream);
    }
  }
  const imports = {
    env: {memory},
    micron: {
      write(pointer, length, stream) {
        try {
          const data = bytes(memory, pointer >>> 0, length >>> 0);
          // TextDecoder's BufferSource excludes shared buffers in some browsers.
          const input = typeof SharedArrayBuffer !== "undefined" && data.buffer instanceof SharedArrayBuffer
            ? new Uint8Array(data) : data;
          let decoder = decoders.get(stream);
          if (!decoder) { decoder = new TextDecoder(); decoders.set(stream, decoder); }
          const text = decoder.decode(input, {stream: true});
          if (text) output(text, stream);
          return length;
        }
        catch { return -14; }
      },
      random(pointer, length) {
        try {
          const out = bytes(memory, pointer >>> 0, length >>> 0);
          const scratch = new Uint8Array(Math.min(out.length, 65536));
          for (let i = 0; i < out.length; i += 65536) {
            const chunk = scratch.subarray(0, Math.min(65536, out.length - i));
            crypto.getRandomValues(chunk);
            out.set(chunk, i);
          }
          return 0;
        } catch { return -5; }
      },
      clock(kind) {
        if (kind === 0) return BigInt(Date.now()) * 1000000n;
        if (kind === 1) return BigInt(Math.floor((performance.timeOrigin + performance.now()) * 1e6));
        if (kind === 2) return 1000000n;
        if (kind === 3) return 1000000n;
        return -95n;
      },
      panic(code) { throw new MicronExit(code); },
      thread_exit() {
        if (!options.threadExit) throw new Error("Micron worker exit is unavailable");
        options.threadExit();
        throw new Error("Micron worker exit handler returned");
      },
      yield() {
        if (options.scheduler) Atomics.wait(options.scheduler, 0, 0, 0);
      },
      worker_spawn(slot, stack, tls, control) {
        if (!options.scheduler || Atomics.load(options.scheduler, 0)) return -95;
        for (let i = 1; i < options.scheduler.length; ++i) {
          if (Atomics.compareExchange(options.scheduler, i, 0, 1) !== 0) continue;
          options.dispatch({type: "spawn", pool: i - 1, slot, stack, tls, control});
          return 0;
        }
        return -11;
      }
    }
  };
  const instance = await WebAssembly.instantiate(module, imports);
  const e = instance.exports;
  if (typeof e.__micron_wasm_abi_version !== "function")
    throw new Error("Micron module is missing its host ABI version export");
  const version = e.__micron_wasm_abi_version();
  if (version !== 1) throw new Error(`Micron host ABI version ${version} is unsupported; expected 1`);
  if (!e.__stack_pointer || !e.__micron_wasm_initialize)
    throw new Error("Micron module is missing its startup exports");
  function initialize() {
    const tls = e.__micron_wasm_prepare();
    if (e.__wasm_init_tls) {
      if (!tls) throw new Error("Micron could not reserve initial TLS");
      e.__wasm_init_tls(tls);
    }
    const initialized = e.__micron_wasm_initialize(options.worker ? 1 : 0);
    if (initialized < 0) throw new Error(`Micron initialization failed: ${initialized}`);
    return initialized;
  }
  if (!options.deferInitialize) initialize();
  let stopped = false, shuttingDown = false;
  function requireRunning() {
    if (stopped) throw new Error("Micron runtime has stopped");
    if (shuttingDown) throw new MicronStateError("Micron runtime is shutting down", -16);
  }
  return {
    memory, instance, exports: e, initialize, flushOutput,
    view: (offset, length) => bytes(memory, offset, length),
    call(name, ...args) {
      requireRunning();
      if (typeof e[name] !== "function") throw new MicronStateError(`Micron export is missing: ${name}`, -38);
      try { return e[name](...args); }
      catch (error) { stopped = true; throw error; }
    },
    runMain(argc = 0, argv = 0) {
      requireRunning();
      try { return e.__micron_wasm_run_main(argc, argv); }
      catch (error) { stopped = true; throw error; }
    },
    shutdown() {
      if (stopped) return 0;
      requireRunning();
      shuttingDown = true;
      let result;
      try { result = e.__micron_wasm_shutdown(); }
      catch (error) { stopped = true; throw error; }
      finally { shuttingDown = false; }
      if (result < 0) throw new MicronStateError(`Micron shutdown failed: ${result}`, result);
      stopped = true;
      flushOutput();
      for (const child of children) child.terminate();
      children.clear();
      return result;
    },
    terminate() {
      stopped = true;
      for (const child of children) child.terminate();
      children.clear();
    }
  };
}

export async function createMicron(source, options = {}) {
  const threads = options.threads ?? false;
  if (threads && !node && !globalThis.crossOriginIsolated)
    throw new Error("Micron threads require cross-origin isolation (COOP: same-origin and COEP: require-corp)");
  const module = await compile(source);
  const memory = options.memory ?? new WebAssembly.Memory({
    initial: options.initialPages ?? 256, maximum: options.maximumPages ?? 8192, shared: threads
  });
  if (threads !== (typeof SharedArrayBuffer !== "undefined" && memory.buffer instanceof SharedArrayBuffer))
    throw new Error("Micron shared-memory profile does not match the supplied memory");
  if (!threads) return createLocal(module, memory, options);
  if (!WorkerClass) throw new Error("Micron threads require module workers");
  const poolSize = options.poolSize ?? 4;
  if (!Number.isInteger(poolSize) || poolSize < 1 || poolSize > 1023)
    throw new RangeError("Micron poolSize must be in 1..1023");
  const scheduler = new Int32Array(new SharedArrayBuffer((poolSize + 1) * 4));
  const all = [];
  try { for (let i = 0; i <= poolSize; ++i) all.push(newWorker()); }
  catch (error) { await Promise.allSettled(all.map(w => w.terminate())); throw error; }
  const [worker, ...pool] = all;
  const pending = new Map();
  const retiring = new Set();
  let serial = 0, stopped = false, poisoned = false, shutdownPromise, termination;
  const output = options.onWrite ?? ((text, stream) => {
    if (node) (stream === 2 ? process.stderr : process.stdout).write(text);
    else (stream === 2 ? console.error : console.log)(text);
  });
  const ready = [];
  function rejectPending(error) {
    for (const {reject} of pending.values()) reject(error);
    pending.clear();
  }
  function stopWorkers() {
    if (!termination) termination = Promise.allSettled([...new Set([...all, ...retiring])].map(w => {
      try { return w.terminate(); } catch (error) { return Promise.reject(error); }
    })).then(() => {});
    return termination;
  }
  function poison(error) {
    if (poisoned) return;
    poisoned = true; stopped = true;
    Atomics.store(scheduler, 0, 1);
    for (const entry of ready) entry.reject(error);
    rejectPending(error);
    stopWorkers();
  }
  function bootWorker(w, index) {
    const entry = {};
    const readiness = new Promise((resolve, reject) => { entry.resolve = resolve; entry.reject = reject; });
    // Posting may fail before the caller receives the readiness promise.
    readiness.catch(() => {});
    ready[index] = entry;
    listen(w, message => {
      if (stopped || all[index] !== w || retiring.has(w)) return;
      try {
        if (message.type === "ready") entry.resolve();
        else if (message.type === "write") output(message.text, message.stream);
        else if (message.type === "failed") poison(message.name === "MicronExit" ? new MicronExit(message.code) : new Error(message.error));
        else if (message.type === "complete") {
          Atomics.store(scheduler, message.pool + 1, 0);
          const state = new Int32Array(memory.buffer, message.control >>> 0, 1);
          Atomics.store(state, 0, message.result < 0 ? 4 : 3);
          Atomics.notify(state, 0);
        } else if (message.type === "worker-exit") {
          if (index === 0 || message.pool !== index - 1) throw new Error("Invalid Micron worker exit");
          replaceWorker(w, index, message).catch(poison);
        } else if (message.type === "spawn") {
          if (!Number.isInteger(message.pool) || !pool[message.pool]) throw new Error("Invalid Micron worker dispatch");
          pool[message.pool].postMessage({...message, type: "run"});
        } else if (message.type === "reply") {
          const item = pending.get(message.id);
          if (!item) return;
          pending.delete(message.id);
          if (message.error) item.reject(message.name === "MicronStateError"
            ? new MicronStateError(message.error, message.code) : new Error(message.error));
          else item.resolve(message.result);
        }
      } catch (error) { poison(error); }
    });
    failure(w, error => { if (all[index] === w && !retiring.has(w)) poison(error); });
    if (node) w.on("exit", code => {
      if (!termination && all[index] === w && !retiring.has(w))
        poison(new Error(`Micron worker exited unexpectedly with status ${code}`));
    });
    else w.addEventListener("messageerror", () => {
      if (all[index] === w && !retiring.has(w)) poison(new Error("Micron worker message could not be decoded"));
    });
    w.postMessage({type: "boot", module, memory, primary: index === 0, scheduler, pool: index - 1});
    return readiness;
  }
  async function replaceWorker(previous, index, message) {
    retiring.add(previous);
    try { await previous.terminate(); }
    finally { retiring.delete(previous); }
    if (stopped) return;
    const replacement = newWorker();
    all[index] = replacement;
    pool[index - 1] = replacement;
    await bootWorker(replacement, index);
    if (!stopped) replacement.postMessage({...message, type: "cleanup"});
  }
  function rpc(operation, args = []) {
    if (stopped) return Promise.reject(new Error("Micron runtime has stopped"));
    if (shutdownPromise && operation !== "shutdown")
      return Promise.reject(new MicronStateError("Micron runtime is shutting down", -16));
    const id = ++serial;
    return new Promise((resolve, reject) => {
      pending.set(id, {resolve, reject});
      try { worker.postMessage({type: "call", id, operation, args}); }
      catch (error) { pending.delete(id); reject(error); }
    });
  }
  try { await Promise.all(all.map(bootWorker)); await rpc("initialize"); }
  catch (error) { poison(error); await stopWorkers(); throw error; }
  return {
    memory, view: (offset, length) => bytes(memory, offset, length),
    call: (name, ...args) => rpc("call", [name, ...args]),
    runMain: (argc = 0, argv = 0) => rpc("runMain", [argc, argv]),
    shutdown() {
      if (shutdownPromise) return shutdownPromise;
      if (stopped) return Promise.resolve(0);
      const attempt = (async () => {
        try {
          const result = await rpc("shutdown");
          stopped = true;
          Atomics.store(scheduler, 0, 1);
          rejectPending(new Error("Micron runtime has stopped"));
          await stopWorkers();
          return result;
        } catch (error) {
          if (!(error instanceof MicronStateError)) poison(error);
          throw error;
        } finally { if (shutdownPromise === attempt) shutdownPromise = undefined; }
      })();
      shutdownPromise = attempt;
      return attempt;
    },
    terminate() {
      poison(new Error("Micron runtime terminated"));
      return stopWorkers();
    }
  };
}
