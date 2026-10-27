// SPDX-License-Identifier: GPL-3.0-only
// HYP-1's SMK-37 build in an AudioWorklet (web/emu/smk37/): amb.wasm is the whole app, built with BOARD_SMK37
// (the FM voice too). Each render quantum asks it for 128 frames, which runs the device's clock forward;
// between quanta the page's input goes in and, about 30 times a second, the screen, the lights, the pads'
// colours and any saved objects go out. (From web/emu/worklet.js, with the SMK-37's controls.)

const clock = globalThis.performance ? () => globalThis.performance.now() : () => Date.now();

class Hyp1Smk extends AudioWorkletProcessor {
  constructor() {
    super();
    this.ex = null;
    this.sentBlits = -1;
    this.sentWrites = -1;
    this.lastFrame = 0;
    this.busy = 0;
    this.frames = 0;
    this.port.onmessage = (e) => this.onMessage(e.data);
  }

  async onMessage(m) {
    if (m.type === "load") {
      const now = () => BigInt(Math.round(currentTime * 1e9));
      const wasi = {
        clock_time_get: (id, prec, out) => { new DataView(this.mem.buffer).setBigUint64(out, now(), true); return 0; },
        fd_write: (fd, iov, n, out) => { new DataView(this.mem.buffer).setUint32(out, 0, true); return 0; },
        proc_exit: () => {},
      };
      const stub = new Proxy(wasi, { get: (t, k) => (k in t ? t[k] : () => 0) });
      const env = new Proxy({}, { get: () => () => 0 });
      const { instance } = await WebAssembly.instantiate(m.wasm, { wasi_snapshot_preview1: stub, env });
      this.ex = instance.exports;
      this.mem = this.ex.memory;
      if (this.ex._initialize) this.ex._initialize();
      const max = this.ex.web_store_max();
      for (const [obj, bytes] of Object.entries(m.store || {})) {
        const b = new Uint8Array(bytes).subarray(0, max);
        new Uint8Array(this.mem.buffer, this.ex.web_store(+obj), b.length).set(b);
        this.ex.web_store_set_len(+obj, b.length);
      }
      this.sentWrites = this.ex.web_store_writes();
      this.ex.web_master(m.master ?? 2047);
      this.ex.web_seed(m.seed >>> 0);
      this.ex.web_boot();
      this.port.postMessage({ type: "ready" });
      return;
    }
    const ex = this.ex;
    if (!ex) return;
    switch (m.type) {
      case "buttons": ex.web_buttons(m.mask >>> 0); break;
      case "smkbtn": ex.web_smk_buttons(m.mask >>> 0); break;
      case "keybed": ex.web_key_vel(m.vel | 0); ex.web_keybed(m.lo >>> 0, m.hi >>> 0); break;
      case "pad": ex.web_pad(m.pad | 0, m.vel | 0); break;
      case "fader": ex.web_fader(m.i | 0, m.v | 0); break;
      case "wheel": ex.web_wheel(m.pitch | 0, m.mod | 0); break;
      case "enc": ex.web_enc(m.role, m.n | 0); break;
      case "master": ex.web_master(m.value | 0); break;
      case "sysex": {
        const b = new Uint8Array(m.bytes).subarray(0, 8192);
        new Uint8Array(this.mem.buffer, ex.web_sysex_buf(), b.length).set(b);
        ex.web_sysex(b.length);
        break;
      }
    }
  }

  publish() {
    const ex = this.ex;
    const blits = ex.web_blits();
    const msg = {
      type: "frame",
      buttons: ex.web_lit_buttons() >>> 0,
      smklit: ex.web_smk_lit() >>> 0,
      pads: new Uint8Array(this.mem.buffer, ex.web_pad_rgb(), 48).slice(),
    };
    if (this.frames >= 44100) {                       // the share of real time spent running the device
      msg.load = Math.round(100 * this.busy / (this.frames / 44.1));
      this.busy = this.frames = 0;
    }
    const transfer = [msg.pads.buffer];
    if (blits !== this.sentBlits) {
      this.sentBlits = blits;
      msg.fb = new Uint16Array(this.mem.buffer, ex.web_fb(), 240 * 240).slice();
      transfer.push(msg.fb.buffer);
    }
    const writes = ex.web_store_writes();
    if (writes !== this.sentWrites) {
      this.sentWrites = writes;
      msg.store = {};
      for (let o = 0; o < ex.web_nobj(); o++) {
        const n = ex.web_store_len(o);
        if (n >= 0) msg.store[o] = new Uint8Array(this.mem.buffer, ex.web_store(o), n).slice();
      }
    }
    this.port.postMessage(msg, transfer);
  }

  process(inputs, outputs) {
    if (!this.ex) return true;
    const out = outputs[0];
    const n = out[0].length;
    const t0 = clock();
    this.ex.web_render(n);
    this.busy += clock() - t0;
    this.frames += n;
    out[0].set(new Float32Array(this.mem.buffer, this.ex.web_out_l(), n));
    if (out[1]) out[1].set(new Float32Array(this.mem.buffer, this.ex.web_out_r(), n));
    if (currentTime - this.lastFrame >= 1 / 30) {
      this.lastFrame = currentTime;
      this.publish();
    }
    return true;
  }
}

registerProcessor("hyp1smk", Hyp1Smk);
