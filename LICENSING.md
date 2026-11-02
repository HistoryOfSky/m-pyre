# M-PYRE licensing

M-PYRE is free software. It is based on [SLOOP](https://github.com/isod89/sloop-fm1) by isod89, which is based on
[Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita / Hügelton Instruments. Its **code** is licensed under
the GNU General Public License, version 3 only (`GPL-3.0-only`, full text in `LICENSE`), like theirs.

- Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments (Felucca)
- Copyright (C) 2026 isod89 and the SLOOP contributors (SLOOP)
- Copyright (C) 2026 3dSam (Clipper: the HYBRID voice, `firmware/src/eng_hybrid.c`, `firmware/src/hybrid_patches.h`)
- Copyright (C) 2026 History Of Sky (M-PYRE)

Files changed for M-PYRE say so in their header ("Modified for M-PYRE by History Of Sky, 2026"); files written for
it carry History Of Sky's copyright. [CHANGELOG.md](CHANGELOG.md) lists what changed from SLOOP 2.8.1.

## What is code (GPL-3.0-only)

Every file in this tree that carries an `SPDX-License-Identifier: GPL-3.0-only` header (one source file carries
another: `firmware/src/fm6_core.c`, msfa, Apache-2.0, below; the firmware built with it is GPL-3.0-only as a whole):

- the firmware: `firmware/` (app, HAL, update loader, the browser emulator's host code)
- the build script and tools: `build.sh`, `tools/`
- the web pages (installer, editor, emulator) and their tests: `web/` (not the Fukiai font, below)
- the host tests: `tests/`

You may use, study, change and share it under the GPL. If you distribute M-PYRE, or firmware derived from it, you
must also give your recipients its complete corresponding source under the same licence.

## History Of Sky's assets

These are not code. They are made by History Of Sky:

| What | Where |
| --- | --- |
| The guitar, palm mute and bass recordings (the sound pack) | `assets/mpyre-src/` (sources), `soundpack.bin` (release) |
| The HEAVY KIT drum recordings (built into the firmware) | `assets/mpyre-src/drums/` |
| The cabinet impulse responses (built into the firmware) | `assets/mpyre-src/ir/` |
| The M-PYRE boot picture and the History Of Sky logo | `assets/mpyre/splash.png` |

Copyright (C) 2026 History Of Sky. **Licence: to be decided before the first public release** (see below).

<!-- History Of Sky chooses one of these before release; the text below is replaced by the choice:
     CC BY 4.0 (anyone may use and share them, also outside M-PYRE, naming History Of Sky; the FSF considers CC BY 4.0
     compatible with GPLv3, which keeps the firmware that contains them simple to share), CC0, or all rights reserved
     with a permission to share them unchanged with M-PYRE and to use them freely in your own music. -->

Whatever the licence, music you make with M-PYRE is yours: the sounds may be used in your recordings and
performances without asking and without credit.

## The Felucca assets (additional permission, GPL-3.0 section 7)

Felucca's icon atlas (`assets/icons.png`), panel image (`docs/panel.jpg`) and the Hügelton Sample Pack
(`tools/gen_waves.py`) are Copyright (C) 2026 Hügelton Instruments, all rights reserved. Felucca grants this permission,
which carries over to M-PYRE:

> As an additional permission under GPL-3.0 section 7, you may combine Felucca, or a work based on it, with the Felucca
> Assets, and convey the combination. This is allowed even though the Felucca Assets are not licensed under the GPL,
> provided that you follow the GPL for every part that is not a Felucca Asset, and you follow the terms published for
> the assets.

M-PYRE's firmware does not contain the Hügelton Sample Pack.

## Third-party material

| What | Licence | Where |
| --- | --- | --- |
| Piano, vibes and strings samples (Versilian Studios VSCO-2 CE, VCSL) | CC0 1.0 | `assets/samples-cc0/`, provenance in `ATTRIBUTION.txt` there |
| Barlow Condensed (Jeremy Tribby): the FM-1's screen font and the installer's | SIL OFL 1.1 | `assets/fonts/BarlowCondensed-*.ttf`, `assets/fonts/BarlowCondensed-OFL.txt` |
| Black Ops One, Damion, Pacifico, Racing Sans One, Russo One: the amp names (drawn into the firmware as pictures) | SIL OFL 1.1 | `assets/mpyre/fonts/` (each with its `OFL-*.txt`) |
| Terminus font 8x16: the web editor's font (`tools/gen_webfont.py` inlines it in `web/editor.html`) | SIL OFL 1.1 | `assets/fonts/ter-u16b.bdf`, `assets/fonts/ter-u16n.bdf`, `assets/fonts/Terminus-LICENSE.txt` |
| Fukiai icon font (Hügelton Instruments), web editor only | MIT | `web/fukiai.ttf`, `web/FUKIAI-LICENSE.txt` |
| CrispyZebra by Leo Kuroshita (<https://github.com/hugelton/CrispyZebra>): the PHASE engine's waveforms | GPL-3.0 | `firmware/src/eng_phase.c` |
| Clipper by 3dSam: the HYBRID voice and its patches | GPL-3.0 | `firmware/src/eng_hybrid.c`, `firmware/src/hybrid_patches.h` |
| msfa by Google Inc. and Pascal Gauthier, from Dexed (<https://github.com/asb2m10/dexed>): the FM6 engine's synthesis | Apache-2.0 | `firmware/src/fm6_core.c`, `LICENSES/Apache-2.0-msfa.txt` |
| DaisySP (Electrosmith, Emilie Gillet) and Rings' sympathetic strings (Emilie Gillet): SLOOP's PHYS engine (in the tree, not built into M-PYRE) | MIT | `firmware/src/phys_dsp.c`, `firmware/src/phys_symp.c` |
| klattsch by Tony Gies: design reference for SLOOP's VOICE engine (not built into M-PYRE); no code copied | MIT (klattsch) | credit only |
| JieLi AC79 SDK: `uboot.boot`, `cfg_tool.bin`, `eq_cfg_hw.bin` are read from your SDK checkout at build time and placed in the package; no SDK files are in this tree | Apache-2.0 | <https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK> |

## Contributions

Contributions are welcome under GPL-3.0-only. By submitting one, you agree that it may be combined with the Felucca
Assets under the section 7 permission above.

## Trademarks

"Felucca" and "Hügelton Instruments" are names of Hügelton Instruments. "History Of Sky" is the name of the band.

"M-VAVE" and "FM-1" are trademarks of their respective owners. M-PYRE is independent firmware that runs on FM-1
hardware. It is not affiliated with, endorsed by or supported by those owners.

## Radio

M-PYRE never enables the Bluetooth / Wi-Fi radio of the hardware.
