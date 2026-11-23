#!/usr/bin/env python3
# Witchaven 360: draws the controller on the Controls page (xenon/wh_menu360.c) and writes it as
# xenon/controls_art.h, in Witchaven's palette, run-length coded. As Heretic 360's tool of the same name.
#
#   python3 tools/make_controls_art.py path/to/palette.dat art/360_controller.svg
#
# Needs Python 3, Pillow and rsvg-convert (librsvg2-bin). palette.dat gives only Witchaven's palette, to pick the
# nearest colours; nothing of it goes into the header.

import subprocess
import sys
import tempfile

from PIL import Image

# The drawing's height on the page, in the 640x480 screen's pixels; its width follows.
ART_H = 176


def read_palette(path):
    raw = open(path, 'rb').read(768)
    # 6-bit VGA levels, 0 to 63.
    return [tuple(min(255, c * 4 + c // 16) for c in raw[i * 3:i * 3 + 3]) for i in range(256)]


def nearest(palette, rgb):
    best, best_d = 0, None
    # Index 255 is Build's transparent colour; never pick it.
    for i, p in enumerate(palette[:255]):
        # Weighted for the eye: green counts most.
        d = 2 * (p[0] - rgb[0]) ** 2 + 4 * (p[1] - rgb[1]) ** 2 + 3 * (p[2] - rgb[2]) ** 2
        if best_d is None or d < best_d:
            best, best_d = i, d
    return best


def picture(path):
    png = tempfile.NamedTemporaryFile(suffix='.png', delete=False).name
    subprocess.run(['rsvg-convert', '-w', '2000', path, '-o', png], check=True)
    img = Image.open(png).convert('RGBA')
    img = img.crop(img.getbbox())
    w = round(img.size[0] * ART_H / img.size[1])
    return img.resize((w, ART_H), Image.LANCZOS)


def encode(img, palette):
    # Per byte h: h & 0x80, a run of (h & 0x7f) + 1 clear pixels; otherwise h + 1 palette indexes follow.
    px = img.load()
    w, h = img.size
    cache = {}
    pixels = []
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a < 128:
                pixels.append(None)
            else:
                if (r, g, b) not in cache:
                    cache[(r, g, b)] = nearest(palette, (r, g, b))
                pixels.append(cache[(r, g, b)])
    out = bytearray()
    i = 0
    while i < len(pixels):
        n = 1
        if pixels[i] is None:
            while i + n < len(pixels) and pixels[i + n] is None and n < 128:
                n += 1
            out.append(0x80 | (n - 1))
        else:
            while i + n < len(pixels) and pixels[i + n] is not None and n < 128:
                n += 1
            out.append(n - 1)
            out.extend(pixels[i:i + n])
        i += n
    return bytes(out)


def main():
    if len(sys.argv) != 3:
        raise SystemExit('usage: make_controls_art.py palette.dat art/360_controller.svg')
    palette = read_palette(sys.argv[1])
    art = picture(sys.argv[2])
    data = encode(art, palette)
    lines = [
        '// Witchaven 360: the controller on the Controls page, written by tools/make_controls_art.py from',
        '// art/360_controller.svg (CC BY-SA 3.0, art/README.md). Run-length coded indexes into Witchaven\'s palette;',
        '// xenon/wh_menu360.c draws it.',
        '',
        '#define CONTROLS_ART_W %d' % art.size[0],
        '#define CONTROLS_ART_H %d' % art.size[1],
        '',
        'static const unsigned char controls_art[%d] = {' % len(data),
    ]
    for i in range(0, len(data), 24):
        lines.append('    ' + ', '.join(str(b) for b in data[i:i + 24]) + ',')
    lines.append('};')
    open('xenon/controls_art.h', 'w').write('\n'.join(lines) + '\n')
    # A preview, to look at.
    prev = Image.new('RGB', art.size, (40, 0, 0))
    pp = prev.load(); ap = art.load()
    for y in range(art.size[1]):
        for x in range(art.size[0]):
            if ap[x, y][3] >= 128:
                pp[x, y] = palette[nearest(palette, ap[x, y][:3])]
    if len(sys.argv) == 3:
        prev.save(tempfile.gettempdir() + '/controls_art_preview.png')
    print('xenon/controls_art.h: %dx%d, %d bytes' % (art.size[0], art.size[1], len(data)))


main()
