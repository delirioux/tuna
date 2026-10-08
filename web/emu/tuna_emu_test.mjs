// SPDX-License-Identifier: GPL-3.0-only
// TUNA in Node, as the browser's worklet runs it (build/tuna_emu/tuna.wasm, web/emu/tuna_web.c).
//   node web/emu/tuna_emu_test.mjs build/tuna_emu/tuna.wasm
// Checks: it boots stopped and silent on HOME; PLAY starts the composer (sound, ticks, PLAY's LED); PRESETS
// moves the library on at the end of the period; a tapped key sets the root; the RESO and MIDI pages open;
// PLAY again stops to silence; a slot saved while stopped reaches the flash.
import fs from "fs";

const bytes = fs.readFileSync(process.argv[2] || "build/tuna_emu/tuna.wasm");
const stub = new Proxy({}, { get: () => () => 0 });   // wasi imports the reactor never calls
const { instance } = await WebAssembly.instantiate(bytes, { wasi_snapshot_preview1: stub });
const w = instance.exports;
const B = { FX: 0, SCL: 1, ENV: 2, LFO: 3, EDIT: 4, GLO: 5, HOME: 6, SAVE: 7, ARP: 8, SEQ: 9, PLAY: 10, REC: 11, OCTDN: 12, OCTUP: 13 };
const EN = { SELECT: 0, ALGO: 1, PRESETS: 2, K1: 3, K2: 4, K3: 5, K4: 6 };
let fails = 0;
const check = (what, ok) => { console.log(`tuna emu: ${what.padEnd(60)} ${ok ? "ok" : "FAIL"}`); if (!ok) fails++; };
const str = (p) => { const m = new Uint8Array(w.memory.buffer); let s = ""; while (m[p]) s += String.fromCharCode(m[p++]); return s; };

w._initialize();
w.web_nor_erase();
w.web_boot();
let peak = 0;
const run = (sec) => {
  for (let i = 0; i < (sec * 44100) / 512; i++) {
    w.web_render(512);
    for (const v of new Float32Array(w.memory.buffer, w.web_out_l(), 512)) peak = Math.max(peak, Math.abs(v));
  }
};
const press = (b) => { w.web_buttons(1 << b); run(0.05); w.web_buttons(0); run(0.05); };

run(2);
check(`boots on ${str(w.web_tuna_piece())}, HOME, stopped`, str(w.web_tuna_piece()).length > 0 && w.web_tuna_page() === 0 && !w.web_playing());
check("silent while stopped", peak < 1e-3);

press(B.PLAY);
peak = 0;
run(8);
check(`PLAY plays (peak ${peak.toFixed(3)}, ${w.web_tuna_ticks()} ticks)`, w.web_playing() && peak > 0.02 && peak <= 1 && w.web_tuna_ticks() > 0);
check("PLAY's green LED lit", (w.web_lit_buttons() >> 14) & 1);   // bit 14: PLAY's green

const first = str(w.web_tuna_piece());
w.web_enc(EN.PRESETS, 3);
run(30);
check(`PRESETS +3 moves on: ${first} -> ${str(w.web_tuna_piece())}`, str(w.web_tuna_piece()) !== first);

w.web_keys(1 << 2);   // G3 (bit 0 is F3)
run(0.1);
w.web_keys(0);
run(3);
check("a tap on G3 sets the root to G", w.web_tuna_root() === 7);

press(B.LFO);
const reso = w.web_tuna_page();
press(B.ENV);
const midi = w.web_tuna_page();
check(`LFO and ENV open the RESO and MIDI pages (${reso}, ${midi})`, reso !== 0 && midi !== 0 && reso !== midi);
press(B.HOME);
run(0.5);
check("HOME returns", w.web_tuna_page() === 0);

press(B.PLAY);
run(2);
peak = 0;
run(1);
check(`PLAY again stops (tail peak ${peak.toFixed(4)})`, !w.web_playing() && peak < 1e-3);

const writes = w.web_flash_writes_count();
press(B.SAVE);
w.web_keys(1 << 0);
run(0.1);
w.web_keys(0);
press(B.OCTUP);
run(0.5);
check("a slot saved while stopped reaches the flash", w.web_flash_writes_count() > writes);

// switch-on: the page's random seed takes over; the first PLAY keeps a seed that was chosen
w.web_tuna_seed(123456);
check("the page's seed at switch-on (one of 256)", w.web_tuna_seed_now() === 123456 % 256);

console.log(fails ? `tuna emu: ${fails} FAILED` : "tuna emu: all passed");
process.exit(fails ? 1 : 0);
