# Outfit Wheeler - Dynamic Equipment Manager

A high-performance equipment management and radial wheel system for The Elder Scrolls V: Skyrim Special Edition and Anniversary Edition. Built with native C++23, CommonLibSSE-NG, and a hardware-accelerated Prisma UI web overlay.

[![Nexus Mods](https://img.shields.io/badge/Nexus%20Mods-187557-orange.svg)](https://www.nexusmods.com/skyrimspecialedition/mods/187557)
[![Skyrim Version](https://img.shields.io/badge/Skyrim-SE%20%7C%20AE-blue.svg)](https://store.steampowered.com/app/489830/)
[![SKSE64](https://img.shields.io/badge/SKSE64-2.2.6+-informational.svg)](https://skse.silverlock.org/)
[![C++ Standard](https://img.shields.io/badge/Language-C%2B%2B23-00599C.svg)](https://en.cppreference.com/w/cpp/23)
[![License](https://img.shields.io/badge/License-GPL--3.0-green.svg)](LICENSE)
[![Backend Tests](https://img.shields.io/badge/Backend%20Tests-16%2F16%20Passed-brightgreen.svg)](#running-test-suites)
[![Frontend Tests](https://img.shields.io/badge/Frontend%20Tests-32%2F32%20Passed-brightgreen.svg)](#running-test-suites)

---

## Overview

[Outfit Wheeler on Nexus Mods](https://www.nexusmods.com/skyrimspecialedition/mods/187557) provides an instant, responsive radial menu for swapping complete outfits, individual armor pieces, clothing, and accessories directly in game without pausing through vanilla inventory screens.

Outfit Wheeler is engineered strictly for apparel. With built-in **Pure Apparel Protection**, it never modifies, unequips, or interferes with your equipped weapons, shields, or spells — ensuring zero conflicts alongside dedicated weapon wheels like *Wheeler* or *dMenu*.

Equipping items is backed by a thread-safe execution engine that dispatches all Skyrim engine modifications onto the main game thread via `SKSE::GetTaskInterface()->AddTask(...)`, preventing race conditions and thread lockups. Visuals are rendered through an isolated offscreen DirectX 11 pipeline that extracts in-engine item meshes, renders lighting-calibrated thumbnails, and caches them to disk.

---

## Key Features

### Radial Wheel & Audition Mode
- Up to 8 configurable wheel pages with dedicated sub-categories (All, Combat, Swim, City, Home).
- **Pure Apparel Protection**: Strictly manages clothing, armor, and accessories. Equipped weapons, shields, and magic hands are completely shielded and left untouched during all outfit transitions.
- **Audition Mode**: Shift-click multiple items inside menus to preview their appearance in real time on the player character without closing the wheel or committing the selection.
- Smooth analog gamepad steering (360-degree angle resolution with deadzone compensation) and DirectInput keyboard scan code navigation.

### Dynamic Adaptive Wardrobe (Auto-Equip Engine)
- Context-sensitive rule evaluator running 23 situational conditions.
- Automatically swaps outfits when entering cities, entering player homes, entering combat, or swimming in water.
- Star preference weighting system with repetition penalties to cycle through matching wardrobe sets dynamically.
- Auto-stash mechanism that safely tucks inactive gear into dedicated storage containers to maintain player carry capacity.

### Mod Explorer & Item Spawner
- On-the-fly catalog indexing of all loaded plugins (`.esm`, `.esp`, `.esl`) via Skyrim's `TESDataHandler`.
- Viewport-based lazy chunk loading and DOM recycling, benchmarked against 500+ plugins and 3,000+ items to eliminate UI lockup during mass catalog expansions.
- Instant item spawning and auditioning directly from any mod in your load order.

### Standalone D3D11 Thumbnail Pipeline
- Extracts skinned and unskinned `.nif` item meshes directly from game archives and data paths.
- Strips custom body prefixes (`cbbe_`, `3ba_`) and protects slot 32 cuirass geometry for true-to-character icon framing.
- Dedicated DirectX 11 offscreen render target with strict state preservation (saved render target views, depth stencil views, and viewports restored to isolate the game's shadow pipeline).
- Multithreaded WIC PNG disk caching with automated loading menu suppression.

### 3D Dressing Studio
- In-game studio mode featuring a 360-degree turntable orbit camera, pedestal lighting, and direct pose switching.
- Allows players to review equipment fit, clipping, and physics in real time.

### Dynamic Theme Engine
- Live customizable color themes with an interactive 2D color spectrum canvas picker.
- Per-slot and per-wheel color overrides.
- Contrast-verified typography and CSS variable tokens for smooth scaling across 1080p, 1440p, and 4K displays.

---

## Requirements

| Component | Minimum Version | Notes |
| :--- | :--- | :--- |
| **Skyrim Special Edition / AE** | `1.5.97` (SE) / `1.6.640`+ (AE) | Tested on SE and AE (VR is untested) |
| **SKSE64** | Version matching your game runtime | Required for DLL loading |
| **Address Library for SKSE Plugins** | Latest for your game version | Required for memory offset resolution |
| **Prisma UI** | Latest release | Hardware-accelerated in-game web rendering engine |

---

## Installation

### Via Mod Manager (Recommended)
1. Download the mod archive from [Nexus Mods (Mod #187557)](https://www.nexusmods.com/skyrimspecialedition/mods/187557).
2. Install as a standard mod archive through Mod Organizer 2 or Vortex.
3. Verify that `Outfit Wheeler` is checked in your active mod list and deployed.

### Manual Installation
Extract the archive contents into your Skyrim `Data/` directory:

```text
Data/
├── SKSE/
│   └── Plugins/
│       ├── OutfitWheeler-DynamicEquipmentManager.dll
│       └── OutfitWheeler.ini
└── PrismaUI/
    └── views/
        └── PrismaOutfitWheel/
            ├── index.html
            ├── style.css
            ├── app.js
            ├── wheel.js
            └── modals.js
```

---

## Controls & Keybindings

### Default Bindings

| Action | Keyboard Binding | DirectInput Scan Code | Gamepad Binding |
| :--- | :--- | :--- | :--- |
| **Open Wheel (Play Mode)** | `V` | `0x2F` (47) | `LB + DPAD Down` |
| **Open Wheel (Edit Mode)** | `N` (or `Shift + M`) | `0x31` (49) / `0x32` (50) | Configurable via INI |
| **Navigate Slots** | Mouse Cursor | N/A | Right Thumbstick (360°) |
| **Equip Item** | Left Click / Enter | `0x1C` (28) | `A Button` |
| **Audition Item** | `Shift + Left Click` | `0x2A` (42) | Configurable |
| **Next / Prev Wheel** | `E` / `Q` | `0x12` / `0x10` | `RB` / `LB` |
| **Toggle Category Filter** | `Tab` / `1-5` | `0x0F` | `DPAD Left / Right` |

*All keybindings can be customized in-game via the Prisma UI Settings Modal or directly in `Data/SKSE/Plugins/OutfitWheeler.ini`.*

---

## Configuration

Settings are saved in `Data/SKSE/Plugins/OutfitWheeler.ini`:

```ini
[Theme]
sThemeAccentColor=#38bdf8
sThemeBgColor=#070a0f
sThemeTextColor=#f8fafc
sThemePreset=sapphire
fThemeDepth=0.60

[Hotkeys]
kPlayKey=47             ; 0x2F = 'V'
kPlayModifier=0         ; 0 = None, 29 = Ctrl, 42 = Shift
kEditModeKey=49         ; 0x31 = 'N'
kEditModeModifier=0

[Gamepad]
kGamepadPlayKey=8       ; 0x0008 = DPAD Down
kGamepadPlayModifier=256 ; 0x0100 = Left Bumper (LB)
fThumbstickDeadzone=0.25

[Appearance]
fWheelScale=1.00
fWheelOffsetX=0.00
fWheelOffsetY=0.00
fUiScale=1.00
uEditUiMode=1
bShowItemPreviews=1     ; 1 = Enable 3D D3D11 thumbnails, 0 = Classic inventory list

[AutoSystem]
bAutoSystem=1           ; Enable contextual auto-equipping
bAutoCity=1             ; Auto-equip town/casual wear in populated settlements
bAutoHome=1             ; Auto-equip sleep/loungewear in player residences
bAutoCombat=1           ; Auto-equip battle armor when entering combat threat
bAutoSwim=1             ; Auto-equip swimsuits when submerged
bAutoStashInactive=1    ; Safely move unused armor sets to player stash
bAutoCloseOnEquip=1     ; Automatically close wheel upon equipping a slot
```

---

## Building from Source

### Prerequisites
- Visual Studio 2022 (v143 toolset with C++ Desktop Development workload)
- [xmake](https://xmake.io/) (version 2.8.2 or higher)
- Git (configured with submodule support)
- Node.js (v18+ recommended for frontend unit tests)

### Compilation Steps

1. **Clone the repository with submodules:**
   ```bash
   git clone --recursive https://github.com/Ariowss/Outfit-Wheeler---Dynamic-Equipment-Manager.git
   cd Outfit-Wheeler---Dynamic-Equipment-Manager
   ```

2. **Configure and build the release DLL:**
   ```bash
   xmake config -m release
   xmake build OutfitWheeler-DynamicEquipmentManager
   ```
   The compiled plugin outputs to `build/windows/x64/release/OutfitWheeler-DynamicEquipmentManager.dll`.

3. **Build the native backend test suite:**
   ```bash
   xmake build OutfitWheeler-BackendTests
   ```

---

## Running Test Suites

Outfit Wheeler carries full, runnable automated verification across both backend C++ algorithms and frontend JavaScript state machines.

### 1. Native Backend Test Suite (16/16 Pass)
Tests INI roundtripping, FormID/ESL arithmetic, tag condition parsing, star preference weighting, enchantment matching, and WIC disk cache persistence:

```bash
xmake run OutfitWheeler-BackendTests
```

### 2. Frontend State Machine Test Suite (32/32 Pass)
Tests wheel CRUD, slot lifecycle, floating panel drag/clamp, 12-criteria inventory sorting, analog gamepad math, mod explorer chunking, and 2D spectrum color picker logic:

```bash
node tests/test_frontend.js
```

### 3. Integration & Input Safety Audit (11/11 Pass)
Validates input event suppression, absence of static XInput linkage, and backup restoration bindings:

```bash
node tests/auto_test.js
```

---

## Architecture & Codebase Map

```text
Outfit-Wheeler/
├── src/
│   ├── AdaptiveWardrobeManager.cpp/.h # 23-condition auto-equip engine & location tags
│   ├── EquipmentManager.cpp/.h        # Equip/unequip events, armor shielding, follower sync
│   ├── ExecutionQueue.cpp/.h          # Thread-safe SKSE task dispatcher
│   ├── InputHandler.cpp/.h            # DirectInput keyboard hooks & 360° gamepad analog math
│   ├── JSInterop.cpp/.h               # High-speed native C++ to PrismaUI JSON bridge
│   ├── ModCatalogManager.cpp/.h       # Real-time TESDataHandler plugin indexing & item spawner
│   ├── OutfitManager.cpp/.h           # Outfits/slots CRUD, JSON storage & .bak backups
│   ├── Settings.cpp/.h                # INI configuration loader & persistence
│   ├── thumbnail/
│   │   ├── ItemMeshExtractor.cpp/.h   # NIF mesh reading, skin stripping, slot 32 preservation
│   │   ├── ItemPreviewRenderer.cpp/.h # Standalone D3D11 offscreen renderer & state recovery
│   │   ├── ItemThumbnailService.cpp/.h# Worker queue & loading menu render safety guards
│   │   └── ThumbnailDiskCache.cpp/.h  # WIC PNG encoding and filesystem cache
│   └── main.cpp                       # SKSE plugin entrypoint & messaging listener
├── view/
│   ├── index.html                     # Prisma UI layout structure & modals
│   ├── style.css                      # Token-driven theme engine & responsive layout
│   ├── app.js                         # Core UI state machine, drag-and-drop, mod explorer
│   ├── wheel.js                       # Radial wheel trigonometry, slice rendering, hit detection
│   └── modals.js                      # Color spectrum picker, settings editor, backup manager
├── tests/
│   ├── test_backend.cpp               # 16 Catch2 native backend tests
│   ├── test_frontend.js               # 32 Headless DOM frontend unit tests
│   └── auto_test.js                   # 11 Regression & input safety checks
└── xmake.lua                          # Unified C++23 build configuration
```

---

## Troubleshooting & FAQ

#### Why are thumbnails taking a few seconds on first load?
Thumbnails are generated dynamically on demand directly from `.nif` meshes using an offscreen D3D11 rendering context. Once rendered, each thumbnail is written to disk in PNG format and served instantly from cache on all subsequent sessions.

#### The wheel menu does not open when pressing the hotkey.
Ensure **Prisma UI** is correctly installed and functioning in your game. Also confirm that no blocking modal menu (such as Console, Journal, or a Loading Screen) is currently active, as hotkey inputs are suppressed during modal states to prevent input bleeding.

#### How do I restore an earlier configuration?
Outfit Wheeler creates automatic timestamped `.bak` files on every save. You can open the **Backups Modal** inside the settings panel to inspect snapshots and revert with a single click.

---

## License

This project is licensed under the **GNU General Public License v3.0** (`GPL-3.0`). See the [`LICENSE`](LICENSE) file for the full license text.

## Author

- **Ariowss** ([@Ariowss](https://github.com/Ariowss))

## Credits & Acknowledgments

- **dTry**: For inspiring the radial interface concept with *Wheeler*.
- **Prisma UI Team**: For the in-game web rendering engine and API.
- **CommonLibSSE-NG & SKSE Teams**: For the reverse engineering and SKSE infrastructure.
