#!/usr/bin/env python3
"""Genera una imagen PNG de un tablero de ajedrez 9x6 (esquinas internas)
sin dependencias externas (escritor PNG manual con zlib)."""
import struct, zlib, sys

SQ = 60            # lado de cada cuadrado en px
COLS, ROWS = 10, 7 # cuadrados (=> 9x6 esquinas internas)
MARGIN = 60        # borde blanco obligatorio para findChessboardCorners

W = COLS * SQ + 2 * MARGIN
H = ROWS * SQ + 2 * MARGIN

def pixel(x, y):
    bx, by = x - MARGIN, y - MARGIN
    if bx < 0 or by < 0 or bx >= COLS * SQ or by >= ROWS * SQ:
        return 255
    return 0 if ((bx // SQ) + (by // SQ)) % 2 == 0 else 255

rows = bytearray()
for y in range(H):
    rows.append(0)  # filtro PNG: None
    for x in range(W):
        v = pixel(x, y)
        rows += bytes((v, v, v))

def chunk(tag, data):
    c = struct.pack(">I", len(data)) + tag + data
    return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

png = b"\x89PNG\r\n\x1a\n"
png += chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0))
png += chunk(b"IDAT", zlib.compress(bytes(rows), 9))
png += chunk(b"IEND", b"")

out = sys.argv[1]
with open(out, "wb") as f:
    f.write(png)
print(f"OK {out} ({W}x{H})")
