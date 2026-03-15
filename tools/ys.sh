#!/bin/bash
# Ys I Chronicles - Memory tools launcher
# Runs Windows .exe helpers inside CrossOver Wine

export WINEPREFIX="/Users/luis/Library/Application Support/CrossOver/Bottles/Steam"
export WINEDEBUG=-all
export WINEMSYNC=1
WINE="/Users/luis/Applications/CrossOver.app/Contents/SharedSupport/CrossOver/lib/wine/x86_64-unix/wine"
DRIVE_C="$WINEPREFIX/drive_c"

# Item name lookup
item_name() {
    case $1 in
        0) echo "Short Sword" ;; 1) echo "Long Sword" ;; 2) echo "Talwar" ;;
        3) echo "Silver Sword" ;; 4) echo "Flame Sword" ;;
        5) echo "Small Shield" ;; 6) echo "Middle Shield" ;; 7) echo "Large Shield" ;;
        8) echo "Silver Shield" ;; 9) echo "Battle Shield" ;;
        10) echo "Chain Mail" ;; 11) echo "Plate Mail" ;; 12) echo "Reflex" ;;
        13) echo "Silver Armor" ;; 14) echo "Battle Armor" ;;
        15) echo "Power Ring" ;; 16) echo "Shield Ring" ;; 17) echo "Timer Ring" ;;
        18) echo "Heal Ring" ;; 19) echo "Evil Ring" ;;
        20) echo "Volume Hadal" ;; 21) echo "Volume Tovah" ;; 22) echo "Volume Dabbie" ;;
        23) echo "Volume Mesa" ;; 24) echo "Volume Gemma" ;; 25) echo "Volume Fact" ;;
        26) echo "Treasure Box Key" ;; 27) echo "Prison Key" ;; 28) echo "Shrine Key" ;;
        29) echo "Ivory Key" ;; 30) echo "Marble Key" ;; 31) echo "Darm Key" ;;
        32) echo "Sara's Crystal" ;; 33) echo "Roda Tree Seed" ;; 34) echo "Silver Bell" ;;
        35) echo "Silver Harmonica" ;; 36) echo "Idol" ;; 37) echo "Rod" ;;
        38) echo "Monocle" ;; 39) echo "Blue Amulet" ;; 40) echo "Ruby" ;;
        41) echo "Sapphire Ring" ;; 42) echo "Necklace" ;; 43) echo "Golden Vase" ;;
        44) echo "Heal Potion" ;; 45) echo "Wing" ;; 46) echo "Hammer" ;;
        47) echo "Mirror" ;; 48) echo "Mask of Eyes" ;; 49) echo "Blue Necklace" ;;
        50) echo "Bestiary Potion" ;; 51) echo "Piece of Paper" ;;
        *) echo "Item $1" ;;
    esac
}

# Item ID lookup by name (case-insensitive partial match)
find_item_id() {
    local search=$(echo "$1" | tr '[:upper:]' '[:lower:]')
    for i in $(seq 0 51); do
        local name=$(item_name $i | tr '[:upper:]' '[:lower:]')
        if [[ "$name" == *"$search"* ]]; then
            echo $i
            return
        fi
    done
    echo ""
}

usage() {
    echo "Ys I Chronicles Memory Tools"
    echo ""
    echo "Stats:"
    echo "  $0 stats                    Show current stats"
    echo "  $0 set hp <val>             Set HP"
    echo "  $0 set maxhp <val>          Set Max HP"
    echo "  $0 set str <val>            Set STR"
    echo "  $0 set def <val>            Set DEF"
    echo "  $0 set gold <val>           Set Gold"
    echo "  $0 set level <val>          Set Level"
    echo ""
    echo "Items:"
    echo "  $0 items                    List owned items"
    echo "  $0 give <id|name>           Give item by ID or name"
    echo "  $0 remove <id|name>         Remove item by ID or name"
    echo "  $0 give-all                 Give all items"
    echo ""
    echo "Monitoring:"
    echo "  $0 monitor                  Start memory change monitor"
    echo "  $0 snapshot save [file]     Save memory snapshot"
    echo "  $0 snapshot diff [file]     Diff against snapshot"
    echo ""
    echo "Raw:"
    echo "  $0 read <addr_hex>          Read DWORD at address"
    echo "  $0 write <addr_hex> <val>   Write DWORD at address"
    echo ""
    echo "Build:"
    echo "  $0 build                    Recompile all .exe tools"
}

build_all() {
    echo "Building all tools..."
    cd /Users/luis/ys-archipelago/tools
    for src in mem_write.c mem_monitor.c mem_snapshot.c mem_items2.c; do
        exe="${src%.c}.exe"
        echo "  $src -> $exe"
        i686-w64-mingw32-gcc -o "$exe" "$src" -lpsapi 2>&1
        cp "$exe" "$DRIVE_C/$exe"
    done
    echo "Done."
}

read_addr() {
    "$WINE" "C:\\mem_write.exe" "$1" 0 2>&1 | head -1 | sed 's/ -> 0//' | sed 's/Verified:.*//'
    # Restore value - read-only hack using write with same value
}

case "${1:-}" in
    stats)
        echo "=== Player Stats ==="
        for field in hp maxhp str def gold level; do
            result=$("$WINE" "C:\\mem_write.exe" $field 0 2>&1 | head -1)
            val=$(echo "$result" | grep -o ': [0-9]*' | head -1 | tr -d ': ')
            # Restore original value
            "$WINE" "C:\\mem_write.exe" $field "$val" 2>&1 > /dev/null
            printf "  %-8s %s\n" "$field:" "$val"
        done
        ;;

    set)
        if [ -z "${2:-}" ] || [ -z "${3:-}" ]; then
            echo "Usage: $0 set <hp|maxhp|str|def|gold|level> <value>"
            exit 1
        fi
        "$WINE" "C:\\mem_write.exe" "$2" "$3" 2>&1
        ;;

    items)
        echo "=== Owned Items ==="
        for i in $(seq 0 51); do
            addr1=$(printf '%X' $((0x531990 + i * 4)))
            result=$("$WINE" "C:\\mem_write.exe" "$addr1" 0 2>&1 | head -1)
            val=$(echo "$result" | grep -o ': [0-9]*' | head -1 | tr -d ': ')
            "$WINE" "C:\\mem_write.exe" "$addr1" "$val" 2>&1 > /dev/null
            if [ "$val" = "1" ]; then
                printf "  [%2d] %s\n" "$i" "$(item_name $i)"
            fi
        done
        ;;

    give)
        if [ -z "${2:-}" ]; then
            echo "Usage: $0 give <item_id or item_name>"
            exit 1
        fi
        id="$2"
        if ! [[ "$id" =~ ^[0-9]+$ ]]; then
            id=$(find_item_id "$2")
            if [ -z "$id" ]; then
                echo "Item not found: $2"
                exit 1
            fi
        fi
        name=$(item_name "$id")
        addr1=$(printf '%X' $((0x531990 + id * 4)))
        addr2=$(printf '%X' $((0x53207C + id * 4)))
        "$WINE" "C:\\mem_write.exe" "$addr1" 1 2>&1
        "$WINE" "C:\\mem_write.exe" "$addr2" 1 2>&1
        echo "Gave: [$id] $name"
        ;;

    remove)
        if [ -z "${2:-}" ]; then
            echo "Usage: $0 remove <item_id or item_name>"
            exit 1
        fi
        id="$2"
        if ! [[ "$id" =~ ^[0-9]+$ ]]; then
            id=$(find_item_id "$2")
            if [ -z "$id" ]; then
                echo "Item not found: $2"
                exit 1
            fi
        fi
        name=$(item_name "$id")
        addr1=$(printf '%X' $((0x531990 + id * 4)))
        addr2=$(printf '%X' $((0x53207C + id * 4)))
        "$WINE" "C:\\mem_write.exe" "$addr1" 0 2>&1
        "$WINE" "C:\\mem_write.exe" "$addr2" 0 2>&1
        echo "Removed: [$id] $name"
        ;;

    ap-give)
        # Give items through the DLL's ap_give (sets permitted flag)
        GAME_DIR="$HOME/Library/Application Support/CrossOver/Bottles/Steam/drive_c/Program Files (x86)/Steam/steamapps/common/Ys I"
        GIVE_FILE="$GAME_DIR/ap_give.txt"
        shift
        if [ $# -eq 0 ]; then
            echo "Usage: $0 ap-give <id> [id] [id] ..."
            exit 1
        fi
        for id in "$@"; do
            echo "$id" >> "$GIVE_FILE"
            echo "Queued: [$id] $(item_name $id)"
        done
        ;;

    give-all)
        echo "Giving all items..."
        for i in $(seq 0 51); do
            addr1=$(printf '%X' $((0x531990 + i * 4)))
            addr2=$(printf '%X' $((0x53207C + i * 4)))
            "$WINE" "C:\\mem_write.exe" "$addr1" 1 2>&1 > /dev/null
            "$WINE" "C:\\mem_write.exe" "$addr2" 1 2>&1 > /dev/null
            printf "  [%2d] %s\n" "$i" "$(item_name $i)"
        done
        echo "Done."
        ;;

    monitor)
        echo "Starting memory monitor (Ctrl+C to stop)..."
        "$WINE" "C:\\mem_monitor.exe" 2>&1
        ;;

    snapshot)
        if [ -z "${2:-}" ]; then
            echo "Usage: $0 snapshot <save|diff> [filename]"
            exit 1
        fi
        "$WINE" "C:\\mem_full_snapshot.exe" "$2" ${3:+"C:\\$3"} 2>&1
        ;;

    read)
        if [ -z "${2:-}" ]; then
            echo "Usage: $0 read <address_hex>"
            exit 1
        fi
        result=$("$WINE" "C:\\mem_write.exe" "$2" 0 2>&1 | head -1)
        val=$(echo "$result" | grep -o ': [0-9]*' | head -1 | tr -d ': ')
        "$WINE" "C:\\mem_write.exe" "$2" "$val" 2>&1 > /dev/null
        echo "0x$2: $val"
        ;;

    write)
        if [ -z "${2:-}" ] || [ -z "${3:-}" ]; then
            echo "Usage: $0 write <address_hex> <value>"
            exit 1
        fi
        "$WINE" "C:\\mem_write.exe" "$2" "$3" 2>&1
        ;;

    build)
        build_all
        ;;

    *)
        usage
        ;;
esac
