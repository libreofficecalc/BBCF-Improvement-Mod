#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GAME_DIR_DEFAULT="/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction"
GAME_DIR="${GAME_DIR_DEFAULT}"
DO_BUILD=1

for arg in "$@"; do
  case "$arg" in
    --no-build)
      DO_BUILD=0
      ;;
    *)
      GAME_DIR="$arg"
      ;;
  esac
done

MSBUILD="/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe"
if [[ "$DO_BUILD" -eq 1 ]]; then
  (
    cd "${ROOT_DIR}"
    /bin/bash -lc "\"${MSBUILD}\" BBCF_IM.sln /m /p:Configuration=DebugDeploy /p:Platform=Win32 /p:PlatformToolset=v143 /p:BbcfDeployEnabled=false /p:BbcfLaunchAfterDeploy=false"
  )
fi

if [[ "$DO_BUILD" -eq 1 ]]; then
  SRC_DLL="${ROOT_DIR}/bin/DebugDeploy/dinput8.dll"
  SRC_SETTINGS="${ROOT_DIR}/bin/DebugDeploy/settings.ini"
else
  SRC_DLL="${ROOT_DIR}/bin/Debug/dinput8.dll"
  SRC_SETTINGS="${ROOT_DIR}/bin/Debug/settings.ini"
fi
DEST_DLL="${GAME_DIR}/dinput8.dll"
DEST_SETTINGS="${GAME_DIR}/settings.ini"
BACKUP_DIR="${GAME_DIR}/BBCF_IM/SettingsBackups"

if [[ ! -f "${SRC_DLL}" ]]; then
  echo "Missing built DLL: ${SRC_DLL}" >&2
  exit 1
fi
if [[ ! -f "${SRC_SETTINGS}" ]]; then
  echo "Missing built settings template: ${SRC_SETTINGS}" >&2
  exit 1
fi

mkdir -p "${BACKUP_DIR}"
timestamp="$(date +%Y%m%d_%H%M%S)"
backup_settings="${BACKUP_DIR}/settings_${timestamp}.ini.bak"

if [[ -f "${DEST_SETTINGS}" ]]; then
  cp -f "${DEST_SETTINGS}" "${backup_settings}"
fi

cp -f "${SRC_DLL}" "${DEST_DLL}"
cp -f "${SRC_SETTINGS}" "${DEST_SETTINGS}"

if [[ -f "${backup_settings}" ]]; then
  awk -v defaults="${SRC_SETTINGS}" -v prev="${backup_settings}" '
    function trim(s) { gsub(/^[ \t\r\n]+|[ \t\r\n]+$/, "", s); return s }
    BEGIN {
      while ((getline line < defaults) > 0) {
        if (line ~ /^[ \t]*#/ || line ~ /^[ \t]*$/ || line !~ /=/) continue
        split(line, a, "=")
        k = trim(a[1])
        v = trim(substr(line, index(line, "=") + 1))
        def[k] = v
      }
      close(defaults)
      while ((getline line < prev) > 0) {
        if (line ~ /^[ \t]*#/ || line ~ /^[ \t]*$/ || line !~ /=/) continue
        split(line, a, "=")
        k = trim(a[1])
        v = trim(substr(line, index(line, "=") + 1))
        prv[k] = v
      }
      close(prev)
      for (k in prv) {
        if (!(k in def) || prv[k] != def[k]) {
          keep[k] = prv[k]
        }
      }
    }
    {
      if ($0 ~ /^[ \t]*#/ || $0 !~ /=/) { print; next }
      split($0, a, "=")
      key = trim(a[1])
      if (key in keep) {
        printf "%s = %s\n", key, keep[key]
        seen[key] = 1
      } else {
        print
      }
    }
    END {
      for (k in keep) {
        if (!(k in seen) && !(k in def)) {
          printf "%s = %s\n", k, keep[k]
        }
      }
    }
  ' "${DEST_SETTINGS}" > "${DEST_SETTINGS}.tmp"
  mv "${DEST_SETTINGS}.tmp" "${DEST_SETTINGS}"
fi

set_or_add_key() {
  local key="$1"
  local value="$2"
  local file="$3"
  if grep -Eq "^${key}[[:space:]]*=" "$file"; then
    sed -E -i "s|^${key}[[:space:]]*=.*|${key} = ${value}|" "$file"
  else
    printf "%s = %s\n" "$key" "$value" >> "$file"
  fi
}

# RE trace defaults for URT reverse-engineering sessions.
set_or_add_key "GenerateDebugLogs" "1" "${DEST_SETTINGS}"
set_or_add_key "URT_RE_TraceEnabled" "1" "${DEST_SETTINGS}"
set_or_add_key "URT_RE_TraceLevel" "2" "${DEST_SETTINGS}"
set_or_add_key "URT_RE_MaxFileMB" "32" "${DEST_SETTINGS}"
set_or_add_key "URT_RE_MaxBackups" "5" "${DEST_SETTINGS}"
set_or_add_key "URT_RE_AllowSizeMismatchProbe" "1" "${DEST_SETTINGS}"
set_or_add_key "URT_RE_AllowUnsafeProbeLoad" "0" "${DEST_SETTINGS}"

echo "Deploy complete."
echo "Game dir: ${GAME_DIR}"
echo "DLL: ${DEST_DLL}"
echo "Settings: ${DEST_SETTINGS}"
if [[ -f "${backup_settings}" ]]; then
  echo "Backup: ${backup_settings}"
fi
