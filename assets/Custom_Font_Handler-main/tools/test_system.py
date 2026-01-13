#!/usr/bin/env python3
"""
Test script - Verifies the font generator is working correctly
"""

import os
import sys
from pathlib import Path

print("=" * 70)
print("E-Paper Font Generator - System Test")
print("=" * 70)
print()

errors = []
warnings = []

# Test 1: Check Python version
print("[1/7] Checking Python version...")
if sys.version_info < (3, 7):
    errors.append("Python 3.7+ required")
    print("  ✗ Python version too old")
else:
    print(f"  ✓ Python {sys.version_info.major}.{sys.version_info.minor}")

# Test 2: Check PIL/Pillow
print("[2/7] Checking Pillow...")
try:
    from PIL import Image, ImageDraw, ImageFont
    print("  ✓ Pillow installed")
except ImportError:
    errors.append("Pillow not installed")
    print("  ✗ Pillow not found")
    print("     Run: pip install Pillow")

# Test 3: Check directory structure
print("[3/7] Checking directory structure...")
required_dirs = ['input', 'output', 'examples']
for d in required_dirs:
    if not os.path.exists(d):
        warnings.append(f"Directory missing: {d}")
        print(f"  ⚠ {d}/ not found (will be created)")
    else:
        print(f"  ✓ {d}/ exists")

# Test 4: Check required files
print("[4/7] Checking required files...")
required_files = ['tools/generate_all.py', 'README.md', 'requirements.txt', 'LICENSE']
for f in required_files:
    if not os.path.exists(f):
        errors.append(f"Missing file: {f}")
        print(f"  ✗ {f} not found")
    else:
        print(f"  ✓ {f} exists")

# Test 5: Check for fonts in input/
print("[5/7] Checking input fonts...")
if os.path.exists('input'):
    fonts = list(Path('input').glob('*.ttf')) + list(Path('input').glob('*.otf'))
    if fonts:
        print(f"  ✓ Found {len(fonts)} font(s)")
        for f in fonts:
            print(f"     - {f.name}")
    else:
        warnings.append("No fonts in input/")
        print("  ⚠ No fonts found in input/")
        print("     Place TTF/OTF files there to generate")
else:
    warnings.append("input/ directory missing")
    print("  ⚠ input/ directory not found")

# Test 6: Test font loading
print("[6/7] Testing font loading...")
if os.path.exists('input'):
    fonts = list(Path('input').glob('*.ttf'))
    if fonts:
        try:
            from PIL import ImageFont
            font = ImageFont.truetype(str(fonts[0]), 16)
            print(f"  ✓ Successfully loaded {fonts[0].name}")
        except Exception as e:
            errors.append(f"Font loading failed: {e}")
            print(f"  ✗ Failed to load font: {e}")
    else:
        print("  ⊘ Skipped (no fonts to test)")
else:
    print("  ⊘ Skipped (input/ not found)")

# Test 7: Check write permissions
print("[7/7] Checking write permissions...")
try:
    os.makedirs('output', exist_ok=True)
    test_file = 'output/.test'
    with open(test_file, 'w') as f:
        f.write('test')
    os.remove(test_file)
    print("  ✓ Can write to output/")
except Exception as e:
    errors.append(f"Write permission error: {e}")
    print(f"  ✗ Cannot write to output/: {e}")

# Summary
print()
print("=" * 70)
print("Test Summary")
print("=" * 70)
print()

if errors:
    print(f"❌ {len(errors)} ERROR(S):")
    for e in errors:
        print(f"   - {e}")
    print()
    
if warnings:
    print(f"⚠️  {len(warnings)} WARNING(S):")
    for w in warnings:
        print(f"   - {w}")
    print()

if not errors:
    print("✅ All tests passed!")
    print()
    print("System is ready to generate fonts.")
    print()
    print("Next steps:")
    print("  1. Place TTF/OTF files in input/")
    print("  2. Run: python generate_all.py")
    print()
else:
    print("❌ Please fix errors before generating fonts")
    print()
    sys.exit(1)
