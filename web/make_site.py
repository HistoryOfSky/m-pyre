#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
# Copyright (C) 2026 History Of Sky (M-PYRE)
"""Make the site (GitHub Pages):

  index.html                  redirect to the installer
  firmware/m-pyre-VER.fwsc    the package
  webapp/installer/index.html index_pkg.html with fm1pkg.js, fm1ota.js and the metadata inlined
  webapp/editor/index.html    editor.html (+ fukiai.ttf, FUKIAI-LICENSE.txt)
  webapp/editor/m-pyre-editor.html  the editor in one file, for offline use (from SLOOP 2.6): its icon font inlined,
                              its links to the installer and the font licence online; opened from the disk in
                              Chrome / Edge (file:// is a secure context: Web MIDI asks for permission)
  webapp/sw.js, webapp/m-pyre.webmanifest, webapp/icon-*.png  the site without internet (from SLOOP 2.6): an app to
                              install; opened once online, the installer, the editor and the firmware are kept
                              by the browser and work offline, still on https (on Android a downloaded file opens
                              as content://, where Chrome refuses Web MIDI)
  src/                        not touched

  web/make_site.py build/felucca-X.Y.fwsc X.Y OUT_DIR [--beta]
  (--beta: web/beta_banner.html at the top of the installer: the beta channel, docs/beta)

The package identity (M-PYRE: FM-1_123) is read from the package; the device reports it
after the install.
"""
import base64
import hashlib
import json
import os
import re
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
# M-PYRE: where the site and the source live (GitHub Pages of the repository); MPYRE_USER / MPYRE_REPO override them
USER = os.environ.get("MPYRE_USER", "historyofsky")
REPO = os.environ.get("MPYRE_REPO", "m-pyre")
ONLINE = f"https://{USER}.github.io/{REPO}/"                     # (the offline editor's links)
SOURCE = f"https://github.com/{USER}/{REPO}"


def offline_editor(html):
    """editor.html as one file for the disk: the icon font inlined, the download link out, the relative links online"""
    font = base64.b64encode((HERE / "fukiai.ttf").read_bytes()).decode()
    for a, b in (('url("fukiai.ttf")', 'url(data:font/ttf;base64,' + font + ')'),
                 ('href="../installer/"', 'href="' + ONLINE + 'webapp/installer/" target="_blank" rel="noopener"'),
                 ('href="FUKIAI-LICENSE.txt"', 'href="' + ONLINE + 'webapp/editor/FUKIAI-LICENSE.txt" target="_blank" rel="noopener"')):
        if html.count(a) != 1:
            raise SystemExit(f"editor.html must contain {a} once; update make_site.py")
        html = html.replace(a, b)
    for pat, what in ((r"<!--DL-->.*?<!--/DL-->", "<!--DL-->...<!--/DL-->"),       # (the file has no site
                      (r"<!--PWA-->.*?<!--/PWA-->", "<!--PWA-->...<!--/PWA-->"),    # around it: no app to
                      (r"/\*PWA\*/.*?/\*/PWA\*/", "/*PWA*/.../*/PWA*/"),           # install, no copy kept)
                      (r'<button id="install"[^>]*></button>\s*', '<button id="install">')):
        html, n = re.subn(pat, "", html, flags=re.S)
        if n != 1:
            raise SystemExit(f"editor.html must contain {what} once; update make_site.py")
    return html


ICONS = ("icon-192.png", "icon-512.png", "icon-maskable-512.png")


def app_files(out, version, fw_name):
    """webapp/: the manifest, the icons and sw.js with the list of what it keeps (relative to webapp/) and a
    stamp of their contents, so any change makes a new sw.js and the browser takes the new site"""
    app = out / "webapp"
    for f in ICONS:
        shutil.copy(HERE / f, app / f)
    manifest = {
        "name": "M-PYRE", "short_name": "M-PYRE", "id": "editor/", "start_url": "editor/", "scope": "./",
        "display": "standalone", "background_color": "#080000", "theme_color": "#080000",
        "description": "M-PYRE, the riffbox firmware for the M-VAVE FM-1: the editor and the installer, also without internet",
        "icons": [{"src": "icon-192.png", "sizes": "192x192", "type": "image/png", "purpose": "any"},
                  {"src": "icon-512.png", "sizes": "512x512", "type": "image/png", "purpose": "any"},
                  {"src": "icon-maskable-512.png", "sizes": "512x512", "type": "image/png", "purpose": "maskable"}],
    }
    (app / "m-pyre.webmanifest").write_text(json.dumps(manifest, indent=1) + "\n", encoding="utf-8")
    keep = {"installer/": app / "installer" / "index.html", "editor/": app / "editor" / "index.html",
            "../firmware/" + fw_name: out / "firmware" / fw_name, "m-pyre.webmanifest": app / "m-pyre.webmanifest"}
    if (out / "firmware" / "soundpack.bin").exists():
        keep["../firmware/soundpack.bin"] = out / "firmware" / "soundpack.bin"   # (M-PYRE: offline too)
    for f in ("fukiai.ttf", "FUKIAI-LICENSE.txt"):
        if (app / "editor" / f).exists():
            keep["editor/" + f] = app / "editor" / f
    for f in ("preview.js", "preview-worklet.js", "sloop-preview.wasm"):
        if (app / "editor" / f).exists():
            keep["editor/" + f] = app / "editor" / f
    for f in ICONS:
        keep[f] = app / f
    h = hashlib.sha256()
    for k in sorted(keep):
        h.update(k.encode() + b"\0" + keep[k].read_bytes())
    sw = (HERE / "sw.js").read_text(encoding="utf-8")
    for mark in ("/*STAMP*/", "/*FILES*/[]"):
        if sw.count(mark) != 1:
            raise SystemExit(f"sw.js must contain {mark} once; update make_site.py")
    sw = sw.replace("/*STAMP*/", f"{version}-{h.hexdigest()[:12]}").replace("/*FILES*/[]", json.dumps(sorted(keep)))
    (app / "sw.js").write_text(sw, encoding="utf-8")
    return sorted(keep)
BLOCKS, BLK, KEEP = 20, 0x30, 0x2F


def strip_module(src):
    src = re.sub(r"^export\s+", "", src, flags=re.M)
    return re.sub(r"^import .*?;\n", "", src, flags=re.M)


def product_of(raw):
    """the package identity: one marker byte after each of the first 20 blocks (fm1pkg.js productOf)"""
    return "".join(chr((m - i - 1) & 0xFF) for i in range(BLOCKS) if (m := raw[i * BLK + KEEP]) != 0x7D)


def main(pkg, version, out):
    pkg, out = Path(pkg), Path(out)
    raw = pkg.read_bytes()
    product = product_of(raw)
    if not re.fullmatch(r"FM-1_(9\d\d|123)", product):
        raise SystemExit(f"{pkg}: identity {product!r} is not a Felucca / M-PYRE package (FM-1_9xx, FM-1_123)")
    if b"FELUCCA-LOADER-1" not in raw:              # marker of firmware/loader
        raise SystemExit(f"{pkg}: no Felucca update loader in it")
    html = (HERE / "index_pkg.html").read_text(encoding="utf-8")
    lib = strip_module((HERE / "fm1pkg.js").read_text(encoding="utf-8")) + "\n" + \
        strip_module((HERE / "fm1ota.js").read_text(encoding="utf-8"))
    name = f"m-pyre-{re.sub(r'[^A-Za-z0-9.-]', '-', version)}.fwsc"
    # M-PYRE: the sound pack next to the firmware: the installer writes it right after (USR1..USR4)
    pack = HERE.parent / "build" / "mpyre" / "soundpack.bin"
    pack_meta = {"pack": "../../firmware/soundpack.bin", "packSha256": hashlib.sha256(pack.read_bytes()).hexdigest()} \
        if pack.exists() else {}
    meta = json.dumps({"version": version, "product": product, "pkg": "../../firmware/" + name,
                       "sha256": hashlib.sha256(raw).hexdigest(), **pack_meta})   # the page checks the download against it
    ed_src = (HERE / "editor.html").read_text(encoding="utf-8")
    a, b = ed_src.find("/*PROTO-BEGIN*/"), ed_src.find("/*PROTO-END*/")
    if a < 0 or b < a:
        raise SystemExit("editor.html must hold /*PROTO-BEGIN*/ .. /*PROTO-END*/; update make_site.py")
    html = html.replace("/*PROTO*/", "const EP = (() => {\n" + ed_src[a:b] +
                        "\nreturn { Link, req, parse, CMD, SMP, fm1Ports, soundPackSlots };\n})();")   # (the editor's protocol)
    for mark in ("/*LIB*/", "/*META*/"):
        if html.count(mark) != 1:
            raise SystemExit(f"index_pkg.html must contain {mark} once; update make_site.py")
    html = html.replace("/*LIB*/", lib).replace("/*META*/", meta)
    splash = HERE.parent / "assets" / "mpyre" / "splash.png"      # M-PYRE: the boot picture, inline
    html = html.replace("<!--LOGO-->", '<img alt="" src="data:image/png;base64,' +
                        base64.b64encode(splash.read_bytes()).decode() + '">' if splash.exists() else "")
    font = HERE.parent / "assets" / "fonts" / "BarlowCondensed-SemiBold.ttf"   # (SIL OFL 1.1: assets/fonts)
    if font.exists():
        html = html.replace('/*FONT*/local("Barlow Condensed SemiBold")',
                            'url(data:font/ttf;base64,' + base64.b64encode(font.read_bytes()).decode() + ') format("truetype")')
    html = html.replace("/*REPO*/", SOURCE).replace("/*REPO_SHORT*/", SOURCE.replace("https://", ""))
    inst, ed, fw = out / "webapp" / "installer", out / "webapp" / "editor", out / "firmware"
    for d in (inst, ed, fw):
        d.mkdir(parents=True, exist_ok=True)
    guide = HERE.parent / "output/pdf/Studio-0.3-guide-rapide-FR.pdf"
    guide_link = ""
    if guide.exists() and "--studio-guide" in sys.argv:   # LIVE: that guide describes the removed RYTHME tab
        shutil.copy(guide, out / guide.name)
        guide_link = '<p lang="fr"><a href="../../' + guide.name + '">Guide rapide illustré (PDF, 4 pages)</a></p>'
    html = html.replace("<!--STUDIO_GUIDE-->", guide_link)
    banner = ""
    if "--beta" in sys.argv:                       # the beta channel (docs/beta): what it is, what it implies
        notes = HERE / "beta_banner.html"
        banner = notes.read_text(encoding="utf-8") if notes.exists() else "<p><b>BETA</b></p>"
    html = html.replace("<!--BANNER-->", banner)
    media = HERE.parent / "assets" / "mpyre" / "video"            # M-PYRE: the promo video, next to the site (not kept offline)
    if (media / "m-pyre-promo.mp4").exists():
        (out / "media").mkdir(parents=True, exist_ok=True)
        for f in ("m-pyre-promo.mp4", "m-pyre-promo-poster.jpg"):
            shutil.copy(media / f, out / "media" / f)
        html = html.replace("<!--VIDEO-->", '<div class="video"><video controls preload="none" playsinline '
                            'poster="../../media/m-pyre-promo-poster.jpg" src="../../media/m-pyre-promo.mp4"></video>'
                            '<p>M-PYRE in two minutes: everything in it rendered by the firmware. Music by History Of Sky: '
                            '<a href="https://open.spotify.com/track/0zcH8TBsfImHzdnM2COh83" target="_blank" rel="noopener">The Prism</a> · '
                            '<a href="https://open.spotify.com/track/41KA0kkv4ogBhnwRf7uqUN" target="_blank" rel="noopener">The Quintessence</a> · '
                            '<a href="https://open.spotify.com/track/2sZQYQhxmWdmGsvXrxyEB4" target="_blank" rel="noopener">The Truth (Parts I–III)</a> · '
                            '<a href="https://open.spotify.com/artist/4ehdjRy2ETpUSkIbtuEG3S" target="_blank" rel="noopener">Spotify</a> · '
                            '<a href="https://www.instagram.com/historyofsky/" target="_blank" rel="noopener">Instagram</a></p></div>')
    html = html.replace("<!--VIDEO-->", "")
    for old in list(fw.glob("felucca-*.fwsc")) + list(fw.glob("sloop-*.fwsc")) + list(fw.glob("m-pyre-*.fwsc")):   # one package: the current one
        old.unlink()
    shutil.copy(pkg, fw / name)
    if pack.exists():
        shutil.copy(pack, fw / "soundpack.bin")
    ed_html = (HERE / "editor.html").read_text(encoding="utf-8")
    if font.exists():                              # M-PYRE: the editor's face too, inline (it also runs offline)
        ed_html = ed_html.replace('/*FONT*/local("Barlow Condensed SemiBold")',
                                  'url(data:font/ttf;base64,' + base64.b64encode(font.read_bytes()).decode() + ') format("truetype")')
    dl = f'download="m-pyre-editor-{re.sub(r"[^A-Za-z0-9.-]", "-", version)}.html"'
    if ed_html.count('download="m-pyre-editor.html"') != 1 or html.count('download="m-pyre-editor.html"') != 1:
        raise SystemExit('editor.html and index_pkg.html must link download="m-pyre-editor.html" once; update make_site.py')
    (ed / "index.html").write_text(ed_html.replace('download="m-pyre-editor.html"', dl), encoding="utf-8")
    (ed / "m-pyre-editor.html").write_text(offline_editor(ed_html), encoding="utf-8")
    (inst / "index.html").write_text(html.replace('download="m-pyre-editor.html"', dl), encoding="utf-8")
    for f in ("fukiai.ttf", "FUKIAI-LICENSE.txt"):
        if (HERE / f).exists():
            shutil.copy(HERE / f, ed / f)
    # Optional browser DSP, built separately without the hardware toolchain.
    for f in ("preview.js", "preview-worklet.js"):
        shutil.copy(HERE / f, ed / f)
    dsp = HERE.parent / "build" / "preview" / "sloop-preview.wasm"
    if dsp.exists():
        shutil.copy(dsp, ed / dsp.name)
    elif (ed / dsp.name).exists():
        (ed / dsp.name).unlink()  # never serve an old audio build as the current one
    emu = HERE.parent / "build" / "emu" / "m-pyre-emulator.html"     # tools/build_emu.py: the firmware in the browser
    if emu.exists():
        (out / "emu").mkdir(parents=True, exist_ok=True)
        (out / "emu" / "index.html").write_text(
            '<!doctype html>\n<html lang="en">\n<head>\n<meta charset="utf-8">\n'
            '<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">\n'
            '<meta name="theme-color" content="#0b0908">\n</head>\n<body>\n'
            + emu.read_text(encoding="utf-8") + '\n</body>\n</html>\n', encoding="utf-8")
    app_files(out, version, name)
    (out / "index.html").write_text(
        '<!doctype html><meta charset="utf-8"><title>M-PYRE</title>'
        '<meta http-equiv="refresh" content="0; url=webapp/installer/">'
        '<a href="webapp/installer/">M-PYRE installer</a>\n', encoding="utf-8")
    print(f"site: {out}: webapp/installer ({len(html)} B), webapp/editor, firmware/{name} ({len(raw)} B, {product})")


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if a not in ("--studio-guide", "--beta")]
    if len(args) != 3:
        sys.exit(__doc__)
    main(*args)
