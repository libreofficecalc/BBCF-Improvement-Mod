# Input reverse-engineering notes

_These findings are cross-referenced against the Ghidra exports shared by **tadatys** in the [BBCF-Ghidra repository](https://github.com/Tadatys/BBCF-Ghidra), which live in `docs/Research/Tadatys-BBCF-Ghidra/` for future contributors._

## How the keyboard/controller split was discovered
- The toggle does not call any documented API. Instead, it writes directly into the game's `battle_key_controller` table at `GetBbcfBaseAdress() + 0x8929c8`, swapping four pointer slots between the two players (menu controls, two auxiliary slots, and character controls).
- The code treats the table as an opaque block of pointers rather than a declared struct, which implies the layout and address were obtained by inspecting the game's memory (e.g., Cheat Engine or Ghidra) rather than headers.
- Because the swap is limited to reordering those pointers, it likely came from watching how the table changes when the game binds controllers and then reproducing that layout in our overlay code.

## Evidence of reverse-engineered assets in the repo
- The project includes a `src/Game/GhidraDefs.h` header full of typedefs and stub structs emitted by Ghidra, but there is no corresponding decompiled source tree for the game's code.
- Aside from small research notes (`notes.h`) and the manually coded pointer swap in `MainWindow.cpp`, there is no packaged disassembly or decompiler output that would explain the controller table; developers have been working from targeted offsets instead of a full decompile.

## Applying the same approach to hotplug refresh
- Our DirectInput wrapper already intercepts `CreateDevice`, tracks each `IDirectInputDevice8` handle, and exposes a `RefreshDevicesAndReinitializeGame` method that re-enumerates devices and re-acquires the tracked handles; however, the game still ignores new devices, suggesting its own input table stays cached.
- Mimicking the keyboard/controller split approach would mean locating the game's internal array of controller device pointers (similar to the `battle_key_controller` table) via memory inspection, then either forcing the game to rebuild that array or swapping in newly created device pointers when the refresh button is pressed.
- The existing wrappers and hooks are sufficient to collect device GUIDs and send `WM_DEVICECHANGE`, so the remaining work is reverse-engineering the game's controller table and writing a safe patch point—just as was done for the keyboard swap, but targeted at the device list instead of the keyboard/controller ownership table.
