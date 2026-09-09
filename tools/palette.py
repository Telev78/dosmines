#!/usr/bin/env python3
"""
Extrait la palette de couleurs d'un fichier BMP (4-bits ou 8-bits)
et génère une image PNG avec des pavés d'échantillons, leurs index et leurs codes Hex
disposés en quinconce pour faciliter la lecture.

Usage :
    python tools/palette.py [chemin_bmp] [chemin_sortie_png]

Par défaut :
    Lit "asset/asset.bmp" et génère "asset/palette_extracted.png".
"""

import sys
import os
import struct
from PIL import Image, ImageDraw, ImageFont

def extract_palette(bmp_path="asset/asset.bmp", output_path="asset/palette_extracted.png"):
    if not os.path.exists(bmp_path):
        print(f"Erreur : fichier '{bmp_path}' introuvable.")
        sys.exit(1)

    with open(bmp_path, "rb") as f:
        header = f.read(54)
        if len(header) < 54 or header[:2] != b'BM':
            print(f"Erreur : '{bmp_path}' n'est pas un fichier BMP valide.")
            sys.exit(1)

        bpp = struct.unpack('<H', header[28:30])[0]
        num_colors_header = struct.unpack('<I', header[46:50])[0]

        if num_colors_header == 0:
            color_count = 1 << bpp
        else:
            color_count = num_colors_header

        color_count = min(color_count, 256)
        print(f"Nombre de couleurs trouvées dans la palette : {color_count}")

        palette_bytes = f.read(color_count * 4)

    colors = []
    display_count = min(16, color_count)
    for i in range(display_count):
        b = palette_bytes[i * 4]
        g = palette_bytes[i * 4 + 1]
        r = palette_bytes[i * 4 + 2]
        colors.append((r, g, b))

    # Configuration des dimensions du rendu
    swatch_size = 50
    out_width = swatch_size * display_count
    out_height = swatch_size + 65  # Espace supplémentaire pour le quinconce

    img = Image.new("RGB", (out_width, out_height), (255, 255, 255))
    draw = ImageDraw.Draw(img)

    # Chargement d'une police TrueType système ou police par défaut
    try:
        font_index = ImageFont.truetype("arialbd.ttf", 11)
        font_hex = ImageFont.truetype("arial.ttf", 9)
    except IOError:
        try:
            font_index = ImageFont.truetype("arial.ttf", 10)
            font_hex = ImageFont.load_default()
        except IOError:
            font_index = ImageFont.load_default()
            font_hex = ImageFont.load_default()

    for i, (r, g, b) in enumerate(colors):
        x0 = i * swatch_size
        y0 = 0
        x1 = x0 + swatch_size
        y1 = swatch_size

        # 1. Dessiner le carré de couleur
        draw.rectangle([x0, y0, x1, y1], fill=(r, g, b), outline=(0, 0, 0), width=1)

        # 2. Textes : Index et code Hex (#RRGGBB)
        index_text = f"[{i}]"
        hex_text = f"#{r:02X}{g:02X}{b:02X}"
        center_x = x0 + swatch_size // 2

        # 3. Positionnement en quinconce (alternance pair / impair)
        if i % 2 == 0:
            pos_y1 = swatch_size + 4
            pos_y2 = pos_y1 + 16
        else:
            pos_y1 = swatch_size + 26
            pos_y2 = pos_y1 + 16

        draw.text((center_x, pos_y1), index_text, fill=(0, 0, 0), font=font_index, anchor="mt")
        draw.text((center_x, pos_y2), hex_text, fill=(0, 0, 0), font=font_hex, anchor="mt")

    img.save(output_path, "PNG")
    print(f"Palette générée avec succès : {output_path}")

if __name__ == "__main__":
    bmp = sys.argv[1] if len(sys.argv) > 1 else "asset/asset.bmp"
    out = sys.argv[2] if len(sys.argv) > 2 else "asset/palette_extracted.png"
    extract_palette(bmp, out)
