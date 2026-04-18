#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GAME_DIR_DEFAULT="/mnt/d/SteamLibrary/steamapps/common/BlazBlue Centralfiction"
GAME_DIR="${GAME_DIR_DEFAULT}"
DO_BUILD=1
WAIT_FOR_RESULT=1
QUIT_ON_FINISH=1
TIMEOUT_SECONDS=300
PRESERVE_GAME_ON_EXIT=0

for arg in "$@"; do
  case "$arg" in
    --no-build)
      DO_BUILD=0
      ;;
    --no-wait)
      WAIT_FOR_RESULT=0
      ;;
    --keep-open)
      QUIT_ON_FINISH=0
      PRESERVE_GAME_ON_EXIT=1
      ;;
    --timeout=*)
      TIMEOUT_SECONDS="${arg#*=}"
      ;;
    *)
      GAME_DIR="$arg"
      ;;
  esac
done

DEPLOY_ARGS=()
if [[ "${DO_BUILD}" -eq 0 ]]; then
  DEPLOY_ARGS+=(--no-build)
fi
DEPLOY_ARGS+=("${GAME_DIR}")

DEPLOY_SH="${ROOT_DIR}/tools/deploy_debugdeploy_to_bbcf.sh"
DEPLOY_TMP="$(mktemp "${ROOT_DIR}/tools/deploy_debugdeploy_to_bbcf.XXXXXX.sh")"
cleanup() {
  local exit_code=$?
  rm -f "${DEPLOY_TMP}"
  if [[ -n "${AUTORUN_TOKEN_PATH:-}" ]]; then
    rm -f "${AUTORUN_TOKEN_PATH}"
  fi
  if [[ "${PRESERVE_GAME_ON_EXIT}" -eq 0 ]]; then
    /mnt/c/Windows/System32/taskkill.exe /IM BBCF.exe /T /F >/dev/null 2>&1 || true
  fi
  exit "${exit_code}"
}
trap cleanup EXIT
tr -d '\r' < "${DEPLOY_SH}" > "${DEPLOY_TMP}"
bash "${DEPLOY_TMP}" "${DEPLOY_ARGS[@]}"

SETTINGS_PATH="${GAME_DIR}/settings.ini"
LOG_PATH="${GAME_DIR}/BBCF_IM/DEBUG.txt"
AUTORUN_TOKEN_PATH="${GAME_DIR}/BBCF_IM/ranked_harness_autorun.token"

set_or_add_key() {
  local key="$1"
  local value="$2"
  local file="$3"
  if grep -Eq "^${key}[[:space:]]*=" "$file"; then
    sed -E -i "s|^${key}[[:space:]]*=.*|${key} = ${value}|" "$file"
  else
    printf "%s = %s\n" "${key}" "${value}" >> "${file}"
  fi
}

set_or_add_key "GenerateDebugLogs" "1" "${SETTINGS_PATH}"
set_or_add_key "EnableInDevelopmentFeatures" "1" "${SETTINGS_PATH}"
set_or_add_key "RankedAutomationHarnessEnabled" "1" "${SETTINGS_PATH}"
set_or_add_key "RankedAutomationHarnessAutorun" "1" "${SETTINGS_PATH}"
set_or_add_key "RankedAutomationHarnessQuitOnFinish" "${QUIT_ON_FINISH}" "${SETTINGS_PATH}"

bbcf_running() {
  /mnt/c/Windows/System32/tasklist.exe //FI "IMAGENAME eq BBCF.exe" 2>/dev/null | grep -Fq "BBCF.exe"
}

print_last_ranked_log_lines() {
  local source_log=""
  if [[ -f "${LOG_PATH}" ]]; then
    source_log="$(tail -n +"$((baseline_lines + 1))" "${LOG_PATH}")"
  fi

  if [[ -n "${source_log}" ]] && grep -Fq "[RankedAuto]" <<<"${source_log}"; then
    echo "Last ranked automation log lines:"
    grep -F "[RankedAuto]" <<<"${source_log}" | tail -n 20
    return
  fi

  if [[ -f "${LOG_PATH}" ]]; then
    echo "Last DEBUG.txt lines:"
    tail -n 20 "${LOG_PATH}"
  fi
}

mkdir -p "$(dirname "${LOG_PATH}")"
printf "autorun\n" > "${AUTORUN_TOKEN_PATH}"

baseline_lines=0
if [[ -f "${LOG_PATH}" ]]; then
  baseline_lines="$(wc -l < "${LOG_PATH}")"
fi

echo "Launching BBCF with ranked automation autorun enabled..."
/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "Start-Process 'C:\Program Files (x86)\Steam\steam.exe' -ArgumentList '-applaunch 586140'" >/dev/null

echo "Settings: ${SETTINGS_PATH}"
echo "Log: ${LOG_PATH}"

if [[ "${WAIT_FOR_RESULT}" -eq 0 ]]; then
  echo "Autorun launched; not waiting for result."
  exit 0
fi

deadline=$(( $(date +%s) + TIMEOUT_SECONDS ))
seen_bbcf_process=0
while [[ "$(date +%s)" -lt "${deadline}" ]]; do
  if bbcf_running; then
    seen_bbcf_process=1
  elif [[ "${seen_bbcf_process}" -eq 1 ]]; then
    echo "BBCF.exe exited before ranked automation sentinel." >&2
    print_last_ranked_log_lines >&2
    exit 3
  fi

  if [[ -f "${LOG_PATH}" ]]; then
    start_line=$(( baseline_lines + 1 ))
    new_log="$(tail -n +"${start_line}" "${LOG_PATH}")"
    if grep -Fq "[RankedAuto] COMPLETED" <<<"${new_log}"; then
      grep -F "[RankedAuto] COMPLETED" <<<"${new_log}" | tail -n 1
      exit 0
    fi
    if grep -Fq "[RankedAuto] FAILED" <<<"${new_log}"; then
      grep -F "[RankedAuto] FAILED" <<<"${new_log}" | tail -n 1 >&2
      exit 1
    fi
  fi
  sleep 1
done

echo "Timed out waiting for ranked automation sentinel in ${LOG_PATH}" >&2
print_last_ranked_log_lines >&2
exit 2
