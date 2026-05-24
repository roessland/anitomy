// Minimal smoke test: instantiate anitomy.wasm with stub imports, parse a
// few filenames, print the JSON. No Emscripten JS glue involved.

import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, resolve } from 'node:path';

const here = dirname(fileURLToPath(import.meta.url));
const wasmPath = resolve(here, '..', 'bin', 'anitomy.wasm');

const bytes = readFileSync(wasmPath);

const imports = {
  env: {
    emscripten_notify_memory_growth: () => {},
  },
  wasi_snapshot_preview1: {
    environ_sizes_get: () => 0,
    environ_get: () => 0,
    fd_close: () => 0,
  },
};

const { instance } = await WebAssembly.instantiate(bytes, imports);
const { memory, malloc, anitomy_free, anitomy_parse, anitomy_version, _initialize } = instance.exports;

_initialize();

const encoder = new TextEncoder();
const decoder = new TextDecoder();

function readCString(ptr) {
  const view = new Uint8Array(memory.buffer, ptr);
  let end = 0;
  while (view[end] !== 0) end++;
  return decoder.decode(new Uint8Array(memory.buffer, ptr, end));
}

function parse(input, pretty = false) {
  const utf8 = encoder.encode(input + '\0');
  const inPtr = malloc(utf8.length);
  new Uint8Array(memory.buffer, inPtr, utf8.length).set(utf8);
  const outPtr = anitomy_parse(inPtr, pretty ? 1 : 0);
  const json = readCString(outPtr);
  anitomy_free(outPtr);
  anitomy_free(inPtr);
  return JSON.parse(json);
}

console.log('version:', readCString(anitomy_version()));

const cases = [
  '[TaigaSubs]_Toradora!_(2008)_-_01v2_-_Tiger_and_Dragon_[1080p_H.264_FLAC][BAD7A16A].mkv',
  '[SubsPlease] Sousou no Frieren - 28 (1080p) [B5E3F7A2].mkv',
  '[Erai-raws] Sousou no Frieren - 01 ~ 28 [1080p][Multiple Subtitle].mkv',
  '[HorribleSubs] Boku no Hero Academia S04 - 12 [720p].mkv',
];

for (const c of cases) {
  console.log('\n>', c);
  console.log(parse(c, true));
}
