#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PS1_PATH="${ROOT_DIR}/Run-BbcfDebugCycle.ps1"

TIMEOUT_ARG=()
if [[ $# -ge 1 ]]; then
  TIMEOUT_ARG=(-TimeoutSec "$1")
fi

AHK_EXE_ARG=()
if [[ -n "${AHK_EXE:-}" ]]; then
  AHK_EXE_ARG=(-AhkExe "$(wslpath -w "${AHK_EXE}")")
fi

/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe \
  -ExecutionPolicy Bypass \
  -File "$(wslpath -w "${PS1_PATH}")" \
  "${AHK_EXE_ARG[@]}" \
  "${TIMEOUT_ARG[@]}"

if [[ "${URT_ANALYZE_MISMATCH:-1}" == "1" ]]; then
  "${ROOT_DIR}/analyze_latest_mismatch.sh" || true
fi
