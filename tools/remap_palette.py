#!/usr/bin/env python3
"""
Normalise et réordonne la palette d'un fichier BMP 4-bits (16 couleurs) pour MS-DOS.

Objectifs :
1. Garantit que l'index 0 est la couleur la plus sombre (contour du curseur souris DOS).
2. Garantit que l'index 15 est la couleur la plus claire (corps du curseur souris DOS).
3. Conserve rigoureusement les données de pixels : si la palette initiale a moins de 16 couleurs,
   le fichier d'origine n'allouait pas 64 octets de palette, l'offset des pixels est donc
   automatiquement ajusté pour insérer les 16 entrées complètes sans corrompre les pixels !
4. Permet l'échange manuel d'index via l'option --swap i j.

Usage :
    python tools/remap_palette.py [input.bmp] [output.bmp]
    python tools/remap_palette.py --swap 1 15 input.bmp output.bmp
"""

import sys
import os
import struct

def read_bmp_4bit(filepath):
    with open(filepath, 'rb') as f:
        data = f.read()

    if len(data) < 54 or data[:2] != b'BM':
        raise ValueError(f"'{filepath}' n'est pas un fichier BMP Windows valide.")

    pixel_offset, = struct.unpack('<I', data[10:14])
    header_size, = struct.unpack('<I', data[14:18])
    width, height = struct.unpack('<ii', data[18:26])
    planes, bpp = struct.unpack('<HH', data[26:30])
    compression, = struct.unpack('<I', data[30:34])
    num_colors, = struct.unpack('<I', data[46:50])

    if bpp != 4:
        raise ValueError(f"Le fichier doit être en 4-bits (16 couleurs). Détecté: {bpp} bits.")
    if compression != 0:
        raise ValueError("Les BMP compressés (RLE4) ne sont pas supportés.")

    pal_start = 14 + header_size
    # Nombre réel d'entrées de palette physiquement présentes dans le fichier source
    actual_pal_entries = (pixel_offset - pal_start) // 4
    if num_colors > 0 and num_colors < actual_pal_entries:
        actual_pal_entries = num_colors

    palette = []
    for i in range(actual_pal_entries):
        p = pal_start + i * 4
        b, g, r, _ = struct.unpack('BBBB', data[p:p+4])
        palette.append((r, g, b))

    # Extraire les données brutes des pixels (sans décalage d'offset)
    stride = ((width * 4 + 31) // 32) * 4
    abs_height = abs(height)
    pixels_data = bytearray(data[pixel_offset : pixel_offset + stride * abs_height])

    return {
        'header_bytes': bytearray(data[:pal_start]),
        'width': width,
        'height': abs_height,
        'palette': palette,
        'actual_colors': len(palette),
        'pixels_data': pixels_data,
        'row_stride': stride
    }

def luminance(color):
    r, g, b = color
    return r * 0.299 + g * 0.587 + b * 0.114

def auto_fix_mapping(palette):
    """
    Détermine la table de permutation (nouvel_index -> ancien_index)
    sur le nombre exact de couleurs de la palette d'origine :
    - index 0 = couleur la plus sombre
    - dernier index = couleur la plus claire
    """
    count = len(palette)
    mapping = list(range(count))

    min_idx = min(range(count), key=lambda i: luminance(palette[i]))
    max_idx = max(range(count), key=lambda i: luminance(palette[i]))

    # 1. Échanger pour que le min soit en 0
    if min_idx != 0:
        target = min_idx
        mapping[0], mapping[target] = mapping[target], mapping[0]

    # 2. Échanger pour que le max soit au dernier index
    last_idx = count - 1
    current_max_pos = mapping.index(max_idx)
    if current_max_pos != last_idx:
        val_at_last = mapping[last_idx]
        mapping[last_idx] = max_idx
        mapping[current_max_pos] = val_at_last

    return mapping

def remap_and_save(bmp_info, new_palette, old_to_new_map, out_path):
    width = bmp_info['width']
    height = bmp_info['height']
    stride = bmp_info['row_stride']
    pixels = bytearray(bmp_info['pixels_data'])
    actual_colors = bmp_info['actual_colors']

    # 1. Réassigner chaque nibble (4-bits) de pixel vers son nouvel index
    for row in range(height):
        row_start = row * stride
        for x in range(width):
            byte_pos = row_start + (x // 2)
            b = pixels[byte_pos]
            if x % 2 == 0:
                old_nibble = (b >> 4) & 0x0F
                new_nibble = old_to_new_map[old_nibble] if old_nibble < actual_colors else old_nibble
                pixels[byte_pos] = (b & 0x0F) | (new_nibble << 4)
            else:
                old_nibble = b & 0x0F
                new_nibble = old_to_new_map[old_nibble] if old_nibble < actual_colors else old_nibble
                pixels[byte_pos] = (b & 0xF0) | (new_nibble & 0x0F)

    # 2. Construire la table de palette avec strictement le nombre de couleurs d'origine
    palette_bytes = bytearray()
    for r, g, b in new_palette:
        palette_bytes.extend([b, g, r, 0])

    # 3. Assembler le header avec les offsets recalculés (taille 100% préservée)
    header = bytearray(bmp_info['header_bytes'])
    new_pixel_offset = len(header) + len(palette_bytes)
    new_file_size = new_pixel_offset + len(pixels)

    struct.pack_into('<I', header, 2, new_file_size)     # bfSize
    struct.pack_into('<I', header, 10, new_pixel_offset)  # bfOffBits
    struct.pack_into('<I', header, 46, actual_colors)     # biClrUsed exact

    with open(out_path, 'wb') as f:
        f.write(header)
        f.write(palette_bytes)
        f.write(pixels)

def main():
    args = sys.argv[1:]

    swap_mode = False
    swap_pair = None
    if len(args) >= 3 and args[0] == '--swap':
        swap_mode = True
        swap_pair = (int(args[1]), int(args[2]))
        args = args[3:]

    in_file = args[0] if len(args) > 0 else "asset/asset.bmp"
    out_file = args[1] if len(args) > 1 else in_file

    print(f"Chargement de '{in_file}'...")
    bmp = read_bmp_4bit(in_file)
    old_pal = bmp['palette']
    count = bmp['actual_colors']
    print(f"Nombre de couleurs détectées : {count}")

    print("Palette d'origine :")
    for i, c in enumerate(old_pal):
        lum = luminance(c)
        print(f"  [{i:2d}] R={c[0]:3d}, G={c[1]:3d}, B={c[2]:3d}  (Lum={lum:5.1f})")

    if swap_mode:
        idx1, idx2 = swap_pair
        print(f"\nPermutation manuelle demandée : index {idx1} <-> index {idx2}")
        old_to_new = list(range(count))
        old_to_new[idx1] = idx2
        old_to_new[idx2] = idx1

        new_pal = list(old_pal)
        new_pal[idx1], new_pal[idx2] = new_pal[idx2], new_pal[idx1]
    else:
        print(f"\nNormalisation automatique (Index 0 = plus sombre, Index {count-1} = plus clair)...")
        new_to_old = auto_fix_mapping(old_pal)

        old_to_new = [0] * count
        for new_idx, old_idx in enumerate(new_to_old):
            old_to_new[old_idx] = new_idx

        new_pal = [old_pal[new_to_old[i]] for i in range(count)]

    print("\nNouvelle palette :")
    for i, c in enumerate(new_pal):
        lum = luminance(c)
        tag = ""
        if i == 0: tag = " <-- Plus sombre"
        if i == count - 1: tag = " <-- Plus clair"
        print(f"  [{i:2d}] R={c[0]:3d}, G={c[1]:3d}, B={c[2]:3d}  (Lum={lum:5.1f}){tag}")

    remap_and_save(bmp, new_pal, old_to_new, out_file)
    print(f"\nFichier réassigné et sauvegardé avec succès dans : '{out_file}' ({os.path.getsize(out_file)} octets)")

if __name__ == '__main__':
    main()
