#!/bin/bash
#
# Setup Ys I Chronicles AP mod
#
# Backs up original steam_api.dll, compiles and deploys:
#   - steam_api.dll (proxy that loads aphook.dll)
#   - aphook.dll    (full AP client)
#
# Usage: ./setup_mod.sh [server] [slot] [password]
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
GAME_DIR="$HOME/Library/Application Support/CrossOver/Bottles/Steam/drive_c/Program Files (x86)/Steam/steamapps/common/Ys I"

SERVER="${1:-localhost:38281}"
SLOT="${2:-Adol}"
PASSWORD="${3:-}"

CC=i686-w64-mingw32-gcc

echo "=== Ys I Chronicles - AP Mod Setup ==="
echo

# Check game directory exists
if [ ! -f "$GAME_DIR/ys1plus.exe" ]; then
    echo "ERROR: ys1plus.exe not found at:"
    echo "  $GAME_DIR"
    exit 1
fi

# Check compiler
if ! command -v $CC &>/dev/null; then
    echo "ERROR: $CC not found. Install with:"
    echo "  brew install mingw-w64"
    exit 1
fi

# Step 1: Compile DLLs
echo "[1/4] Compiling aphook.dll..."
$CC -shared -O2 -o "$SCRIPT_DIR/player/aphook.dll" "$SCRIPT_DIR/src/aphook.c" -lws2_32 -lkernel32
echo "  OK"

echo "[1/4] Compiling steam_api.dll (proxy)..."
$CC -shared -O2 -o "$SCRIPT_DIR/player/steam_api.dll" \
    "$SCRIPT_DIR/src/steam_api_proxy.c" "$SCRIPT_DIR/src/steam_api_proxy.def" -lkernel32
echo "  OK"

# Step 2: Backup original steam_api.dll
if [ -f "$GAME_DIR/steam_api.dll" ] && [ ! -f "$GAME_DIR/steam_api_orig.dll" ]; then
    echo "[2/4] Backing up original steam_api.dll → steam_api_orig.dll"
    cp "$GAME_DIR/steam_api.dll" "$GAME_DIR/steam_api_orig.dll"
    echo "  OK"
elif [ -f "$GAME_DIR/steam_api_orig.dll" ]; then
    echo "[2/4] Backup already exists (steam_api_orig.dll)"
else
    echo "ERROR: steam_api.dll not found in game directory"
    exit 1
fi

# Step 3: Deploy DLLs
echo "[3/4] Deploying to game directory..."
cp "$SCRIPT_DIR/player/steam_api.dll" "$GAME_DIR/steam_api.dll"
cp "$SCRIPT_DIR/player/aphook.dll" "$GAME_DIR/aphook.dll"
echo "  steam_api.dll (proxy) → installed"
echo "  aphook.dll            → installed"

# Step 4: Write config
CONFIG_PATH="$GAME_DIR/ap_connect.txt"
echo "[4/4] Writing config..."
cat > "$CONFIG_PATH" <<EOF
server=$SERVER
slot=$SLOT
password=$PASSWORD
EOF
echo "  server=$SERVER"
echo "  slot=$SLOT"

echo
echo "=== Setup complete ==="
echo
echo "Just launch the game normally through Steam/CrossOver."
echo "aphook.dll will connect to the AP server automatically."
echo
echo "Log file: $GAME_DIR/aphook.log"
echo
echo "To uninstall:"
echo "  cp \"$GAME_DIR/steam_api_orig.dll\" \"$GAME_DIR/steam_api.dll\""
echo "  rm \"$GAME_DIR/aphook.dll\" \"$GAME_DIR/ap_connect.txt\""
