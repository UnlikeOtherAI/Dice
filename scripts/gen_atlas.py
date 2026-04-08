#!/usr/bin/env python3
"""Generate texture atlases for each die type.

Each atlas is a 512x512 RGBA PNG with white numbers on a transparent background.
Grid layout: cols = ceil(sqrt(N)), rows = ceil(N / cols). 8px padding per cell.
"""

import math
import os
from PIL import Image, ImageDraw, ImageFont

ATLAS_SIZE = 512
PADDING = 8
OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "..", "core", "assets", "atlas")

DIE_TYPES = [4, 6, 8, 10, 12, 16, 20, 32]


def make_atlas(n: int) -> Image.Image:
    cols = math.ceil(math.sqrt(n))
    rows = math.ceil(n / cols)

    cell_w = ATLAS_SIZE // cols
    cell_h = ATLAS_SIZE // rows

    inner_w = cell_w - 2 * PADDING
    inner_h = cell_h - 2 * PADDING

    # Pick the largest font size that fits within the inner cell area
    font_size = min(inner_w, inner_h)
    font = ImageFont.load_default(size=font_size)

    img = Image.new("RGBA", (ATLAS_SIZE, ATLAS_SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    for i in range(1, n + 1):
        col = (i - 1) % cols
        row = (i - 1) // cols

        cell_x = col * cell_w
        cell_y = row * cell_h

        text = str(i)
        bbox = draw.textbbox((0, 0), text, font=font)
        text_w = bbox[2] - bbox[0]
        text_h = bbox[3] - bbox[1]

        # Center in the inner area
        x = cell_x + PADDING + (inner_w - text_w) // 2 - bbox[0]
        y = cell_y + PADDING + (inner_h - text_h) // 2 - bbox[1]

        draw.text((x, y), text, fill=(255, 255, 255, 255), font=font)

    return img


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    for n in DIE_TYPES:
        atlas = make_atlas(n)
        out_path = os.path.join(OUTPUT_DIR, f"d{n}.png")
        atlas.save(out_path)
        print(f"Saved {out_path}")


if __name__ == "__main__":
    main()
