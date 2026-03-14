"""
Ys I Chronicles - Region Definitions

Defines the game regions and their connections with access rules.
"""

from typing import Dict, List, Callable, TYPE_CHECKING

if TYPE_CHECKING:
    from BaseClasses import CollectionState


# =============================================================================
# Region Definitions
# =============================================================================

YS1_REGIONS: Dict[str, str] = {
    "Menu": "Starting point (Archipelago origin)",
    "Minea": "Starting town with shops and Sara",
    "Barbado": "Port town where Slaff gives a sword",
    "Zepik": "Village with Jeba and the Shrine Key",
    "Minea Fields": "Overworld fields with Roda Trees and locked chest",
    "Shrine F1": "First floor of the Shrine",
    "Shrine B1": "Shrine basement 1 (locked chests)",
    "Shrine B2": "Shrine basement 2 (Prison Key area)",
    "Shrine B3": "Shrine basement 3 (Jenocres boss)",
    "Mine F1": "Abandoned Mine entrance",
    "Mine B1": "Mine basement 1",
    "Mine B2": "Mine basement 2 (Vagullion boss)",
    "Tower Lower": "Darm Tower floors 1-7",
    "Tower F8": "Darm Tower floor 8 (Pictimos boss)",
    "Tower Mid": "Darm Tower floors 9-13",
    "Tower F14": "Darm Tower floor 14 (Khonsclard boss)",
    "Tower Upper": "Darm Tower floors 15-21",
    "Tower Top": "Darm Tower floors 22-25 (Dark Fact)",
}


# =============================================================================
# Region Connections
# =============================================================================

YS1_CONNECTIONS: List[tuple] = [
    # Starting area
    ("Menu", "Minea", None),
    ("Minea", "Barbado", None),
    ("Minea", "Zepik", None),
    ("Minea", "Minea Fields", None),

    # Shrine access — Sara's Crystal needed for warp statue, Shrine Key for door
    ("Zepik", "Shrine F1", "can_enter_shrine"),
    ("Shrine F1", "Shrine B1", None),
    ("Shrine B1", "Shrine B2", None),
    ("Shrine B2", "Shrine B3", "has_ivory_key"),

    # Mine access — freely accessible from the overworld
    ("Minea Fields", "Mine F1", None),
    ("Mine F1", "Mine B1", None),
    ("Mine B1", "Mine B2", None),

    # Tower access — point of no return, must complete all pre-tower areas first
    ("Minea Fields", "Tower Lower", "can_enter_tower"),
    ("Tower Lower", "Tower F8", None),

    # Tower progression
    ("Tower F8", "Tower Mid", None),              # Defeating Pictimos opens the path
    ("Tower Mid", "Tower F14", "has_hammer"),      # Hammer to break walls
    ("Tower F14", "Tower Upper", "has_rod"),       # Rod to activate statues
    ("Tower Upper", "Tower Top", "has_all_books"),
]


# =============================================================================
# Access Rules
# =============================================================================

def has_shrine_key(state: "CollectionState", player: int) -> bool:
    return state.has("Shrine Key", player)


def can_enter_shrine(state: "CollectionState", player: int) -> bool:
    """Sara's Crystal activates the warp statue, Shrine Key opens the door."""
    return state.has("Sara's Crystal", player) and state.has("Shrine Key", player)


def has_ivory_key(state: "CollectionState", player: int) -> bool:
    return state.has("Ivory Key", player)


def has_marble_key(state: "CollectionState", player: int) -> bool:
    return state.has("Marble Key", player)


def has_mask_of_eyes(state: "CollectionState", player: int) -> bool:
    return state.has("Mask of Eyes", player)


def has_mask_and_marble(state: "CollectionState", player: int) -> bool:
    return state.has("Mask of Eyes", player) and state.has("Marble Key", player)


def has_prison_key(state: "CollectionState", player: int) -> bool:
    return state.has("Prison Key", player)


def has_darm_key(state: "CollectionState", player: int) -> bool:
    return state.has("Darm Key", player)


def can_enter_tower(state: "CollectionState", player: int) -> bool:
    """Tower is point of no return — Goban checks equipment + books, and all
    pre-tower gating items must be obtainable before entering."""
    return (
        state.has("Darm Key", player) and
        # Goban requires all Silver equipment
        state.has("Silver Sword", player) and
        state.has("Silver Shield", player) and
        state.has("Silver Armor", player) and
        # Jeba must read all 3 pre-tower Books of Ys
        state.has("Book of Ys (Hadal)", player) and
        state.has("Book of Ys (Tovah)", player) and
        state.has("Book of Ys (Dabbie)", player) and
        # All keys that gate pre-tower locations
        state.has("Sara's Crystal", player) and
        state.has("Shrine Key", player) and
        state.has("Ivory Key", player) and
        state.has("Marble Key", player) and
        state.has("Mask of Eyes", player) and
        state.has("Treasure Box Key", player) and
        state.has("Prison Key", player) and
        # Quest items that gate pre-tower checks
        state.has("Silver Bell", player) and
        state.has("Roda Tree Seed", player) and
        state.has("Silver Harmonica", player)
    )


def has_hammer(state: "CollectionState", player: int) -> bool:
    return state.has("Hammer", player)


def has_rod(state: "CollectionState", player: int) -> bool:
    return state.has("Rod", player)


def has_all_books(state: "CollectionState", player: int) -> bool:
    return (
        state.has("Book of Ys (Hadal)", player) and
        state.has("Book of Ys (Tovah)", player) and
        state.has("Book of Ys (Dabbie)", player) and
        state.has("Book of Ys (Mesa)", player) and
        state.has("Book of Ys (Gemma)", player) and
        state.has("Book of Ys (Fact)", player)
    )


def has_roda_tree_access(state: "CollectionState", player: int) -> bool:
    return (
        state.has("Roda Tree Seed", player) and
        state.has("Silver Harmonica", player)
    )


def has_silver_bell(state: "CollectionState", player: int) -> bool:
    return state.has("Silver Bell", player)


def has_silver_harmonica(state: "CollectionState", player: int) -> bool:
    return state.has("Silver Harmonica", player)


def has_idol(state: "CollectionState", player: int) -> bool:
    return state.has("Idol", player)


def has_treasure_box_key(state: "CollectionState", player: int) -> bool:
    return state.has("Treasure Box Key", player)


def has_book_hadal(state: "CollectionState", player: int) -> bool:
    return state.has("Book of Ys (Hadal)", player)


def has_blue_necklace(state: "CollectionState", player: int) -> bool:
    return state.has("Blue Necklace", player)


# Map rule names to functions
RULE_FUNCTIONS: Dict[str, Callable] = {
    "has_shrine_key": has_shrine_key,
    "can_enter_shrine": can_enter_shrine,
    "has_ivory_key": has_ivory_key,
    "has_marble_key": has_marble_key,
    "has_mask_of_eyes": has_mask_of_eyes,
    "has_mask_and_marble": has_mask_and_marble,
    "has_prison_key": has_prison_key,
    "has_darm_key": has_darm_key,
    "can_enter_tower": can_enter_tower,
    "has_hammer": has_hammer,
    "has_rod": has_rod,
    "has_all_books": has_all_books,
    "has_roda_tree_access": has_roda_tree_access,
    "has_silver_bell": has_silver_bell,
    "has_silver_harmonica": has_silver_harmonica,
    "has_idol": has_idol,
    "has_treasure_box_key": has_treasure_box_key,
    "has_book_hadal": has_book_hadal,
    "has_blue_necklace": has_blue_necklace,
}


# =============================================================================
# Location-Specific Rules
# =============================================================================

LOCATION_RULES: Dict[str, str] = {
    # Shrine F1 locked chest needs Treasure Box Key
    "Shrine F1 - Shield Ring Chest": "has_treasure_box_key",

    # Shrine B3 — Mask of Eyes reveals hidden walls, Marble Key opens the door
    "Shrine B3 - Marble Key Chest": "has_mask_of_eyes",
    "Shrine B3 - Silver Shield Chest": "has_mask_and_marble",
    "Shrine B3 - Volume Hadal Chest": "has_mask_and_marble",
    "Boss: Jenocres": "has_mask_and_marble",

    # Plains locked chest
    "Minea Fields - Locked Chest": "has_treasure_box_key",

    # Roda Tree requires seed + harmonica
    "Southern Roda Tree": "has_roda_tree_access",

    # Zepik trades
    "Silver Bell Reward": "has_silver_bell",
    "Slaff's Second Gift": "has_prison_key",  # After rescuing Feena

    # Franz gives Tovah after you bring Hadal and Sara's Crystal
    "Franz's Gift": "has_book_hadal",

    # Reah's gift requires Silver Harmonica
    "Reah's Gift": "has_silver_harmonica",

    # Tower trades
    "Raba Trade": "has_idol",

    # Tower F19 Battle Armor chest requires Blue Necklace
    "Tower F19 - Battle Armor Chest": "has_blue_necklace",
}


# =============================================================================
# Boss Location Names (for conditional creation)
# =============================================================================

BOSS_LOCATIONS = [
    "Boss: Jenocres",
    "Boss: Nygtilger",
    "Boss: Vagullion",
    "Boss: Pictimos",
    "Boss: Khonsclard",
    "Boss: Yogleks & Omulgun",
    "Boss: Dark Fact",
]

BOSS_ITEMS = [
    "Book of Ys (Hadal)",    # Jenocres
    "Book of Ys (Dabbie)",   # Vagullion
    "Book of Ys (Mesa)",     # Pictimos
    "Book of Ys (Gemma)",    # Khonsclard
    "Book of Ys (Fact)",     # Dark Fact
]
