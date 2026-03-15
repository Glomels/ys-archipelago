#!/bin/bash
# Test: watch Bestiary Potion chest, give Timer Ring instead

WINE_PATH="/Users/luis/Applications/CrossOver.app/Contents/SharedSupport/CrossOver/lib/wine/x86_64-unix/wine"
WINEPREFIX="$HOME/Library/Application Support/CrossOver/Bottles/Steam"
export WINEDEBUG="-all" WINEMSYNC=1 WINEPREFIX="$WINEPREFIX"

CHEST_FLAG=0x531894
GIVE_ID=15
# item 15 = Timer Ring
GIVE_ADDR1=0x5319CC
GIVE_ADDR2=0x5320B8

# Clean up item 15 from prior test
"$WINE_PATH" "C:\\mem_write.exe" "$GIVE_ADDR1" 0 2>/dev/null >/dev/null
"$WINE_PATH" "C:\\mem_write.exe" "$GIVE_ADDR2" 0 2>/dev/null >/dev/null

echo "Watching chest at $CHEST_FLAG (Bestiary Potion)"
echo "Will give item $GIVE_ID (Timer Ring) when opened"
echo "Go open the chest!"
echo ""

while true; do
    output=$("$WINE_PATH" "C:\\mem_read.exe" "$CHEST_FLAG" 2>/dev/null)
    val=$(echo "$output" | awk -F': ' 'NF==2{print $2}')
    if [ "$val" = "1" ]; then
        echo ">>> Chest opened!"
        echo "DLL will suppress vanilla Bestiary Potion..."

        # Permit and give the replacement
        echo "$GIVE_ID" >> "$WINEPREFIX/drive_c/ap_items.txt"
        "$WINE_PATH" "C:\\mem_write.exe" "$GIVE_ADDR1" 1 2>/dev/null
        "$WINE_PATH" "C:\\mem_write.exe" "$GIVE_ADDR2" 1 2>/dev/null
        echo ">>> Gave Timer Ring!"

        echo ""
        echo "=== itemhook.log (last 5 lines) ==="
        tail -5 "$WINEPREFIX/drive_c/itemhook.log"
        break
    fi
    sleep 0.3
done
