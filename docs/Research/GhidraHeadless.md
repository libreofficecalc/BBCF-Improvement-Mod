# Repo-local Ghidra workflow

This repo keeps the Ghidra workflow reproducible, but does not track the generated project database. Ghidra stores large packed DB files that exceed GitHub's 100 MB blob limit, so `docs/Research/BBCF-Ghidra-Project/` is ignored and should be rebuilt locally when needed.

- local project folder: `docs/Research/BBCF-Ghidra-Project/`
- project name: `BBCF`
- program: `BBCF.exe`
- binary source used for import: `D:\SteamLibrary\steamapps\common\BlazBlue Centralfiction\BBCF.exe`
- Ghidra used in this workspace: `D:\Programs\ghidra_11.4.2_PUBLIC`

Tracked repo artifacts:

- headless helper commands: `docs/Research/run_ghidra_*.cmd`
- focused scripts: `docs/Research/ghidra_scripts/`
- small text reports: `docs/Research/RankedLp*GhidraReport.txt`

Ignored local artifacts:

- `docs/Research/BBCF-Ghidra-Project/`

## Run headless decompile reports

From WSL/repo root, run the Windows helpers through `cmd.exe`:

```sh
'/mnt/c/Windows/System32/cmd.exe' /C 'C:\Users\Usuario\source\repos\HaiKamDesu\BBCF-Improvement-Mod\docs\Research\run_ghidra_ranked_lp_decompile.cmd'
```

```sh
'/mnt/c/Windows/System32/cmd.exe' /C 'C:\Users\Usuario\source\repos\HaiKamDesu\BBCF-Improvement-Mod\docs\Research\run_ghidra_ranked_lp_callers.cmd'
```

```sh
'/mnt/c/Windows/System32/cmd.exe' /C 'C:\Users\Usuario\source\repos\HaiKamDesu\BBCF-Improvement-Mod\docs\Research\run_ghidra_ranked_lp_helpers.cmd'
```

Outputs:

- `docs/Research/RankedLpGhidraReport.txt`
- `docs/Research/RankedLpCallersGhidraReport.txt`
- `docs/Research/RankedLpHelpersGhidraReport.txt`

If `BBCF.exe` changes, the ignored project folder is missing, or the project must be recreated, run:

```sh
'/mnt/c/Windows/System32/cmd.exe' /C 'C:\Users\Usuario\source\repos\HaiKamDesu\BBCF-Improvement-Mod\docs\Research\run_ghidra_ranked_lp_import.cmd'
```

The import helper runs full Ghidra analysis and can take several minutes. The decompile/caller/helper reports use `-noanalysis` after the project exists and should finish much faster.

## Notes for agents

- Do not add `docs/Research/BBCF-Ghidra-Project/` to Git. It contains generated binary Ghidra database files over GitHub's normal file limit.
- If Git reports files under `BBCF-Ghidra-Project/`, check `.gitignore` first and remove them from the index with `git rm --cached -r docs/Research/BBCF-Ghidra-Project` if they were staged before the ignore rule existed.
- Prefer Jython scripts (`.py`) for headless reports in this workspace. The Java script path hit a Ghidra/Java 25 OSGi script-load failure even though project import succeeded.
- Keep focused scripts in `docs/Research/ghidra_scripts/`.
- Use Ghidra image-base addresses from docs directly. The imported image base is `0x00400000`.
- Do not route destructive commands through the read-only helper or Ghidra scripts. These scripts only read/decompile and write text reports.
