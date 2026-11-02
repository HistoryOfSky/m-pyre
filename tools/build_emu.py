#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 History Of Sky (M-PYRE)
"""M-PYRE browser emulator: firmware/host/emu.c -> build/emu/mpyre-emu.wasm (clang --target=wasm32 + wasm-ld,
tools/wasm_shim), then web/emu.html with the wasm and the sound pack inlined -> build/emu/m-pyre-emulator.html
(one file: open it, or publish it).

python3 tools/build_emu.py [SOUNDPACK]
"""
from pathlib import Path
import base64
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
GEN = ROOT / "build/gen"
OUT = ROOT / "build/emu"
EXPORTS = ["init", "buffer", "screen", "block_frames", "rate", "slot", "reset_tracks", "volume", "keys", "button",
           "knob", "led_keys", "led_buttons", "ui", "render", "proj_get", "proj_put", "proj_changed"]


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    shim = ROOT / "tools/wasm_shim"
    mem = OUT / "mem.o"
    subprocess.run(["clang", "--target=wasm32", "-O2", "-fno-builtin", "-ffreestanding", "-c", str(shim / "mem.c"), "-o", str(mem)],
                   check=True)
    wasm = OUT / "mpyre-emu.wasm"
    subprocess.run(["clang", "--target=wasm32", "-O2", "-fwrapv", "-fno-strict-aliasing", "-ffreestanding", "-nostdlib", "-w",
                    "-I" + str(shim), "-I" + str(GEN), "-I" + str(ROOT / "firmware/src"), "-I" + str(ROOT / "firmware/hal"),
                    str(ROOT / "firmware/host/emu.c"), str(mem), "-Wl,--no-entry", "-Wl,--export-memory",
                    "-Wl,--initial-memory=33554432", "-Wl,-z,stack-size=262144"]
                   + ["-Wl,--export=emu_" + n for n in EXPORTS] + ["-o", str(wasm)], check=True)
    mem.unlink()
    pack = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "build/mpyre/soundpack.bin"
    html = (ROOT / "web/emu.html").read_text()
    html = html.replace("/*WASM_B64*/\"\"", '"' + base64.b64encode(wasm.read_bytes()).decode() + '"')
    html = html.replace("/*PACK_B64*/\"\"", '"' + (base64.b64encode(pack.read_bytes()).decode() if pack.exists() else "") + '"')
    out = OUT / "m-pyre-emulator.html"
    out.write_text(html)
    print(f"emu: {wasm} {wasm.stat().st_size} B; {out} {out.stat().st_size} B (sound pack: {'yes' if pack.exists() else 'no'})")


if __name__ == "__main__":
    main()
