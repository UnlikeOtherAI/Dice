#!/usr/bin/env python3
"""Generate texture atlases for each die type.

Each atlas is a 512x512 RGBA PNG with white numbers on a transparent background.
Grid layout: cols = ceil(sqrt(N)), rows = ceil(N / cols).
"""

import math
import os
from PIL import Image, ImageDraw, ImageFont

ATLAS_SIZE = 512
CELL_PADDING_RATIO = 0.18
OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "..", "core", "assets", "atlas")

DIE_TYPES = [4, 6, 8, 10, 12, 16, 20, 32]
GLYPH_BOX_RATIO_BY_DIE = {
    4: 0.52,
    6: 0.62,
    8: 0.42,
    10: 0.34,
    12: 0.58,
    16: 0.30,
    20: 0.52,
    32: 0.52,
}


def load_font(size: int) -> ImageFont.ImageFont:
    try:
        return ImageFont.truetype("DejaVuSans-Bold.ttf", size=size)
    except OSError:
        return ImageFont.load_default(size=size)


def fit_font(labels: list[str], max_w: int, max_h: int) -> ImageFont.ImageFont:
    probe = Image.new("RGBA", (ATLAS_SIZE, ATLAS_SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(probe)
    for size in range(min(max_w, max_h), 1, -1):
        font = load_font(size)
        if all(
            (bbox := draw.textbbox((0, 0), text, font=font))[2] - bbox[0] <= max_w
            and bbox[3] - bbox[1] <= max_h
            for text in labels
        ):
            return font
    return load_font(12)


def make_atlas(n: int) -> Image.Image:
    cols = math.ceil(math.sqrt(n))
    rows = math.ceil(n / cols)

    cell_w = ATLAS_SIZE // cols
    cell_h = ATLAS_SIZE // rows

    cell_padding_x = int(cell_w * CELL_PADDING_RATIO)
    cell_padding_y = int(cell_h * CELL_PADDING_RATIO)
    inner_w = cell_w - 2 * cell_padding_x
    inner_h = cell_h - 2 * cell_padding_y
    glyph_ratio = GLYPH_BOX_RATIO_BY_DIE.get(n, 0.52)
    glyph_box_w = max(1, int(inner_w * glyph_ratio))
    glyph_box_h = max(1, int(inner_h * glyph_ratio))
    font = fit_font([str(i) for i in range(1, n + 1)], glyph_box_w, glyph_box_h)

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

        # Center in a smaller safe box inside the cell so triangular faces do
        # not sample oversized digits near the edges.
        x = cell_x + cell_padding_x + (inner_w - text_w) // 2 - bbox[0]
        y = cell_y + cell_padding_y + (inner_h - text_h) // 2 - bbox[1]

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
