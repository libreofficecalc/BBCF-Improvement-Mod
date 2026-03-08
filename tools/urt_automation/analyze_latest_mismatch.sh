#!/usr/bin/env bash
set -euo pipefail

GAME_DIR="${1:-/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction}"
BLOBS_DIR="${GAME_DIR}/BBCF_IM/URT_RE_BLOBS"
PY_SCRIPT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/analyze_mismatch_blob.py"

latest_meta="$(ls -1t "${BLOBS_DIR}"/mismatch_*_meta.txt 2>/dev/null | head -n 1 || true)"
if [[ -z "${latest_meta}" ]]; then
  echo "No mismatch meta files found in: ${BLOBS_DIR}" >&2
  exit 1
fi

base="${latest_meta%_meta.txt}"
seeded="${base}_seeded.bin"
entry="${base}_entry.bin"

if [[ ! -f "${seeded}" || ! -f "${entry}" ]]; then
  echo "Missing seeded/entry blob pair for: ${base}" >&2
  exit 1
fi

python3 "${PY_SCRIPT}" --seeded "${seeded}" --entry "${entry}"
