#!/usr/bin/env python3
"""
Convertit un fichier BMP 4-bits en tableau d'octets C++ (assetdef.h / assetdef.cpp)
pour intégration directe dans l'exécutable sous Borland C++ 3.1 (MS-DOS).

Usage :
    python bin2c.py [chemin_du_bmp]

Par défaut :
    Lit "asset/asset.bmp" et génère "src/assetdef.h" et "src/assetdef.cpp".
"""

import sys
import os

def generate_asset_sources(bmp_path="asset/asset.bmp",
                           out_header="src/assetdef.h",
                           out_source="src/assetdef.cpp"):
    if not os.path.exists(bmp_path):
        print(f"Erreur : fichier '{bmp_path}' introuvable.")
        sys.exit(1)

    with open(bmp_path, "rb") as f:
        data = f.read()

    total_bytes = len(data)
    print(f"Lecture de '{bmp_path}' ({total_bytes} octets)...")

    # 1. Génération du header (.h)
    with open(out_header, "w", encoding="cp850") as f:
        f.write("/* Fichier généré automatiquement par bin2c.py - NE PAS MODIFIER */\n")
        f.write("#ifndef ASSETDEF_H\n")
        f.write("#define ASSETDEF_H\n\n")
        f.write(f"/* Sprite sheet asset.bmp embarqué par défaut ({total_bytes} octets) */\n")
        f.write(f"extern const unsigned char far default_asset_bmp[{total_bytes}];\n\n")
        f.write("#endif\n")

    # 2. Génération du source C++ (.cpp)
    with open(out_source, "w", encoding="cp850") as f:
        f.write("/* Fichier généré automatiquement par bin2c.py - NE PAS MODIFIER */\n")
        f.write('#include "assetdef.h"\n\n')
        f.write(f"const unsigned char far default_asset_bmp[{total_bytes}] = {{\n")

        # 16 octets par ligne pour un rendu lisible
        for i in range(0, total_bytes, 16):
            chunk = data[i : i + 16]
            hex_values = ", ".join(f"0x{byte:02X}" for byte in chunk)
            is_last = (i + 16 >= total_bytes)
            comma = "" if is_last else ","
            f.write(f"    {hex_values}{comma}\n")

        f.write("};\n")

    print(f"Génération terminée avec succès :")
    print(f"  - {out_header}")
    print(f"  - {out_source}")

if __name__ == "__main__":
    bmp = sys.argv[1] if len(sys.argv) > 1 else "asset/asset.bmp"
    generate_asset_sources(bmp)
