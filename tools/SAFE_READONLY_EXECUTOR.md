# Safe Read-Only Executor

Purpose:
- Run harmless read-only commands that would otherwise trigger repeated approval prompts when arguments change.
- Primary use in this repo: repeated `cdb.exe` crash-dump analysis.

Files:
- `tools/safe_readonly_exec.ps1`: allowlisted read-only executor with destructive-command guardrails.
- `tools/safe_readonly_request.json`: fixed request payload consumed by `safe_readonly_exec.ps1 -RequestFile ...`.

Policy:
- Hard prohibition: do NOT use this path for delete/destructive commands (`rm`, `del`, `Remove-Item`, move/rename/copy, registry edits, etc.).
- If a command mutates system/game state, use normal explicit command flow and approvals.

Usage pattern (fixed invocation):
1. Write request JSON to `tools/safe_readonly_request.json` with schema:
   - `exe` (string): executable path/name.
   - `args` (string[]): argument list.
2. Invoke the helper with a constant command line:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File C:\Users\Usuario\source\repos\HaiKamDesu\BBCF-Improvement-Mod\tools\safe_readonly_exec.ps1 -RequestFile C:\Users\Usuario\source\repos\HaiKamDesu\BBCF-Improvement-Mod\tools\safe_readonly_request.json
```

Example request JSON (cdb read-only dump analysis):
```json
{
  "exe": "C:\\Program Files (x86)\\Windows Kits\\10\\Debuggers\\x86\\cdb.exe",
  "args": [
    "-z",
    "C:\\Users\\Usuario\\AppData\\Local\\CrashDumps\\BBCF.exe.12345.dmp",
    "-c",
    ".ecxr; !analyze -v; kb 120; r; q"
  ]
}
```
