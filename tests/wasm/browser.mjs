// Copyright (c) 2026 micron contributors.
// Distributed under the Boost Software License, Version 1.0.
import {createServer} from 'node:http';
import {readFile, stat, writeFile} from 'node:fs/promises';
import {resolve, extname, sep} from 'node:path';
import {pathToFileURL} from 'node:url';

const root = resolve(new URL('../..', import.meta.url).pathname);
const argument = (name, fallback) => process.argv.find(x => x.startsWith(`--${name}=`))?.slice(name.length + 3) ?? fallback;
const manifest = JSON.parse(await readFile(argument('manifest', `${root}/bin/wasm-tests/modules.json`), 'utf8'));
if (!Array.isArray(manifest.modules) || !manifest.modules.length) throw new Error('browser manifest has no modules');
const playwright = await import(pathToFileURL(resolve(process.env.MICRON_PLAYWRIGHT_MODULE ?? `${root}/bin/wasm-tools/node_modules/playwright/index.mjs`)));
const engines = argument('engines', 'chromium,firefox,webkit').split(',');
const allowedImports = new Set(['env.memory', 'micron.write', 'micron.clock', 'micron.panic', 'micron.yield', 'micron.worker_spawn', 'micron.thread_exit', 'micron.random']);
const results = [];
let failed = 0;

const server = createServer(async (request, response) => {
  try {
    const url = new URL(request.url, 'http://localhost');
    if (url.pathname !== '/unisolated') {
      response.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
      response.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');
    }
    response.setHeader('Cache-Control', 'no-store');
    if (url.pathname === '/' || url.pathname === '/unisolated') {
      response.setHeader('Content-Type', 'text/html');
      response.end('<!doctype html><meta charset="utf-8"><title>Micron WASM tests</title>'); return;
    }
    const file = resolve(root, '.' + decodeURIComponent(url.pathname));
    if (!file.startsWith(root + sep) || !(await stat(file)).isFile()) { response.writeHead(404); response.end(); return; }
    response.setHeader('Content-Type', {'.wasm': 'application/wasm', '.mjs': 'text/javascript', '.js': 'text/javascript', '.json': 'application/json'}[extname(file)] ?? 'application/octet-stream');
    response.end(await readFile(file));
  } catch { response.writeHead(404); response.end(); }
});
await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
const origin = `http://127.0.0.1:${server.address().port}`;

function deadline(promise, seconds) {
  let timer;
  return Promise.race([promise, new Promise((_, reject) => { timer = setTimeout(() => reject(new Error('browser test timed out')), seconds * 1000); })]).finally(() => clearTimeout(timer));
}
try {
  for (const engine of engines) {
    let browser;
    try {
      if (!playwright[engine]) throw new Error(`unknown browser engine ${engine}`);
      browser = await playwright[engine].launch({headless: true});
      for (const entry of manifest.modules) {
        const page = await browser.newPage();
        try {
          await page.goto(origin);
          const record = await deadline(page.evaluate(async ({entry, imports}) => {
            const {createMicron, createLocal} = await import('/src/bits/wasm/host.mjs');
            const binary = await (await fetch('/' + entry.file)).arrayBuffer();
            const module = await WebAssembly.compile(binary);
            for (const item of WebAssembly.Module.imports(module))
              if (!imports.includes(`${item.module}.${item.name}`)) throw new Error(`unexpected import ${item.module}.${item.name}`);
            const output = [];
            const runtime = await createMicron(module, {threads: entry.threads, initialPages: 256, maximumPages: entry.maximumPages ?? 8192, onWrite: (text, stream) => output.push({text, stream})});
            let code, cases = 0, hostFuzz, numericOracle, mathOracles;
            try {
              code = await runtime.runMain();
              if (code !== (entry.exit ?? 1)) throw new Error(`exit ${code}; expected ${entry.exit ?? 1}`);
              if (entry.minCases) {
                const count = async name => {
                  const value = await runtime.call(name);
                  if (!Number.isInteger(value) || value < -0x80000000 || value > 0xffffffff)
                    throw new Error(`invalid unsigned test coverage export ${name}: ${String(value)}`);
                  return value >>> 0;
                };
                cases = await count('__micron_wasm_test_cases');
                const skips = await count('__micron_wasm_test_skips');
                if (cases < entry.minCases || skips !== 0) throw new Error(`cases=${cases}, skips=${skips}, expected >=${entry.minCases} and no skips`);
              }
              if (entry.hostProbe) {
                const old = runtime.view();
                const size = old.byteLength + 65536;
                const pointer = await runtime.call('test_reserve', size);
                if (!pointer || runtime.view(pointer, 1)[0] !== 0x35 || runtime.view(pointer + size - 1, 1)[0] !== 0xa7)
                  throw new Error('growth did not preserve fresh views and stored bytes');
                if (!entry.threads && old.byteLength !== 0) throw new Error('ordinary old memory view was not detached');
                if (entry.threads && old.byteLength >= runtime.view().byteLength) throw new Error('shared old memory view unexpectedly grew');
                if (await runtime.call('test_write', 0xfffffff0, 64) !== -14) throw new Error('out-of-bounds host write accepted');
                let bounds = false;
                try { runtime.view(0xfffffff0, 64); } catch (e) { bounds = e instanceof RangeError; }
                if (!bounds) throw new Error('out-of-bounds host view accepted');
                const {checkUtf8} = await import('/tests/wasm/runtime_utf8.mjs');
                await checkUtf8(runtime, output, entry.threads);
              }
              if (entry.lifecycle) {
                const {checkLifecycle} = await import('/tests/wasm/runtime_lifecycle.mjs');
                await checkLifecycle(runtime, entry.threads);
              }
            } finally { await runtime.shutdown(); }
            if (entry.lifecycle) {
              const {checkHostLifecycle} = await import('/tests/wasm/runtime_host_lifecycle.mjs');
              hostFuzz = await checkHostLifecycle(module, {threads: entry.threads});
            }
            if (entry.numericOracle || entry.mathOracle) {
              // Arithmetic probes execute synchronously; their separate memory has no workers.
              const memory = new WebAssembly.Memory({initial: 256, maximum: entry.maximumPages ?? 8192, shared: entry.threads});
              const local = await createLocal(module, memory, {worker: entry.threads});
              try {
                if (entry.numericOracle) {
                  const {checkNumeric} = await import('/tests/wasm/numeric_oracle.mjs');
                  numericOracle = checkNumeric(local.exports, memory, {iterations: entry.oracleIterations ?? 6000,
                    seed: entry.oracleSeed ?? '0x9e3779b97f4a7c15'});
                }
                if (entry.mathOracle) {
                  const {checkMath} = await import('/tests/wasm/math_oracle.mjs');
                  mathOracles = [];
                  for (const corpus of ['math_reference.json', 'fuzz_math_reference.json']) {
                    const reference = await (await fetch('/tests/wasm/' + corpus)).json();
                    const checked = checkMath(local.exports, memory, reference);
                    if (checked.failed) throw new Error(`${corpus}: ${JSON.stringify(checked.failures.slice(0, 3))}`);
                    mathOracles.push(checked);
                  }
                }
              } finally { await local.shutdown(); }
            }
            let stopped = false;
            try { await runtime.runMain(); } catch { stopped = true; }
            if (!stopped) throw new Error('runtime accepted a call after shutdown');
            await runtime.shutdown();
            return {code, cases, hostFuzz, numericOracle, mathOracles,
              outputBytes: output.reduce((n, x) => n + x.text.length, 0), isolated: crossOriginIsolated};
          }, {entry, imports: [...allowedImports]}), entry.numericOracle || entry.mathOracle ? 180 : entry.timeout ?? 30);
          results.push({engine, version: browser.version(), name: entry.name, status: 'PASS', ...record});
          console.log(`PASS ${engine} ${entry.name}`);
        } catch (error) {
          failed++; results.push({engine, name: entry.name, status: 'FAIL', error: String(error)});
          console.error(`FAIL ${engine} ${entry.name}: ${error.message}`);
        } finally { await page.close(); }
      }
      const shared = manifest.modules.find(x => x.threads);
      if (shared) {
        const page = await browser.newPage();
        await page.goto(origin + '/unisolated');
        const rejected = await page.evaluate(async file => {
          const {createMicron} = await import('/src/bits/wasm/host.mjs');
          try { await createMicron('/' + file, {threads: true}); return false; }
          catch (e) { return !crossOriginIsolated && /isolation/.test(e.message); }
        }, shared.file);
        await page.close();
        if (!rejected) throw new Error('shared profile accepted a page without cross-origin isolation');
        results.push({engine, name: 'isolation-negative-control', status: 'PASS'});
      }
    } catch (error) {
      failed++; results.push({engine, status: 'FAIL', error: String(error)});
      console.error(`FAIL ${engine}: ${error.message}`);
    } finally { await browser?.close(); }
  }
} finally { await new Promise(resolve => server.close(resolve)); }
await writeFile(argument('output', `${root}/bin/wasm-tests/browser-results.json`), JSON.stringify({failed, results}, null, 2) + '\n');
process.exitCode = failed ? 1 : 0;
