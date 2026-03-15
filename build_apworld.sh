#!/bin/bash
# Build script for Ys Chronicles APWorld

cd "$(dirname "$0")"

# Cross-compile itemhook DLL (requires mingw-w64)
echo "Building itemhook.dll..."
i686-w64-mingw32-gcc -shared -O2 -o tools/itemhook.dll tools/itemhook.c -lkernel32
if [ $? -eq 0 ]; then
    echo "  Built: tools/itemhook.dll"
else
    echo "  WARNING: itemhook.dll build failed (need: brew install mingw-w64)"
fi

# Remove old apworld
rm -f ys_chronicles.apworld

# Package from apworld directory
cd apworld
zip -r ../ys_chronicles.apworld ys_chronicles -x "*.pyc" -x "*/__pycache__/*" -x "*.DS_Store"

echo ""
echo "Built: ys_chronicles.apworld"
echo ""
echo "To install, copy to:"
echo "  ~/Library/Application Support/Archipelago/worlds/"
