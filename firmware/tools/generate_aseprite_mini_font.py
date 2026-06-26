#!/usr/bin/env python3
r"""
Generate an Arduino bitmap font header from Aseprite Mini.

Preferred input is a user-supplied .otf/.ttf font file:

  python firmware/tools/generate_aseprite_mini_font.py ^
    --input C:\path\to\aseprite-mini.otf ^
    --output firmware/src/mp3gotchi-0.1.7/AsepriteMiniFontData.h ^
    --font-size 5

PNG sheet input is still supported for old/manual workflows.
"""

from __future__ import annotations

import argparse
from pathlib import Path


DEFAULT_CHARS = "".join(chr(code) for code in range(32, 127))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Convert Aseprite Mini into an Arduino bitmap font header.")
    parser.add_argument("--input", required=True, type=Path, help="Path to a user-supplied .otf/.ttf font or PNG sheet.")
    parser.add_argument("--output", required=True, type=Path, help="Output header path.")
    parser.add_argument("--chars", default=DEFAULT_CHARS, help="Characters to export.")
    parser.add_argument("--font-size", type=int, default=5, help="Pixel size for .otf/.ttf input.")
    parser.add_argument("--char-width", type=int, default=3, help="PNG sheet glyph width.")
    parser.add_argument("--char-height", type=int, default=5, help="PNG sheet glyph height.")
    parser.add_argument("--columns", type=int, default=16, help="PNG sheet columns.")
    parser.add_argument("--x-offset", type=int, default=0)
    parser.add_argument("--y-offset", type=int, default=0)
    parser.add_argument("--x-spacing", type=int, default=1)
    parser.add_argument("--y-spacing", type=int, default=1)
    parser.add_argument("--threshold", type=int, default=16, help="Alpha/luma threshold for a lit pixel.")
    return parser.parse_args()


def lit_pixel(pixel: object, threshold: int) -> bool:
    if isinstance(pixel, int):
        return pixel > threshold
    channels = tuple(pixel)
    if len(channels) >= 4 and channels[3] <= threshold:
        return False
    return max(channels[:3]) > threshold


def rows_from_png(args: argparse.Namespace) -> list[tuple[str, int, list[int]]]:
    from PIL import Image

    image = Image.open(args.input).convert("RGBA")
    glyphs: list[tuple[str, int, list[int]]] = []

    for index, char in enumerate(args.chars):
        col = index % args.columns
        row = index // args.columns
        x0 = args.x_offset + col * (args.char_width + args.x_spacing)
        y0 = args.y_offset + row * (args.char_height + args.y_spacing)
        rows: list[int] = []
        for y in range(args.char_height):
            bits = 0
            for x in range(args.char_width):
                if lit_pixel(image.getpixel((x0 + x, y0 + y)), args.threshold):
                    bits |= 1 << (args.char_width - 1 - x)
            rows.append(bits)
        glyphs.append((char, args.char_width, rows))

    return glyphs


def rows_from_font(args: argparse.Namespace) -> list[tuple[str, int, list[int]]]:
    from PIL import Image, ImageDraw, ImageFont

    font = ImageFont.truetype(str(args.input), size=args.font_size)
    probe = ImageDraw.Draw(Image.new("L", (1, 1), 0))
    glyphs: list[tuple[str, int, list[int]]] = []

    for char in args.chars:
        advance = int(round(probe.textlength(char, font=font)))
        if advance <= 0:
            advance = 1

        image = Image.new("L", (max(advance, 1), args.font_size), 0)
        draw = ImageDraw.Draw(image)
        draw.text((0, 0), char, font=font, fill=255)
        rows: list[int] = []
        for y in range(args.font_size):
            bits = 0
            for x in range(advance):
                if image.getpixel((x, y)) > args.threshold:
                    bits |= 1 << (advance - 1 - x)
            rows.append(bits)
        glyphs.append((char, advance, rows))

    return glyphs


def escaped_char(char: str) -> str:
    if char == "'":
        return "\\'"
    if char == "\\":
        return "\\\\"
    if char == "\t":
        return "\\t"
    return char


def write_header(path: Path, glyphs: list[tuple[str, int, list[int]]], height: int, source: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="\n") as out:
        out.write("#pragma once\n")
        out.write("#include <Arduino.h>\n\n")
        out.write("// Generated from the real Aseprite Mini font asset.\n")
        out.write("// Original font: Aseprite Font by David Capello.\n")
        out.write("// Original license: Creative Commons Attribution 4.0 International.\n")
        out.write("// Source license URL: https://github.com/aseprite/aseprite/blob/main/data/fonts/LICENSE.txt\n")
        out.write("// Modification: converted to compact C/C++ bitmap glyph data for MP3-Gotchi firmware.\n")
        out.write(f"// Source file: {source.name}\n")
        out.write(f"constexpr uint8_t ASEPRITE_MINI_FONT_HEIGHT = {height};\n")
        out.write(f"constexpr uint8_t ASEPRITE_MINI_FONT_GLYPH_COUNT = {len(glyphs)};\n\n")
        out.write("struct AsepriteMiniGlyph {\n")
        out.write("  char code;\n")
        out.write("  uint8_t width;\n")
        out.write(f"  uint8_t rows[{height}];\n")
        out.write("};\n\n")
        out.write("const AsepriteMiniGlyph ASEPRITE_MINI_FONT[] PROGMEM = {\n")
        for char, width, rows in glyphs:
            bits = ", ".join(f"0b{value:0{width}b}" for value in rows)
            out.write(f"  {{'{escaped_char(char)}', {width}, {{{bits}}}}},\n")
        out.write("};\n")


def main() -> None:
    args = parse_args()
    try:
        import PIL  # noqa: F401
    except ImportError as exc:
        raise SystemExit("Pillow is required: python -m pip install pillow") from exc

    suffix = args.input.suffix.lower()
    if suffix in {".otf", ".ttf"}:
        glyphs = rows_from_font(args)
        height = args.font_size
    else:
        glyphs = rows_from_png(args)
        height = args.char_height

    write_header(args.output, glyphs, height, args.input)
    print(f"Wrote {args.output}")


if __name__ == "__main__":
    main()
