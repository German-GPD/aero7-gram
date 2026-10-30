#!/usr/bin/env python3
"""Generate a multi-resolution Windows ICO using only Pillow (no native SVG runtime)."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / 'assets' / 'aero7-gram.ico'
N = 512

# Render at high resolution, then downsample each ICO frame for crisp small sizes.
base = Image.new('RGBA', (N, N), (0, 0, 0, 0))
pixels = base.load()
cx = cy = N / 2
radius = N * 0.485
for y in range(N):
    for x in range(N):
        dx, dy = x - cx, y - cy
        d = (dx * dx + dy * dy) ** 0.5
        if d <= radius:
            t = max(0.0, min(1.0, (y / N) * 0.82 + (x / N) * 0.18))
            top = (83, 207, 246)
            bottom = (5, 91, 166)
            shade = 1.0 - 0.10 * (d / radius) ** 2
            pixels[x, y] = tuple(int((top[i] * (1 - t) + bottom[i] * t) * shade) for i in range(3)) + (255,)

# Thin glass rim and restrained Windows 7 highlight.
draw = ImageDraw.Draw(base, 'RGBA')
draw.ellipse((N*0.015, N*0.015, N*0.985, N*0.985), outline=(226, 249, 255, 235), width=5)
draw.ellipse((N*0.055, N*0.045, N*0.945, N*0.60), fill=(255, 255, 255, 28))
draw.arc((N*0.06, N*0.04, N*0.94, N*0.94), 195, 340, fill=(255,255,255,110), width=5)

# Paper-plane silhouette with a subtle drop shadow.
plane = [(98, 226), (365, 119), (405, 103), (397, 151), (355, 384), (345, 419), (319, 410), (215, 341), (177, 352), (143, 338), (102, 325), (69, 311), (82, 284)]
plane = [(int(x*N/512), int(y*N/512)) for x, y in plane]
shadow = Image.new('RGBA', (N, N), (0, 0, 0, 0))
sd = ImageDraw.Draw(shadow)
sd.polygon([(x, y+8) for x, y in plane], fill=(0, 35, 70, 105))
shadow = shadow.filter(ImageFilter.GaussianBlur(8))
base = Image.alpha_composite(base, shadow)
draw = ImageDraw.Draw(base, 'RGBA')
draw.polygon(plane, fill=(246, 252, 255, 255), outline=(210, 240, 255, 255))
# Cut the characteristic swept inner fold into the plane.
draw.polygon([(214, 341), (311, 259), (405, 103), (355, 384), (345, 419), (319, 410)], fill=(190, 229, 249, 255))
draw.polygon([(214, 341), (310, 258), (177, 352)], fill=(224, 244, 255, 255))
draw.line([(102, 226), (310, 258), (405, 103)], fill=(255,255,255,210), width=4, joint='curve')

sizes = (16, 20, 24, 32, 40, 48, 64, 128, 256)
frames = [base.resize((size, size), Image.Resampling.LANCZOS) for size in sizes]
frames[-1].save(TARGET, format='ICO', sizes=[(size, size) for size in sizes],
                append_images=frames[:-1])
print(f'Generated {TARGET} ({TARGET.stat().st_size} bytes)')
