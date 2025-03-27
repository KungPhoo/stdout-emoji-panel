import unicodedata
import os
output_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), "unicode_full.inc")

with open(output_file, "w", encoding="utf-8") as f:
    for cp in range(0x110000):
        name = unicodedata.name(chr(cp), None)
        if name:
            f.write(f"{{0x{cp:X}, \"{name}\"}},\n")

