# Controller override and keyboard separation internals

_Reverse-engineering notes for this feature lean on the memory map published by **tadatys** in the [BBCF-Ghidra repository](https://github.com/Tadatys/BBCF-Ghidra), preserved under `docs/Research/Tadatys-BBCF-Ghidra/` for reference._

## Refresh controllers
- **UI hook:** `Overlay/Window/MainWindow.cpp` uses the Controller Override Manager when the **Refresh controllers** button is pressed.
- **What happens:**
  1. Re-enumerates DirectInput/WinMM devices to update the override list.
  2. "Bounces" every DirectInput device the game has already created by unacquiring and reacquiring the handle. This helps BlazBlue notice devices that appeared after launch without restarting the game.
  3. Sends a `WM_DEVICECHANGE`/`DBT_DEVNODES_CHANGED` message to the game window to mimic an OS-level device hotplug notification.
- **Key code paths:**
  - `ControllerOverrideManager::RefreshDevicesAndReinitializeGame` orchestrates the re-enumeration, device bounce, and broadcast.
  - `ControllerOverrideManager::RegisterCreatedDevice` tracks every device the game opens through our `DirectInput8` wrapper so we can reacquire them later.
  - `DirectInput8A/WWrapper::CreateDevice` registers each created device with the manager.

## Separate keyboard and controllers toggle
- **UI hook:** The "Separate keyboard and controllers" checkbox lives in `Overlay/Window/MainWindow.cpp`.
- **How it works:**
  - The checkbox toggles a swap in the game's internal controller slots by editing the structure at `GetBbcfBaseAdress() + 0x8929c8` (named `battle_key_controller` in comments).
  - It swaps the pointers for menu controls, character controls, and two auxiliary slots between Player 1 and Player 2. This detaches the shared keyboard mapping so the keyboard can belong to a different player than the first detected controller.
  - The address and layout come from reverse-engineering the game's controller table rather than any public API; the code treats it as an opaque array of pointers and manually reorders the entries in place when the toggle changes.
- **Notes:** The structure layout is still handled as raw pointers (see the `battle_key_controller` comment block); formalizing it into a typed struct would make future work safer.
