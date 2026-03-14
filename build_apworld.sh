#!/bin/bash
# Build script for Ys Chronicles APWorld

cd "$(dirname "$0")"

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
