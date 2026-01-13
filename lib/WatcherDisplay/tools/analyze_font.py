#!/usr/bin/env python3
"""
Universal font analyzer - works with any TTF/OTF font
Usage: python analyze_font.py <font_file.ttf>
"""

import sys
from PIL import Image, ImageDraw, ImageFont

if len(sys.argv) != 2:
    print("Usage: python analyze_font.py <font_file.ttf>")
    sys.exit(1)

font_path = sys.argv[1]

print(f"Analyzing {font_path}...")
print("=" * 70)
print()

# Test multiple sizes
test_sizes = [8, 10, 12, 14, 16, 18, 20, 22, 24, 28, 32, 36, 40, 48]

for size in test_sizes:
    try:
        font = ImageFont.truetype(font_path, size)
    except Exception as e:
        print(f"Size {size}px: ERROR - {e}")
        continue
    
    test_img = Image.new('1', (200, 200), 1)
    draw = ImageDraw.Draw(test_img)
    
    # Measure all printable ASCII
    all_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()gjpqy"
    
    # Get width (check if monospace)
    bbox_m = draw.textbbox((0, 0), 'M', font=font)
    width_m = bbox_m[2] - bbox_m[0]
    bbox_i = draw.textbbox((0, 0), 'i', font=font)
    width_i = bbox_i[2] - bbox_i[0]
    
    is_monospace = abs(width_m - width_i) < 2
    
    # Get height bounds
    min_y = 999
    max_y = -999
    
    for char in all_chars:
        try:
            bbox = draw.textbbox((0, 0), char, font=font)
            min_y = min(min_y, bbox[1])
            max_y = max(max_y, bbox[3])
        except:
            continue
    
    natural_height = max_y - min_y
    
    # Recommendation
    mono_str = "MONO" if is_monospace else "PROP"
    
    print(f"Size {size:2d}px: natural_height={natural_height:2d}px, width={width_m:2d}px [{mono_str}]", end="")
    
    # Suggest target heights
    if natural_height <= size:
        print(f" → Use target={size}px (fits perfectly!)")
    elif natural_height <= size + 2:
        print(f" → Use target={size+2}px (slight padding)")
    elif natural_height <= size + 4:
        print(f" → Use target={size+4}px (comfortable)")
    else:
        print(f" → Use target={natural_height}px (tight fit)")

print()
print("=" * 70)
print("RECOMMENDATIONS:")
print()
print("For UI labels (small):")
sizes_small = [s for s in test_sizes if 12 <= s <= 18]
print(f"  Try sizes: {', '.join(map(str, sizes_small))}")
print()
print("For body text (medium):")
sizes_medium = [s for s in test_sizes if 16 <= s <= 24]
print(f"  Try sizes: {', '.join(map(str, sizes_medium))}")
print()
print("For headers (large):")
sizes_large = [s for s in test_sizes if 24 <= s <= 48]
print(f"  Try sizes: {', '.join(map(str, sizes_large))}")
print()
print("MONOSPACE CHECK:")
print("  If font shows [MONO], it's good for UI elements, code, tables")
print("  If font shows [PROP], it's better for paragraphs, body text")
