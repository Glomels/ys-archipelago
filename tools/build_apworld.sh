#!/bin/bash
# Build script for Ys Chronicles AP mod

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$ROOT_DIR"

CC=i686-w64-mingw32-gcc

# Cross-compile DLLs (requires mingw-w64)
echo "Building aphook.dll..."
$CC -shared -O2 -o player/aphook.dll src/aphook.c -lws2_32 -lkernel32
if [ $? -eq 0 ]; then
    echo "  Built: player/aphook.dll"
else
    echo "  WARNING: aphook.dll build failed (need: brew install mingw-w64)"
fi

echo "Building steam_api.dll (proxy)..."
$CC -shared -O2 -o player/steam_api.dll src/steam_api_proxy.c src/steam_api_proxy.def -lkernel32
if [ $? -eq 0 ]; then
    echo "  Built: player/steam_api.dll"
else
    echo "  WARNING: steam_api.dll build failed"
fi

# Remove old apworld
rm -f player/ys_chronicles.apworld

# Package world files from root into ys_chronicles/ inside the zip
mkdir -p /tmp/ys_apworld/ys_chronicles
cp "$ROOT_DIR"/__init__.py "$ROOT_DIR"/items.py "$ROOT_DIR"/locations.py \
   "$ROOT_DIR"/regions.py "$ROOT_DIR"/options.py "$ROOT_DIR"/client.py \
   "$ROOT_DIR"/archipelago.json \
   "$ROOT_DIR/en_Ys I Chronicles.md" \
   "$ROOT_DIR/en_setup_Ys I Chronicles.md" \
   /tmp/ys_apworld/ys_chronicles/

cd /tmp/ys_apworld
zip -r "$ROOT_DIR/player/ys_chronicles.apworld" ys_chronicles -x "*.pyc" -x "*/__pycache__/*" -x "*.DS_Store"
rm -rf /tmp/ys_apworld

echo ""
echo "Built: player/ys_chronicles.apworld"
echo ""
echo "To install, copy to:"
echo "  ~/Library/Application Support/Archipelago/worlds/"
