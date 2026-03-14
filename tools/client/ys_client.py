#!/usr/bin/env python3
"""
Ys I & II Chronicles Archipelago Client

This client connects to PPSSPP and the Archipelago server to enable
multiworld randomizer functionality.
"""

import asyncio
import base64
import hashlib
import json
import struct
import subprocess
from typing import Optional, Dict, Set, List, Tuple

try:
    import websockets
except ImportError:
    print("ERROR: websockets library required. Install with: pip install websockets")
    raise

# Archipelago imports (available when running in AP context)
try:
    from CommonClient import CommonContext, server_loop, get_base_parser
    ARCHIPELAGO_AVAILABLE = True
except ImportError:
    ARCHIPELAGO_AVAILABLE = False


# =============================================================================
# Constants
# =============================================================================

PSP_RAM_BASE = 0x08800000

# Default PPSSPP port (can be auto-detected)
DEFAULT_PPSSPP_PORT = 54453


# =============================================================================
# Memory Addresses
# =============================================================================

class Ys1Addresses:
    """Memory addresses for Ys I (offsets from PSP_RAM_BASE)."""

    # Player stats
    CURRENT_HP = 0x0175C20
    MAX_HP = 0x0175C24
    ATTACK = 0x0175C28
    DEFENSE = 0x0175C2C
    GOLD = 0x0175C30

    # Movement
    MOVE_SPEED = 0x0074510

    # Equipment slots (currently equipped)
    EQUIP_WEAPON = 0x0175A98
    EQUIP_ARMOR = 0x0175A9C
    EQUIP_SHIELD = 0x0175AA0
    EQUIP_RING = 0x0175AA4
    EQUIP_CONSUMABLE = 0x0175AA8

    # Item arrays - BOTH must be set for items to be usable
    ITEM_OWNERSHIP = 0x0175DB4   # Makes item selectable/equippable
    ITEM_DISCOVERED = 0x01764A0  # Shows item in inventory list

    # Chest flags array — each chest is a 4-byte flag (0=closed, 1=opened)
    CHEST_FLAGS = 0x0175CB8

    # Boss defeated flags (placeholder — needs memory research)
    # Expected to be near story flags at 0x0177xxx
    # TODO: Discover via memory_diff.py before/after each boss fight
    BOSS_FLAGS_BASE = 0x0177B00  # Placeholder — not verified

    # Story progression flags
    STORY_FLAGS = 0x0177AC8

    # Ring effect flags
    RING_EFFECT_HEAL = 0x0175EBC

    # Legacy aliases
    HP = MAX_HP
    ITEM_POSSESSION = ITEM_DISCOVERED  # Backwards compat

    @classmethod
    def absolute(cls, offset: int) -> int:
        """Convert offset to absolute PSP address."""
        return PSP_RAM_BASE + offset

    @classmethod
    def item_ownership_addr(cls, item_id: int) -> int:
        """Get absolute address for item ownership flag (makes selectable)."""
        return PSP_RAM_BASE + cls.ITEM_OWNERSHIP + (item_id * 4)

    @classmethod
    def item_discovered_addr(cls, item_id: int) -> int:
        """Get absolute address for item discovered flag (shows in list)."""
        return PSP_RAM_BASE + cls.ITEM_DISCOVERED + (item_id * 4)

    @classmethod
    def chest_flag_addr(cls, chest_id: int) -> int:
        """Get absolute address for a chest flag."""
        return PSP_RAM_BASE + cls.CHEST_FLAGS + (chest_id * 4)

    @classmethod
    def get_equip_slot(cls, item_id: int) -> Optional[int]:
        """Get equipment slot address for an item, or None if not equippable."""
        if 0 <= item_id <= 4:  # Weapons
            return cls.EQUIP_WEAPON
        elif 5 <= item_id <= 9:  # Shields
            return cls.EQUIP_SHIELD
        elif 10 <= item_id <= 14:  # Armor
            return cls.EQUIP_ARMOR
        elif 15 <= item_id <= 19:  # Rings
            return cls.EQUIP_RING
        return None


class Ys2Addresses:
    """Memory addresses for Ys II (offsets from PSP_RAM_BASE)."""

    # Player stats
    HP = 0x027809C
    MP = 0x02780A4
    ATTACK = 0x02780A8
    DEFENSE = 0x02780AC
    GOLD = 0x02780B4

    # Magic/Rings
    MAGIC_SPELLS = 0x06C8800
    RINGS = 0x06C8804

    # Items blocks (need further mapping)
    ITEMS_BLOCK = 0x027757C
    ITEMS_BLOCK_ALT = 0x02775C4

    @classmethod
    def absolute(cls, offset: int) -> int:
        """Convert offset to absolute PSP address."""
        return PSP_RAM_BASE + offset


# =============================================================================
# PPSSPP Connection
# =============================================================================

class PPSSPPInterface:
    """
    WebSocket interface for communicating with PPSSPP's debugger.
    """

    def __init__(self, host: str = "localhost", port: int = DEFAULT_PPSSPP_PORT):
        self.host = host
        self.port = port
        self.ws: Optional[websockets.WebSocketClientProtocol] = None
        self.ticket_counter = 0
        self.connected = False
        self._request_lock = asyncio.Lock()  # Prevent concurrent requests

    @property
    def uri(self) -> str:
        return f"ws://{self.host}:{self.port}/debugger"

    async def connect(self) -> bool:
        """Connect to PPSSPP's WebSocket debugger."""
        try:
            self.ws = await websockets.connect(
                self.uri,
                subprotocols=["debugger.ppsspp.org"]
            )
            self.connected = True
            print(f"Connected to PPSSPP at {self.uri}")
            return True
        except ConnectionRefusedError:
            print(f"Connection refused at {self.uri}")
            return False
        except Exception as e:
            print(f"Connection failed: {e}")
            return False

    async def disconnect(self):
        """Close the connection."""
        if self.ws:
            await self.ws.close()
            self.ws = None
            self.connected = False

    def _next_ticket(self) -> str:
        """Generate unique ticket ID."""
        self.ticket_counter += 1
        return f"ys-{self.ticket_counter}"

    async def _recv_response(self, ticket: str) -> dict:
        """Receive response matching the given ticket ID."""
        while True:
            response_text = await self.ws.recv()
            response = json.loads(response_text)
            if response.get("ticket") == ticket:
                return response

    async def send_request(self, event: str, **params) -> dict:
        """Send request and wait for response with timeout and locking."""
        if not self.ws:
            raise RuntimeError("Not connected to PPSSPP")

        async with self._request_lock:  # Serialize requests to prevent overwhelming PPSSPP
            try:
                ticket = self._next_ticket()
                request = {"event": event, "ticket": ticket, **params}

                await self.ws.send(json.dumps(request))

                # Add timeout to prevent hanging
                response = await asyncio.wait_for(self._recv_response(ticket), timeout=2.0)
                return response
            except asyncio.TimeoutError:
                print(f"Request timeout: {event}")
                return {"event": "error", "message": "timeout"}
            except Exception as e:
                print(f"Request error: {e}")
                return {"event": "error", "message": str(e)}

    async def read_memory(self, address: int, size: int) -> bytes:
        """Read bytes from PSP memory."""
        response = await self.send_request(
            "memory.read",
            address=address,
            size=size
        )

        if response.get("event") == "error":
            raise RuntimeError(f"Memory read error: {response.get('message')}")
        elif "base64" in response:
            return base64.b64decode(response["base64"])
        else:
            raise RuntimeError(f"Unexpected response: {response}")

    async def write_memory(self, address: int, data: bytes) -> bool:
        """Write bytes to PSP memory."""
        response = await self.send_request(
            "memory.write",
            address=address,
            base64=base64.b64encode(data).decode("ascii")
        )
        return response.get("event") != "error"

    async def read_u32(self, address: int) -> int:
        """Read 32-bit unsigned integer."""
        data = await self.read_memory(address, 4)
        return struct.unpack("<I", data)[0]

    async def read_u16(self, address: int) -> int:
        """Read 16-bit unsigned integer."""
        data = await self.read_memory(address, 2)
        return struct.unpack("<H", data)[0]

    async def read_u8(self, address: int) -> int:
        """Read 8-bit unsigned integer."""
        data = await self.read_memory(address, 1)
        return data[0]

    async def write_u32(self, address: int, value: int) -> bool:
        """Write 32-bit unsigned integer."""
        return await self.write_memory(address, struct.pack("<I", value))

    async def write_u16(self, address: int, value: int) -> bool:
        """Write 16-bit unsigned integer."""
        return await self.write_memory(address, struct.pack("<H", value))

    async def write_u8(self, address: int, value: int) -> bool:
        """Write 8-bit unsigned integer."""
        return await self.write_memory(address, bytes([value]))

    @staticmethod
    def find_ppsspp_port() -> Optional[int]:
        """Try to find PPSSPP's listening port via lsof."""
        try:
            result = subprocess.run(
                ["lsof", "-i", "-P", "-n"],
                capture_output=True,
                text=True
            )
            for line in result.stdout.split("\n"):
                if "PPSSPP" in line and "LISTEN" in line:
                    # Extract port from line like "TCP *:54453 (LISTEN)"
                    parts = line.split()
                    for part in parts:
                        if part.startswith("*:") or "::" in part:
                            port_str = part.split(":")[-1]
                            if port_str.isdigit():
                                return int(port_str)
        except Exception:
            pass
        return None


# =============================================================================
# Game State Tracking
# =============================================================================

# Item count for Ys I
YS1_ITEM_COUNT = 52  # Items 0-51

# Maximum number of chest flags to scan
MAX_CHEST_FLAGS = 63  # Up to 63 before overlapping with item ownership


class YsGameState:
    """Tracks game state by reading PPSSPP memory."""

    def __init__(self, ppsspp: PPSSPPInterface):
        self.ppsspp = ppsspp
        self.current_game: Optional[str] = None  # "ys1" or "ys2"

        # Item tracking - stores last known state of each item
        self.item_states: Dict[int, bool] = {}  # item_id -> owned
        self.checked_locations: Set[int] = set()  # Items we've already reported

        # Chest flag tracking
        self.prev_chest_flags: Dict[int, bool] = {}  # chest_id -> opened

        # Boss flag tracking
        self.prev_boss_flags: Dict[int, bool] = {}  # boss_id -> defeated

        # Items given by the client (to avoid reporting as location checks)
        self.client_given_items: Set[int] = set()

        # AP-received items that need to persist across save loads
        self.ap_received_items: Set[int] = set()

        # Save/load detection via checksum
        self.last_state_checksum: Optional[str] = None

        # Other tracking
        self.received_items: List[int] = []
        self.last_hp = 0
        self.last_gold = 0

    async def detect_game(self) -> Optional[str]:
        """Detect whether Ys I or Ys II is active."""
        try:
            # Try reading Ys I gold - if it's a reasonable value, we're in Ys I
            ys1_gold = await self.ppsspp.read_u32(Ys1Addresses.absolute(Ys1Addresses.GOLD))
            ys1_hp = await self.ppsspp.read_u32(Ys1Addresses.absolute(Ys1Addresses.HP))

            # Try reading Ys II gold
            ys2_gold = await self.ppsspp.read_u32(Ys2Addresses.absolute(Ys2Addresses.GOLD))
            ys2_hp = await self.ppsspp.read_u32(Ys2Addresses.absolute(Ys2Addresses.HP))

            # Heuristic: valid stats are non-zero and reasonable
            ys1_valid = 0 < ys1_hp < 1000 and ys1_gold < 1000000
            ys2_valid = 0 < ys2_hp < 1000 and ys2_gold < 1000000

            if ys1_valid and not ys2_valid:
                self.current_game = "ys1"
            elif ys2_valid and not ys1_valid:
                self.current_game = "ys2"
            elif ys1_valid and ys2_valid:
                # Both valid - use HP as tiebreaker (whichever changed recently)
                self.current_game = "ys1"  # Default to Ys I
            else:
                self.current_game = None

            return self.current_game
        except Exception:
            return None

    async def get_stats(self) -> Dict[str, int]:
        """Read current player stats."""
        if self.current_game == "ys1":
            addr = Ys1Addresses
        elif self.current_game == "ys2":
            addr = Ys2Addresses
        else:
            return {}

        stats = {
            "hp": await self.ppsspp.read_u32(addr.absolute(addr.HP)),
            "attack": await self.ppsspp.read_u32(addr.absolute(addr.ATTACK)),
            "defense": await self.ppsspp.read_u32(addr.absolute(addr.DEFENSE)),
            "gold": await self.ppsspp.read_u32(addr.absolute(addr.GOLD)),
        }

        if self.current_game == "ys2":
            stats["mp"] = await self.ppsspp.read_u32(addr.absolute(addr.MP))

        return stats

    async def read_all_items(self) -> Dict[int, bool]:
        """Read ownership state of all items in one bulk read."""
        if self.current_game != "ys1":
            return {}

        items = {}
        try:
            # Read entire item ownership array in one request (52 items * 4 bytes = 208 bytes)
            base_addr = Ys1Addresses.absolute(Ys1Addresses.ITEM_OWNERSHIP)
            data = await self.ppsspp.read_memory(base_addr, YS1_ITEM_COUNT * 4)

            # Parse each 4-byte value
            for item_id in range(YS1_ITEM_COUNT):
                offset = item_id * 4
                value = struct.unpack("<I", data[offset:offset+4])[0]
                items[item_id] = (value != 0)
        except Exception as e:
            print(f"read_all_items error: {e}")
            # Fall back to empty dict on error
            for item_id in range(YS1_ITEM_COUNT):
                items[item_id] = False

        return items

    async def read_chest_flags(self, count: int = MAX_CHEST_FLAGS) -> Dict[int, bool]:
        """Read chest opened flags in one bulk read."""
        if self.current_game != "ys1":
            return {}

        flags = {}
        try:
            base_addr = Ys1Addresses.absolute(Ys1Addresses.CHEST_FLAGS)
            data = await self.ppsspp.read_memory(base_addr, count * 4)

            for chest_id in range(count):
                offset = chest_id * 4
                value = struct.unpack("<I", data[offset:offset + 4])[0]
                flags[chest_id] = (value != 0)
        except Exception as e:
            print(f"read_chest_flags error: {e}")

        return flags

    async def read_boss_flags(self) -> Dict[int, bool]:
        """
        Read boss defeated flags.

        WARNING: BOSS_FLAGS_BASE is a placeholder address. Before this will work,
        the actual boss flag addresses must be discovered using memory_diff.py:
          1. Take snapshot before boss fight
          2. Defeat boss
          3. Diff the snapshots
          4. Look for 0->1 transitions near 0x0177xxx

        Boss order: 0=Jenocres, 1=Vagullion, 2=Pictimos, 3=Khonsclard, 4=Dark Fact
        """
        if self.current_game != "ys1":
            return {}

        flags = {}
        try:
            base_addr = Ys1Addresses.absolute(Ys1Addresses.BOSS_FLAGS_BASE)
            data = await self.ppsspp.read_memory(base_addr, 5 * 4)

            for boss_id in range(5):
                offset = boss_id * 4
                value = struct.unpack("<I", data[offset:offset + 4])[0]
                flags[boss_id] = (value != 0)
        except Exception:
            # Boss flags not yet discovered — silently fail
            pass

        return flags

    async def check_new_locations(self) -> Tuple[Set[int], Set[int], Set[int]]:
        """
        Check for newly acquired items, opened chests, and defeated bosses.

        Returns:
            Tuple of (new_item_ids, new_chest_ids, new_boss_ids) that transitioned 0->1
        """
        if self.current_game != "ys1":
            return set(), set(), set()

        new_items = set()
        new_chests = set()
        new_bosses = set()

        # --- Check item ownership transitions ---
        current_items = await self.read_all_items()
        for item_id, owned in current_items.items():
            was_owned = self.item_states.get(item_id, False)
            if owned and not was_owned:
                if item_id in self.client_given_items:
                    self.client_given_items.discard(item_id)
                elif item_id not in self.checked_locations:
                    new_items.add(item_id)
                    self.checked_locations.add(item_id)
                    print(f"  Item acquired: {item_id}")
        self.item_states = current_items

        # --- Check chest flag transitions ---
        current_chests = await self.read_chest_flags()
        for chest_id, opened in current_chests.items():
            prev = self.prev_chest_flags.get(chest_id, False)
            if opened and not prev:
                new_chests.add(chest_id)
                print(f"  Chest opened: {chest_id} (flag addr: 0x{Ys1Addresses.CHEST_FLAGS + chest_id * 4:07X})")
        self.prev_chest_flags = current_chests

        # --- Check boss flag transitions ---
        current_bosses = await self.read_boss_flags()
        boss_names = {0: "Jenocres", 1: "Vagullion", 2: "Pictimos", 3: "Khonsclard", 4: "Dark Fact"}
        for boss_id, defeated in current_bosses.items():
            prev = self.prev_boss_flags.get(boss_id, False)
            if defeated and not prev:
                new_bosses.add(boss_id)
                print(f"  Boss defeated: {boss_names.get(boss_id, boss_id)}")
        self.prev_boss_flags = current_bosses

        return new_items, new_chests, new_bosses

    async def give_item(self, item_id: int, auto_equip: bool = True) -> bool:
        """
        Give an item to the player.

        Args:
            item_id: The item ID (0-19 for equipment, 20+ for quest items)
            auto_equip: If True, automatically equip the item if it's equipment

        Returns:
            True if successful
        """
        if self.current_game != "ys1":
            print(f"give_item: Only Ys I is supported currently")
            return False

        try:
            # Set BOTH ownership and discovered flags
            ownership_addr = Ys1Addresses.item_ownership_addr(item_id)
            discovered_addr = Ys1Addresses.item_discovered_addr(item_id)

            # Ownership makes item selectable/equippable
            success1 = await self.ppsspp.write_u32(ownership_addr, 1)
            # Discovered shows item in inventory list
            success2 = await self.ppsspp.write_u32(discovered_addr, 1)

            if not (success1 and success2):
                print(f"give_item: Failed to set flags for item {item_id}")
                return False

            print(f"give_item: Gave item {item_id}")

            # Track that we gave this item (don't report as location check)
            self.client_given_items.add(item_id)

            # Auto-equip if it's equipment
            if auto_equip:
                equip_slot = Ys1Addresses.get_equip_slot(item_id)
                if equip_slot:
                    equip_addr = Ys1Addresses.absolute(equip_slot)
                    await self.ppsspp.write_u32(equip_addr, item_id)
                    print(f"give_item: Auto-equipped item {item_id} in slot 0x{equip_slot:07X}")

            return True

        except Exception as e:
            print(f"give_item: Error - {e}")
            return False

    async def remove_item(self, item_id: int) -> bool:
        """Remove an item from the player's inventory."""
        if self.current_game != "ys1":
            return False

        try:
            # Clear both ownership and discovered flags
            ownership_addr = Ys1Addresses.item_ownership_addr(item_id)
            discovered_addr = Ys1Addresses.item_discovered_addr(item_id)
            await self.ppsspp.write_u32(ownership_addr, 0)
            await self.ppsspp.write_u32(discovered_addr, 0)

            # Unequip if currently equipped
            equip_slot = Ys1Addresses.get_equip_slot(item_id)
            if equip_slot:
                equip_addr = Ys1Addresses.absolute(equip_slot)
                current = await self.ppsspp.read_u32(equip_addr)
                if current == item_id:
                    await self.ppsspp.write_u32(equip_addr, 0xFFFFFFFF)

            return True
        except Exception:
            return False

    async def has_item(self, item_id: int) -> bool:
        """Check if player owns an item (checks ownership flag)."""
        if self.current_game != "ys1":
            return False

        try:
            ownership_addr = Ys1Addresses.item_ownership_addr(item_id)
            value = await self.ppsspp.read_u32(ownership_addr)
            return value != 0
        except Exception:
            return False

    def compute_state_checksum(self, items: Dict[int, bool]) -> str:
        """Compute a checksum of the current game state."""
        item_bits = ''.join('1' if items.get(i, False) else '0' for i in range(YS1_ITEM_COUNT))
        return hashlib.md5(item_bits.encode()).hexdigest()[:16]

    async def check_save_load(self) -> bool:
        """
        Check if a save was loaded by detecting if AP-given items disappeared.

        Returns True if a save load was detected (and items were re-applied).
        """
        if self.current_game != "ys1":
            return False

        if not self.ap_received_items:
            return False  # Nothing to check

        # Read current items
        current_items = await self.read_all_items()
        current_checksum = self.compute_state_checksum(current_items)

        # If checksum hasn't changed, no need to check further
        if current_checksum == self.last_state_checksum:
            return False

        # Checksum changed - check if any AP items are missing
        current_owned = {item_id for item_id, owned in current_items.items() if owned}
        missing_ap_items = self.ap_received_items - current_owned

        if missing_ap_items:
            print(f"Save load detected! {len(missing_ap_items)} AP items missing")
            print(f"  Missing: {sorted(missing_ap_items)}")

            # Re-apply all AP-received items
            await self.reapply_ap_items()

            # Update checksum after re-applying
            current_items = await self.read_all_items()
            self.last_state_checksum = self.compute_state_checksum(current_items)
            return True

        # Checksum changed but no AP items missing - normal gameplay
        self.last_state_checksum = current_checksum
        return False

    async def reapply_ap_items(self):
        """Re-apply all items received from AP server."""
        print(f"Re-applying {len(self.ap_received_items)} AP items...")
        for item_id in self.ap_received_items:
            await self.give_item(item_id, auto_equip=False)
            # Mark as client-given so we don't re-report as location check
            self.client_given_items.add(item_id)
        print("AP items re-applied")

    async def receive_ap_item(self, item_id: int, auto_equip: bool = True) -> bool:
        """
        Receive an item from the AP server.

        This tracks the item so it can be re-applied after save loads.
        """
        # Track this item as AP-received
        self.ap_received_items.add(item_id)

        # Give the item
        success = await self.give_item(item_id, auto_equip=auto_equip)

        if success:
            # Update checksum to include this new item
            current_items = await self.read_all_items()
            self.last_state_checksum = self.compute_state_checksum(current_items)

        return success

    def get_sync_data(self) -> dict:
        """Get data to sync with AP server for persistence."""
        return {
            "checksum": self.last_state_checksum,
            "ap_received_items": list(self.ap_received_items),
            "checked_locations": list(self.checked_locations),
        }

    def load_sync_data(self, data: dict):
        """Load sync data from AP server."""
        self.last_state_checksum = data.get("checksum")
        self.ap_received_items = set(data.get("ap_received_items", []))
        self.checked_locations = set(data.get("checked_locations", []))


# =============================================================================
# Main Entry Point
# =============================================================================

async def main():
    """Main client loop."""
    print("=" * 50)
    print("Ys I & II Chronicles - Archipelago Client")
    print("=" * 50)

    # Find PPSSPP port
    port = PPSSPPInterface.find_ppsspp_port()
    if port:
        print(f"Found PPSSPP on port {port}")
    else:
        port = DEFAULT_PPSSPP_PORT
        print(f"Using default port {port}")

    # Connect to PPSSPP
    ppsspp = PPSSPPInterface(port=port)
    if not await ppsspp.connect():
        print("\nMake sure PPSSPP is running with remote debugger enabled.")
        return

    # Initialize game state
    game_state = YsGameState(ppsspp)

    # Detect game
    game = await game_state.detect_game()
    if game:
        print(f"Detected: {game.upper()}")
    else:
        print("Could not detect game. Make sure you're in-game.")

    # Show stats
    stats = await game_state.get_stats()
    if stats:
        print("\nPlayer Stats:")
        for key, value in stats.items():
            print(f"  {key}: {value}")

    # Initialize item tracking (read current state)
    print("\nInitializing item tracking...")
    await game_state.read_all_items()
    # Mark all currently owned items as already checked
    for item_id, owned in game_state.item_states.items():
        if owned:
            game_state.checked_locations.add(item_id)
    print(f"Found {len(game_state.checked_locations)} items already owned")

    # Initialize chest flag tracking
    chest_flags = await game_state.read_chest_flags()
    game_state.prev_chest_flags = chest_flags
    opened_chests = sum(1 for v in chest_flags.values() if v)
    print(f"Chest flags: {opened_chests} already opened")

    # Initialize boss flag tracking
    boss_flags = await game_state.read_boss_flags()
    game_state.prev_boss_flags = boss_flags
    defeated_bosses = sum(1 for v in boss_flags.values() if v)
    if defeated_bosses > 0:
        print(f"Boss flags: {defeated_bosses} already defeated")

    # Initialize checksum for save detection
    current_items = await game_state.read_all_items()
    game_state.last_state_checksum = game_state.compute_state_checksum(current_items)
    print(f"Initial state checksum: {game_state.last_state_checksum}")

    # Main loop
    print("\n[Client ready - monitoring for item acquisitions, chest opens, and boss defeats]")
    print("Press Ctrl+C to exit")

    try:
        while True:
            await asyncio.sleep(0.5)  # Poll twice per second

            # Check for save loads (re-applies AP items if detected)
            save_loaded = await game_state.check_save_load()
            if save_loaded:
                print("  -> State restored after save load")

            # Check for new location checks
            new_items, new_chests, new_bosses = await game_state.check_new_locations()

            if new_items or new_chests or new_bosses:
                # Update checksum after location check
                current_items = await game_state.read_all_items()
                game_state.last_state_checksum = game_state.compute_state_checksum(current_items)

                # TODO: Send to Archipelago server
                for item_id in new_items:
                    print(f"  -> Would send item location check for item {item_id}")
                for chest_id in new_chests:
                    print(f"  -> Would send chest location check for chest {chest_id}")
                for boss_id in new_bosses:
                    boss_names = {0: "Jenocres", 1: "Vagullion", 2: "Pictimos", 3: "Khonsclard", 4: "Dark Fact"}
                    print(f"  -> Would send boss location check for {boss_names.get(boss_id, boss_id)}")

    except KeyboardInterrupt:
        print("\nShutting down...")
    finally:
        await ppsspp.disconnect()


if __name__ == "__main__":
    asyncio.run(main())
