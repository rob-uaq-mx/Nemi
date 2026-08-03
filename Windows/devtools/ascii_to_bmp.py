#!/usr/bin/env python3
"""ascii_to_bmp.py -- convert a hex-digit pixel grid into a 4bpp (16-color)
Windows BMP, for hand-authoring classic toolbar-button bitmaps.

Usage:
    python ascii_to_bmp.py entrada.art.txt salida.bmp

Input format: a plain text file with exactly 16 lines (one per pixel row).
Each line is exactly `16 * n` hex digits (0-9, A-F, case-insensitive), no
separators, where n is the number of 16x16 icons laid out side by side
(n = 1 for a single icon, n = 9 for a 9-button toolbar strip, ...). Each
hex digit is one pixel, indexed into the classic 16-color VGA/EGA console
palette below -- see devtools/art/*.art.txt for example input.
"""
import sys

# Classic 16-color VGA/EGA console palette (index 0x0-0xF), as (R, G, B).
PALETTE = [
    (0x00, 0x00, 0x00),  # 0 black
    (0x80, 0x00, 0x00),  # 1 dark red
    (0x00, 0x80, 0x00),  # 2 dark green
    (0x80, 0x80, 0x00),  # 3 dark yellow / olive
    (0x00, 0x00, 0x80),  # 4 dark blue / navy
    (0x80, 0x00, 0x80),  # 5 dark magenta / purple
    (0x00, 0x80, 0x80),  # 6 dark cyan / teal
    (0xC0, 0xC0, 0xC0),  # 7 light gray / silver
    (0x80, 0x80, 0x80),  # 8 dark gray
    (0xFF, 0x00, 0x00),  # 9 red
    (0x00, 0xFF, 0x00),  # A green
    (0xFF, 0xFF, 0x00),  # B yellow
    (0x00, 0x00, 0xFF),  # C blue
    (0xFF, 0x00, 0xFF),  # D magenta
    (0x00, 0xFF, 0xFF),  # E cyan
    (0xFF, 0xFF, 0xFF),  # F white
]

ROW_HEIGHT = 16  # every icon cell is 16 px tall


def read_grid(path):
    with open(path, encoding="ascii") as f:
        lines = f.read().splitlines()
    if len(lines) != ROW_HEIGHT:
        raise ValueError(f"expected {ROW_HEIGHT} rows, got {len(lines)}")
    width = len(lines[0])
    if width == 0 or width % ROW_HEIGHT != 0:
        raise ValueError(f"row width {width} must be a positive multiple of {ROW_HEIGHT}")
    for i, row in enumerate(lines):
        if len(row) != width:
            raise ValueError(f"row {i} has length {len(row)}, expected {width}")
        for ch in row:
            if ch.upper() not in "0123456789ABCDEF":
                raise ValueError(f"row {i} has invalid hex digit {ch!r}")
    return lines, width


def build_bmp(rows, width):
    height = ROW_HEIGHT
    row_bytes = (width + 1) // 2          # 2 pixels per byte (4bpp)
    padded_row_bytes = (row_bytes + 3) & ~3  # rows padded to a 4-byte boundary

    # Pixel data is stored bottom-up (BMP convention for positive biHeight).
    pixel_data = bytearray()
    for row in reversed(rows):
        packed = bytearray(padded_row_bytes)
        for x, ch in enumerate(row):
            nibble = int(ch, 16)
            byte_index = x // 2
            if x % 2 == 0:
                packed[byte_index] |= nibble << 4
            else:
                packed[byte_index] |= nibble
        pixel_data.extend(packed)

    palette_bytes = bytearray()
    for (r, g, b) in PALETTE:
        palette_bytes += bytes((b, g, r, 0))  # BMP palette entries are BGR + reserved

    header_size = 14 + 40 + len(palette_bytes)  # BITMAPFILEHEADER + BITMAPINFOHEADER + palette
    file_size = header_size + len(pixel_data)

    file_header = bytearray()
    file_header += b"BM"
    file_header += file_size.to_bytes(4, "little")
    file_header += (0).to_bytes(2, "little")   # bfReserved1
    file_header += (0).to_bytes(2, "little")   # bfReserved2
    file_header += header_size.to_bytes(4, "little")  # bfOffBits

    info_header = bytearray()
    info_header += (40).to_bytes(4, "little")               # biSize
    info_header += width.to_bytes(4, "little", signed=True)   # biWidth
    info_header += height.to_bytes(4, "little", signed=True)  # biHeight (+ = bottom-up)
    info_header += (1).to_bytes(2, "little")                # biPlanes
    info_header += (4).to_bytes(2, "little")                # biBitCount
    info_header += (0).to_bytes(4, "little")                # biCompression (BI_RGB)
    info_header += len(pixel_data).to_bytes(4, "little")    # biSizeImage
    info_header += (0).to_bytes(4, "little", signed=True)   # biXPelsPerMeter
    info_header += (0).to_bytes(4, "little", signed=True)   # biYPelsPerMeter
    info_header += (16).to_bytes(4, "little")               # biClrUsed
    info_header += (16).to_bytes(4, "little")               # biClrImportant

    return bytes(file_header + info_header + palette_bytes + pixel_data)


def main():
    if len(sys.argv) != 3:
        print("uso: python ascii_to_bmp.py entrada.art.txt salida.bmp", file=sys.stderr)
        return 1
    rows, width = read_grid(sys.argv[1])
    data = build_bmp(rows, width)
    with open(sys.argv[2], "wb") as f:
        f.write(data)
    print(f"escrito {sys.argv[2]}: {width}x{ROW_HEIGHT}px, {len(data)} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
