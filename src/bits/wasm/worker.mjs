// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
import {createLocal, MicronStateError} from "./host.mjs";
const node = typeof process !== "undefined" && Boolean(process.versions?.node);
const port = node ? (await import("node:worker_threads")).parentPort : globalThis;
const send = message => port.postMessage(message);
let runtime, scheduler, memory, active, busy = false;
function exitWorker() {
  if (!active || !busy) throw new Error("Invalid Micron worker exit state");
  const park = new Int32Array(new SharedArrayBuffer(4));
  runtime.flushOutput();
  send({...active, type: "worker-exit"});
  for (;;) Atomics.wait(park, 0, 0);
}
async function receive(message) {
  if (message.type === "boot") {
    try {
      scheduler = message.scheduler; memory = message.memory;
      runtime = await createLocal(message.module, memory, {
        worker: true, deferInitialize: true, scheduler, dispatch: send, threadExit: exitWorker,
        onWrite: (text, stream) => send({type: "write", text, stream})
      });
      send({type: "ready"});
    } catch (error) { send({type: "failed", error: String(error?.stack ?? error), name: error?.name, code: error?.code}); }
  } else if (message.type === "run" || message.type === "cleanup") {
    if (busy || Atomics.load(scheduler, 0)) { send({type: "failed", error: "Invalid Micron worker state"}); return; }
    busy = true; active = message;
    try {
      runtime.exports.__stack_pointer.value = message.stack;
      let result;
      if (message.type === "cleanup") {
        runtime.exports.__tls_base.value = message.tls;
        result = runtime.exports.__micron_wasm_worker_cleanup(message.slot);
        if (result !== 0) throw new Error(`Micron worker cleanup failed: ${result}`);
      } else {
        runtime.exports.__wasm_init_tls(message.tls);
        result = runtime.exports.__micron_wasm_worker_entry(message.slot);
      }
      runtime.flushOutput();
      // Publish only after Wasm has returned: its stack and TLS may now be reused.
      busy = false; active = undefined;
      // The broker sees every preceding output message before publishing completion.
      send({type: "complete", pool: message.pool, control: message.control, result});
    } catch (error) {
      Atomics.store(scheduler, 0, 1);
      send({type: "failed", error: String(error?.stack ?? error), name: error?.name, code: error?.code});
    }
  } else if (message.type === "call") {
    try {
      const result = await runtime[message.operation](...message.args);
      send({type: "reply", id: message.id, result});
    } catch (error) {
      send({type: error instanceof MicronStateError ? "reply" : "failed", id: message.id,
        error: String(error?.stack ?? error), name: error?.name, code: error?.code});
    }
  }
}
if (node) port.on("message", receive); else port.addEventListener("message", e => receive(e.data));
