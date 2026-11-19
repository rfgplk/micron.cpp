#!/usr/bin/env node
// Optional smoke test: Node 22+ and Chromium. The browser is network-blocked.
const { spawn } = require('node:child_process');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { pathToFileURL } = require('node:url');

const site = path.resolve(__dirname, '..');
const profile = fs.mkdtempSync(path.join(os.tmpdir(), 'micron-reference-browser-'));
const browser = spawn(process.env.CHROMIUM || 'chromium-browser', [
  '--headless', '--no-sandbox', '--disable-gpu', '--disable-background-networking',
  '--host-resolver-rules=MAP * ~NOTFOUND', '--remote-debugging-port=0',
  '--user-data-dir=' + profile, 'about:blank',
], { stdio: 'ignore' });
const delay = ms => new Promise(resolve => setTimeout(resolve, ms));
const fileURL = name => pathToFileURL(path.join(site, name)).href;

async function waitFor(fn, message) {
  for (let i = 0; i < 120; ++i) {
    const value = await fn();
    if (value) return value;
    await delay(100);
  }
  throw new Error('Timed out: ' + message);
}

async function main() {
  const active = path.join(profile, 'DevToolsActivePort');
  await waitFor(() => fs.existsSync(active), 'Chromium startup');
  const port = fs.readFileSync(active, 'utf8').split('\n')[0];
  const response = await fetch('http://127.0.0.1:' + port + '/json/new?about:blank', { method: 'PUT' });
  const target = await response.json();
  const ws = new WebSocket(target.webSocketDebuggerUrl);
  await new Promise((resolve, reject) => { ws.onopen = resolve; ws.onerror = reject; });
  let sequence = 0;
  const pending = new Map();
  const requests = [];
  const exceptions = [];
  ws.onmessage = event => {
    const message = JSON.parse(event.data);
    if (message.id && pending.has(message.id)) {
      const [resolve, reject] = pending.get(message.id);
      pending.delete(message.id);
      message.error ? reject(new Error(message.error.message)) : resolve(message.result);
    }
    if (message.method === 'Network.requestWillBeSent') requests.push(message.params.request.url);
    if (message.method === 'Runtime.exceptionThrown') exceptions.push(message.params.exceptionDetails.text);
  };
  function command(method, params = {}) {
    const id = ++sequence;
    ws.send(JSON.stringify({ id, method, params }));
    return new Promise((resolve, reject) => pending.set(id, [resolve, reject]));
  }
  async function evaluate(expression) {
    const result = await command('Runtime.evaluate', { expression, returnByValue: true });
    if (result.exceptionDetails) throw new Error(result.exceptionDetails.text);
    return result.result.value;
  }
  async function navigate(url) {
    await command('Page.navigate', { url });
    await waitFor(async () => {
      try { return await evaluate(`location.href === ${JSON.stringify(url)} && document.readyState === 'complete'`); }
      catch (_) { return false; }
    }, url);
  }
  await command('Page.enable');
  await command('Runtime.enable');
  await command('Network.enable');
  await command('Network.setBlockedURLs', { urls: ['http://*', 'https://*'] });

  await navigate(fileURL('index.html'));
  if (!(await evaluate("document.querySelector('h1').textContent === 'micron reference'"))) throw new Error('Home page missing');
  if (!(await evaluate("document.querySelector('.t-dsc td').getBoundingClientRect().width > 100"))) throw new Error('Table layout is too narrow');
  await evaluate("[...document.querySelectorAll('a')].find(a => a.textContent === 'Vectors').click()");
  await waitFor(async () => {
    try { return await evaluate("location.pathname.endsWith('/modules/vectors.html') && document.readyState === 'complete'"); }
    catch (_) { return false; }
  }, 'vector navigation');
  if (!(await evaluate("document.body.textContent.includes('null pointer when no element matches')"))) throw new Error('Vector contract missing');

  await navigate(fileURL('headers/vector/vector.hpp.html'));
  if (!(await evaluate("document.querySelectorAll('.function-entry').length > 100"))) throw new Error('Per-overload documentation missing');
  const overflowing = await evaluate("[...document.querySelectorAll('.function-entry .t-dcl-begin')].filter(e => e.scrollWidth > e.clientWidth + 1).slice(0, 3).map(e => ({name: e.closest('.function-entry').querySelector('h3').textContent, width: e.clientWidth, scroll: e.scrollWidth, pre: e.querySelector('pre').getBoundingClientRect().width}))");
  if (overflowing.length) throw new Error('Declaration table overflows its container: ' + JSON.stringify(overflowing));
  if (!(await evaluate("[...document.querySelectorAll('.function-entry')].some(e => e.querySelector('h3').textContent.startsWith('micron::vector::find(') && e.textContent.includes('Equivalent definition') && e.textContent.includes('Return value'))"))) throw new Error('Function effects or return contract missing');
  await evaluate("const entry = [...document.querySelectorAll('.function-entry')].find(e => e.querySelector('h3').textContent.startsWith('micron::vector::find(')); entry.querySelector('details').open = true; entry.scrollIntoView(); true");
  if (process.env.REFERENCE_SCREENSHOT) {
    const screenshot = await command('Page.captureScreenshot', { format: 'png' });
    fs.writeFileSync(process.env.REFERENCE_SCREENSHOT, Buffer.from(screenshot.data, 'base64'));
  }
  await evaluate("[...document.querySelectorAll('.function-entry')].find(e => e.querySelector('h3').textContent.startsWith('micron::vector::find(')).querySelector('p a').click(); true");
  await waitFor(async () => {
    try { return await evaluate("location.pathname.endsWith('/source/vector/vector.hpp.html') && document.readyState === 'complete' && Boolean(document.querySelector(':target.source-line'))"); }
    catch (_) { return false; }
  }, 'source evidence navigation');

  const search = fileURL('search.html') + '?q=micron%3A%3Avector';
  await navigate(search);
  await waitFor(() => evaluate("document.querySelector('#search-status').textContent.includes('result')"), 'local search');
  if (!(await evaluate("[...document.querySelectorAll('#search-results a')].some(a => a.textContent === 'micron::vector')"))) throw new Error('Vector search missing');
  await evaluate("document.querySelector('#query').value = 'robin_map::find'; document.querySelector('#search-form').requestSubmit(); true");
  if (!(await evaluate("[...document.querySelectorAll('#search-results a')].some(a => a.textContent === 'micron::robin_map::find')"))) throw new Error('Search submission failed');

  await command('Emulation.setScriptExecutionDisabled', { value: true });
  await navigate(fileURL('symbols/index.html'));
  if (!(await evaluate("[...document.querySelectorAll('a')].some(a => a.textContent === 'V')"))) throw new Error('Static symbol navigation missing');
  const remote = requests.filter(url => /^https?:/i.test(url));
  if (remote.length) throw new Error('Unexpected remote requests: ' + remote.join(', '));
  if (exceptions.length) throw new Error('Browser exceptions: ' + exceptions.join(', '));
  ws.close();
  console.log('Offline browser navigation, function effects, source evidence, layout, search, and script-free index passed');
}
main().catch(error => { console.error(error); process.exitCode = 1; }).finally(async () => {
  browser.kill();
  await delay(300);
  fs.rmSync(profile, { recursive: true, force: true });
});
