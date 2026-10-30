#!/usr/bin/env python3
"""Render the project SVG mark into a multi-resolution Windows ICO."""
from pathlib import Path
import cairosvg
from PIL import Image
from io import BytesIO

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'assets' / 'telegram-aero.svg'
TARGET = ROOT / 'assets' / 'aero7-gram.ico'
SIZES = (16, 20, 24, 32, 40, 48, 64, 128, 256)

if not SOURCE.is_file():
    raise SystemExit(f'Missing icon source: {SOURCE}')

images = []
for size in SIZES:
    png = cairosvg.svg2png(url=str(SOURCE), output_width=size, output_height=size)
    image = Image.open(BytesIO(png)).convert('RGBA')
    images.append(image)

images[-1].save(TARGET, format='ICO', sizes=[(s, s) for s in SIZES],
                append_images=images[:-1])
print(f'Generated {TARGET} ({TARGET.stat().st_size} bytes)')
