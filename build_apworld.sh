#!/bin/bash
# Build script for Ys Chronicles AP mod

cd "$(dirname "$0")"

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
rm -f ys_chronicles.apworld player/ys_chronicles.apworld

# Package from apworld directory
cd apworld
zip -r ../player/ys_chronicles.apworld ys_chronicles -x "*.pyc" -x "*/__pycache__/*" -x "*.DS_Store"
cp ../player/ys_chronicles.apworld ../ys_chronicles.apworld

echo ""
echo "Built: player/ys_chronicles.apworld"
echo ""
echo "To install, copy to:"
echo "  ~/Library/Application Support/Archipelago/worlds/"
