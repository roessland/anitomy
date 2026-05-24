// Microbenchmark: parse every entry in test/data.json N times,
// report per-parse time. Calls into the wasm directly.

import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, resolve } from 'node:path';

const here = dirname(fileURLToPath(import.meta.url));
const root = resolve(here, '..');

const wasmBytes = readFileSync(resolve(root, 'bin', 'anitomy.wasm'));
const data = JSON.parse(readFileSync(resolve(root, 'test', 'data.json'), 'utf8'));
const inputs = data.map(d => d.input);

const imports = {
  env: { emscripten_notify_memory_growth: () => {} },
  wasi_snapshot_preview1: {
    environ_sizes_get: () => 0,
    environ_get: () => 0,
    fd_close: () => 0,
  },
};
const { instance } = await WebAssembly.instantiate(wasmBytes, imports);
const { memory, malloc, anitomy_free, anitomy_parse, _initialize } = instance.exports;
_initialize();

const enc = new TextEncoder();
const dec = new TextDecoder();

function readCString(ptr) {
  const view = new Uint8Array(memory.buffer, ptr);
  let end = 0;
  while (view[end] !== 0) end++;
  return dec.decode(new Uint8Array(memory.buffer, ptr, end));
}

// Pre-allocate input buffers once
const inputPtrs = inputs.map(s => {
  const utf8 = enc.encode(s + '\0');
  const p = malloc(utf8.length);
  new Uint8Array(memory.buffer, p, utf8.length).set(utf8);
  return p;
});

function pass() {
  for (const p of inputPtrs) {
    const out = anitomy_parse(p, 0);
    readCString(out);  // force the work; discard
    anitomy_free(out);
  }
}

// Warm-up
for (let i = 0; i < 5; i++) pass();

const N = 100;
const samples = [];
for (let i = 0; i < N; i++) {
  const t0 = performance.now();
  pass();
  samples.push(performance.now() - t0);
}
samples.sort((a, b) => a - b);

const sum = samples.reduce((a, b) => a + b, 0);
const median = samples[Math.floor(samples.length / 2)];
const p95 = samples[Math.floor(samples.length * 0.95)];

console.log(`entries     : ${inputs.length}`);
console.log(`runs        : ${N}`);
console.log(`per pass    : min=${samples[0].toFixed(2)}ms  median=${median.toFixed(2)}ms  p95=${p95.toFixed(2)}ms  max=${samples[N-1].toFixed(2)}ms`);
console.log(`per filename: median ${(median / inputs.length * 1000).toFixed(1)}µs  (mean over all runs: ${(sum / N / inputs.length * 1000).toFixed(1)}µs)`);
console.log(`throughput  : ~${Math.round(inputs.length / median * 1000)} filenames/sec (median)`);
