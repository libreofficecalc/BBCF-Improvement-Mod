# URT Automation (Temporary)

This folder is a temporary harness for repetitive URT repro loops.

## Files
- `Run-UrtAutomation.ps1`: runner that launches AutoHotkey, watches BBCF, and reports machine-readable status.
- `urt_macro_template.ahk`: editable input macro template (AutoHotkey v2).
- `runtime/`: generated per-run signal/result/log files.

## Handshake model (AHK <-> PowerShell)
Runner passes:
- `/signal=<path>`
- `/result=<path>`

AHK writes:
- Signal file: startup marker.
- Result file first line: final status token (`DONE` or `SCRIPT_REPORTED_FAILURE`).
- Result file second line (optional): detail text.

Runner prints:
- `AUTOMATION_STATUS: <token>`
- `AUTOMATION_DETAIL: ...`
- `AUTOMATION_RUNNER_LOG: <path>`
- `AUTOMATION_SIGNAL_FILE: <path>`
- `AUTOMATION_RESULT_FILE: <path>`

Codex can key off `AUTOMATION_STATUS` and exit code.

## Status and exit codes
- `DONE` -> exit `0`
- `SCRIPT_REPORTED_FAILURE` -> exit `13`
- `TIMEOUT` -> exit `10`
- `AHK_EXITED_WITHOUT_RESULT` -> exit `11`
- `BBCF_PROCESS_NOT_RUNNING` -> exit `12`
- anything else -> exit `14`

## Usage
From PowerShell:

```powershell
cd <repo>\tools\urt_automation
.\Run-UrtAutomation.ps1 -LaunchGame -TimeoutSec 300
```

If your AutoHotkey executable is in a custom location:

```powershell
.\Run-UrtAutomation.ps1 -LaunchGame -AhkExe "C:\Path\To\AutoHotkey64.exe"
```

If BBCF is already running:

```powershell
.\Run-UrtAutomation.ps1 -TimeoutSec 300
```

If you want runner to close BBCF only on success:

```powershell
.\Run-UrtAutomation.ps1 -LaunchGame -CloseGameOnDone
```

For the single-cycle runner using your root `BBCF-Automatic-Debugger.ahk`, from this repo terminal (bash):

```bash
./tools/urt_automation/run_bbcf_debug_cycle.sh
```

With timeout override (seconds):

```bash
./tools/urt_automation/run_bbcf_debug_cycle.sh 1200
```

If AutoHotkey is installed in a non-standard path, set it explicitly:

```bash
AHK_EXE="/mnt/c/path/to/AutoHotkey64.exe" ./tools/urt_automation/run_bbcf_debug_cycle.sh
```

After a cycle, you can analyze the newest seeded-vs-entry mismatch pair:

```bash
./tools/urt_automation/analyze_latest_mismatch.sh
```

Optional game path override:

```bash
./tools/urt_automation/analyze_latest_mismatch.sh "/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction"
```

`run_bbcf_debug_cycle.sh` now auto-runs this analyzer after the PowerShell cycle returns.  
Disable auto-analysis for a run by setting:

```bash
URT_ANALYZE_MISMATCH=0 ./tools/urt_automation/run_bbcf_debug_cycle.sh
```

Additional offline analyzers:

```bash
python3 ./tools/urt_automation/analyze_mismatch_series.py --blobs-dir "/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/URT_RE_BLOBS" --limit 8 --header-bytes 512
python3 ./tools/urt_automation/analyze_pointer_deltas.py --blobs-dir "/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/URT_RE_BLOBS" --limit 8 --scan-bytes 4096
python3 ./tools/urt_automation/analyze_mismatch_heatmap.py --blobs-dir "/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction/BBCF_IM/URT_RE_BLOBS" --limit 8 --chunk 4096
```

Important:
- During execution, the AHK macro may fully control keyboard/mouse input.
- If invoking PowerShell directly, use a Windows `C:\...` path for `-File` (not `/mnt/...`).

## What "one cycle" means right now
For `BBCF-Automatic-Debugger.ahk`, one cycle is the full toast-labeled flow:
1. Focus Visual Studio and wait for build/game launch.
2. Skip BBCF intro and navigate to replay theater.
3. Play replay, skip replay intro, pause.
4. Open mod menu -> `States library` -> `Replay takeover` -> `Unlimited replay takeover`.
5. Start URT record (`Record on match replay as P2`), play/pause replay, then stop/save recording.
6. Return to main menu and navigate to character select.
7. Select training setup (`Ragna P1`, `Kokonoe P2`, `Lab BG`, random music), enter lab, skip lab intro.
8. Run final URT UI clicks, show `DONE` toast, and exit AHK.

The runner treats AHK process exit as completion and then closes `BBCF.exe` if still running.

## Editing the macro
Edit `urt_macro_template.ahk` directly:
- Use `Sleep(ms)` for timing windows.
- Use `Tap("j")`, `TapMany("Up", 2)`, etc.
- Set `expectGameRunningAtEnd := true/false` to define expected final process state.
- Keep `WriteResult(...)` calls at the end so the runner always receives completion.

## Notes
- This is intentionally temporary and script-driven.
- If BBCF disappears before the macro reports completion, runner emits `BBCF_PROCESS_NOT_RUNNING` (could be crash or manual close).
