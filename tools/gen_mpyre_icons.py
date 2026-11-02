#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 History Of Sky (M-PYRE)
"""M-PYRE: the web app's icons (web/icon-192.png, icon-512.png, icon-maskable-512.png): the name in Barlow Condensed
Bold (assets/fonts, SIL OFL 1.1) on the embers of the boot picture's colours"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parents[1]
FONT = ROOT / "assets" / "fonts" / "BarlowCondensed-Bold.ttf"


def icon(size, safe):
    """safe: the share of the width the name may take (a maskable icon is cut to a circle: 0.62)"""
    s = size * 4                                        # drawn large, scaled down: smooth edges
    im = Image.new("RGB", (s, s), (8, 0, 0))
    px = im.load()
    for y in range(s):                                  # the glow from below: black-red to fire
        t = max(0.0, (y / s - 0.35) / 0.65) ** 1.6
        c = (int(8 + 222 * t), int(0 + 70 * t * t), int(0 + 10 * t * t))
        for x in range(s):
            px[x, y] = c
    d = ImageDraw.Draw(im)
    pt = s
    while True:
        f = ImageFont.truetype(str(FONT), pt)
        l, t_, r, b = d.textbbox((0, 0), "M-PYRE", font=f)
        if r - l <= s * safe:
            break
        pt -= 8
    x, y = (s - (r - l)) // 2 - l, (s - (b - t_)) // 2 - t_ - s // 20
    glow = Image.new("RGB", (s, s), (0, 0, 0))
    ImageDraw.Draw(glow).text((x, y), "M-PYRE", font=f, fill=(255, 120, 20))
    glow = glow.filter(ImageFilter.GaussianBlur(s / 60))
    im = Image.blend(im, Image.composite(glow, im, glow.convert("L").point(lambda v: min(255, v * 2))), 0.55)
    ImageDraw.Draw(im).text((x, y), "M-PYRE", font=f, fill=(255, 140, 30))
    return im.resize((size, size), Image.LANCZOS)


if __name__ == "__main__":
    web = ROOT / "web"
    icon(192, 0.84).save(web / "icon-192.png")
    icon(512, 0.84).save(web / "icon-512.png")
    icon(512, 0.62).save(web / "icon-maskable-512.png")
    print("icons: web/icon-192.png, icon-512.png, icon-maskable-512.png")
