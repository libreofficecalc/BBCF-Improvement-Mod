[CmdletBinding()]
param(
    [string]$GameExe = "C:\\SteamLibrary\\steamapps\\common\\BlazBlue Centralfiction\\BBCF.exe",
    [string]$AhkScript = "",
    [string]$AhkExe = "",
    [int]$TimeoutSec = 240,
    [switch]$LaunchGame,
    [switch]$CloseGameOnDone
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
if ([string]::IsNullOrWhiteSpace($AhkScript)) {
    $AhkScript = Join-Path $scriptRoot "urt_macro_template.ahk"
}

if (-not (Test-Path -LiteralPath $AhkScript)) {
    throw "AHK script not found: $AhkScript"
}

function Resolve-AhkExe {
    param([string]$Configured)

    if (-not [string]::IsNullOrWhiteSpace($Configured)) {
        if (-not (Test-Path -LiteralPath $Configured)) {
            throw "Configured AutoHotkey executable not found: $Configured"
        }
        return $Configured
    }

    $candidates = @(
        "C:\\Program Files\\AutoHotkey\\v2\\AutoHotkey64.exe",
        "C:\\Program Files\\AutoHotkey\\AutoHotkey64.exe",
        "C:\\Program Files\\AutoHotkey\\AutoHotkey.exe",
        "C:\\Program Files (x86)\\AutoHotkey\\AutoHotkey.exe"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }

    throw "AutoHotkey executable not found. Pass -AhkExe explicitly."
}

$ahkExeResolved = Resolve-AhkExe -Configured $AhkExe

$signalDir = Join-Path $scriptRoot "runtime"
if (-not (Test-Path -LiteralPath $signalDir)) {
    New-Item -ItemType Directory -Path $signalDir | Out-Null
}

$runId = Get-Date -Format "yyyyMMdd_HHmmss"
$signalFile = Join-Path $signalDir ("urt_run_" + $runId + "_signal.txt")
$resultFile = Join-Path $signalDir ("urt_run_" + $runId + "_result.txt")
$runnerLog = Join-Path $signalDir ("urt_run_" + $runId + "_runner.log")

"[$(Get-Date -Format o)] run_id=$runId" | Out-File -FilePath $runnerLog -Encoding utf8 -Append
"[$(Get-Date -Format o)] signal_file=$signalFile" | Out-File -FilePath $runnerLog -Encoding utf8 -Append
"[$(Get-Date -Format o)] result_file=$resultFile" | Out-File -FilePath $runnerLog -Encoding utf8 -Append

$gameProcess = $null
if ($LaunchGame.IsPresent) {
    if (-not (Test-Path -LiteralPath $GameExe)) {
        throw "Game executable not found: $GameExe"
    }

    $gameProcess = Start-Process -FilePath $GameExe -PassThru
    "[$(Get-Date -Format o)] launched_game_pid=$($gameProcess.Id)" | Out-File -FilePath $runnerLog -Encoding utf8 -Append
}

$ahkArgs = @(
    $AhkScript,
    "/signal=$signalFile",
    "/result=$resultFile"
)

$ahkProcess = Start-Process -FilePath $ahkExeResolved -ArgumentList $ahkArgs -PassThru
"[$(Get-Date -Format o)] launched_ahk_pid=$($ahkProcess.Id)" | Out-File -FilePath $runnerLog -Encoding utf8 -Append

$deadline = (Get-Date).AddSeconds($TimeoutSec)
$status = "UNKNOWN"
$detail = ""

while ((Get-Date) -lt $deadline) {
    Start-Sleep -Milliseconds 300

    if (Test-Path -LiteralPath $resultFile) {
        $lines = Get-Content -LiteralPath $resultFile
        if ($lines.Count -gt 0) {
            $status = $lines[0].Trim()
            if ($lines.Count -gt 1) {
                $detail = ($lines[1..($lines.Count - 1)] -join " ").Trim()
            }
        } else {
            $status = "RESULT_FILE_EMPTY"
        }
        break
    }

    if ($ahkProcess.HasExited) {
        $status = "AHK_EXITED_WITHOUT_RESULT"
        $detail = "exit_code=$($ahkProcess.ExitCode)"
        break
    }

    $bbcfrunning = Get-Process -Name "BBCF" -ErrorAction SilentlyContinue
    if (-not $bbcfrunning) {
        $status = "BBCF_PROCESS_NOT_RUNNING"
        $detail = "game closed/crashed before AHK completion"
        break
    }
}

if ($status -eq "UNKNOWN") {
    $status = "TIMEOUT"
    $detail = "runner timed out after $TimeoutSec sec"
}

"[$(Get-Date -Format o)] status=$status detail=$detail" | Out-File -FilePath $runnerLog -Encoding utf8 -Append

if ($CloseGameOnDone.IsPresent -and $status -eq "DONE") {
    Get-Process -Name "BBCF" -ErrorAction SilentlyContinue | Stop-Process -Force
    "[$(Get-Date -Format o)] close_game_on_done=true" | Out-File -FilePath $runnerLog -Encoding utf8 -Append
}

Write-Host "AUTOMATION_STATUS: $status"
if (-not [string]::IsNullOrWhiteSpace($detail)) {
    Write-Host "AUTOMATION_DETAIL: $detail"
}
Write-Host "AUTOMATION_RUNNER_LOG: $runnerLog"
Write-Host "AUTOMATION_SIGNAL_FILE: $signalFile"
Write-Host "AUTOMATION_RESULT_FILE: $resultFile"

switch ($status) {
    "DONE" { exit 0 }
    "SCRIPT_REPORTED_FAILURE" { exit 13 }
    "TIMEOUT" { exit 10 }
    "AHK_EXITED_WITHOUT_RESULT" { exit 11 }
    "BBCF_PROCESS_NOT_RUNNING" { exit 12 }
    default { exit 14 }
}
