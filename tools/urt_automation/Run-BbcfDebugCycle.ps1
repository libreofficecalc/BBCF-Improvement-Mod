[CmdletBinding()]
param(
    [string]$AhkScript = "",
    [string]$AhkExe = "",
    [int]$TimeoutSec = 900,
    [int]$PostAhkMonitorSec = 120,
    [int]$PostExitGraceSec = 12,
    [int]$UnresponsiveKillSec = 12,
    [int]$LogStallKillSec = 25
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
if ([string]::IsNullOrWhiteSpace($AhkScript)) {
    $AhkScript = Join-Path $repoRoot "tools\urt_automation\BBCF-Automatic-Debugger.ahk"
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
        "C:\Program Files\AutoHotkey\v2\AutoHotkey64.exe",
        "C:\Program Files\AutoHotkey\AutoHotkey64.exe",
        "C:\Program Files\AutoHotkey\AutoHotkeyU64.exe",
        "C:\Program Files\AutoHotkey\v2\AutoHotkey.exe",
        "C:\Program Files\AutoHotkey\AutoHotkey.exe",
        "C:\Program Files (x86)\AutoHotkey\AutoHotkeyU64.exe",
        "C:\Program Files (x86)\AutoHotkey\AutoHotkey.exe",
        "C:\Users\Usuario\AppData\Local\Programs\AutoHotkey\v2\AutoHotkey64.exe",
        "C:\Users\Usuario\AppData\Local\Programs\AutoHotkey\AutoHotkey64.exe",
        "C:\Users\Usuario\AppData\Local\Programs\AutoHotkey\AutoHotkeyU64.exe",
        "C:\Users\Usuario\AppData\Local\Programs\AutoHotkey\AutoHotkey.exe",
        "C:\tools\AutoHotkey\AutoHotkey64.exe",
        "C:\tools\AutoHotkey\AutoHotkeyU64.exe",
        "C:\tools\AutoHotkey\AutoHotkey.exe"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }

    $pathHit = $null
    foreach ($name in @("AutoHotkey64.exe", "AutoHotkeyU64.exe", "AutoHotkey.exe")) {
        $cmd = Get-Command $name -ErrorAction SilentlyContinue
        if ($cmd -and $cmd.Source -and (Test-Path -LiteralPath $cmd.Source)) {
            $pathHit = $cmd.Source
            break
        }
    }
    if ($pathHit) {
        return $pathHit
    }

    throw "AutoHotkey executable not found. Pass -AhkExe explicitly (or set AHK_EXE in run_bbcf_debug_cycle.sh)."
}

$resolvedAhkExe = $null
$resolveError = $null
try {
    $resolvedAhkExe = Resolve-AhkExe -Configured $AhkExe
} catch {
    $resolveError = $_
}
$launchViaAssociation = $false
if (-not $resolvedAhkExe) {
    try {
        $assoc = & "$env:SystemRoot\System32\cmd.exe" /c "assoc .ahk" 2>$null
        if ($assoc -match "AutoHotkeyScript") {
            $launchViaAssociation = $true
        }
    } catch {
    }
}
if (-not $resolvedAhkExe -and -not $launchViaAssociation) {
    if ($resolveError) {
        throw $resolveError
    }
    throw "No AutoHotkey executable or .ahk association found."
}

function Ensure-BbcfStoppedBeforeCycle {
    $existing = @(Get-Process -Name "BBCF" -ErrorAction SilentlyContinue)
    Write-Host ("DEBUG_CYCLE: preflight_existing_bbcf_count={0}" -f $existing.Count)
    if ($existing.Count -eq 0) {
        return
    }

    Write-Host "DEBUG_CYCLE: preflight_close_attempt=Stop-Process"
    try {
        $existing | Stop-Process -Force -ErrorAction Stop
    } catch {
    }
    Start-Sleep -Milliseconds 400
    $remaining = @(Get-Process -Name "BBCF" -ErrorAction SilentlyContinue)
    if ($remaining.Count -eq 0) {
        Write-Host "DEBUG_CYCLE: preflight_bbcf_closed=true (Stop-Process)"
        return
    }

    Write-Host "DEBUG_CYCLE: preflight_close_attempt=taskkill"
    try {
        & "$env:SystemRoot\System32\taskkill.exe" /IM BBCF.exe /T /F | Out-Null
    } catch {
    }
    Start-Sleep -Milliseconds 500
    $remaining = @(Get-Process -Name "BBCF" -ErrorAction SilentlyContinue)
    if ($remaining.Count -eq 0) {
        Write-Host "DEBUG_CYCLE: preflight_bbcf_closed=true (taskkill)"
        return
    }

    Write-Host ("DEBUG_CYCLE: preflight_bbcf_closed=false remaining_count={0}" -f $remaining.Count)
    throw "Cannot start debug cycle: BBCF.exe is already running and could not be closed."
}

Ensure-BbcfStoppedBeforeCycle

Write-Host "DEBUG_CYCLE: starting AHK script"
if ($launchViaAssociation) {
    Write-Host "DEBUG_CYCLE: ahk_launch=windows_file_association"
} else {
    Write-Host "DEBUG_CYCLE: ahk_exe=$resolvedAhkExe"
}
Write-Host "DEBUG_CYCLE: ahk_script=$AhkScript"
$cycleStartUtc = [datetime]::UtcNow

if ($launchViaAssociation) {
    $ahkProc = Start-Process -FilePath $AhkScript -PassThru
} else {
    $ahkProc = Start-Process -FilePath $resolvedAhkExe -ArgumentList @($AhkScript) -PassThru
}

function Resolve-DebugPaths {
    $candidates = @(
        "D:\SteamLibrary\steamapps\common\BlazBlue Centralfiction\BBCF_IM",
        "C:\SteamLibrary\steamapps\common\BlazBlue Centralfiction\BBCF_IM"
    )
    foreach ($base in $candidates) {
        if (Test-Path -LiteralPath $base) {
            return @{
                Base = $base
                Debug = Join-Path $base "DEBUG.txt"
                CrashRoot = Join-Path $base "CrashReports"
            }
        }
    }
    return @{
        Base = ""
        Debug = ""
        CrashRoot = ""
    }
}

$paths = Resolve-DebugPaths
$debugPath = $paths.Debug
$cycleDeadline = (Get-Date).AddSeconds([Math]::Max(30, $TimeoutSec))
$ahkExit = $null
$sawBbcfSpawnedDuringAhk = $false
while ((Get-Date) -lt $cycleDeadline) {
    if ($ahkProc.HasExited) {
        $ahkExit = $ahkProc.ExitCode
        break
    }

    $bbcfAlive = Get-Process -Name "BBCF" -ErrorAction SilentlyContinue
    if ($bbcfAlive) {
        $sawBbcfSpawnedDuringAhk = $true
    }
    if (-not $bbcfAlive -and $sawBbcfSpawnedDuringAhk -and $debugPath -and (Test-Path -LiteralPath $debugPath)) {
        $tail = Get-Content -LiteralPath $debugPath -Tail 4000 -ErrorAction SilentlyContinue
        $tailText = ($tail -join "`n")
        $sawRecordingSavedEarly = $tailText -match "\[URT\] StopRecordingAndSave success"
        $sawPlayAttemptEarly = $tailText -match "\[URT\] PlayEntryByIndex idx="
        $sawCrashEarly = ($tailText -match "\[Crash\] UnhandledExFilter invoked\." -or $tailText -match "\[Crash\] WriteCrashBundle invoked")
        Write-Host ("DEBUG_CYCLE: ahk_force_stop_on_bbcf_exit recording_saved={0} play_attempt={1} crash_log={2}" -f `
            ([int]$sawRecordingSavedEarly), ([int]$sawPlayAttemptEarly), ([int]$sawCrashEarly))
        try {
            Stop-Process -Id $ahkProc.Id -Force -ErrorAction SilentlyContinue
        } catch {
        }
        Start-Sleep -Milliseconds 200
        $ahkExit = 0
        break
    }

    Start-Sleep -Milliseconds 500
}

if ($null -eq $ahkExit) {
    Write-Host "DEBUG_CYCLE_STATUS: TIMEOUT"
    Write-Host "DEBUG_CYCLE_DETAIL: AHK did not exit within $TimeoutSec seconds"
    try {
        Stop-Process -Id $ahkProc.Id -Force -ErrorAction SilentlyContinue
    } catch {
    }
    exit 10
}

Write-Host "DEBUG_CYCLE: ahk_exit_code=$ahkExit"

if ($ahkExit -eq 0) {
    Write-Host "DEBUG_CYCLE: entering_post_ahk_monitor=true"
} else {
    Write-Host "DEBUG_CYCLE_STATUS: AHK_FAILED"
    Write-Host "DEBUG_CYCLE_DETAIL: AHK exit code was $ahkExit"
    exit 11
}

function Capture-BbcfHangSnapshot {
    param(
        [Parameter(Mandatory = $true)]
        [int]$TargetPid,
        [Parameter(Mandatory = $true)]
        [string]$BbcfImDir
    )

    $cdb = "C:\Program Files (x86)\Windows Kits\10\Debuggers\x86\cdb.exe"
    if (-not (Test-Path -LiteralPath $cdb)) {
        Write-Host "DEBUG_CYCLE: hang_capture=skipped (cdb missing)"
        return
    }

    $hangDir = Join-Path $BbcfImDir "URT_RE_HANG"
    New-Item -ItemType Directory -Path $hangDir -Force | Out-Null
    $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $outFile = Join-Path $hangDir ("hang_{0}_pid{1}.txt" -f $stamp, $TargetPid)

    try {
        $cmd = ".ecxr; ~* kb 60; !uniqstack -pn; q"
        & $cdb -p $TargetPid -c $cmd *> $outFile
        Write-Host ("DEBUG_CYCLE: hang_capture=ok file={0}" -f $outFile)
    } catch {
        Write-Host ("DEBUG_CYCLE: hang_capture=failed ({0})" -f $_.Exception.Message)
    }
}

function Get-NewestCrashFolder {
    param(
        [string]$CrashRoot,
        [datetime]$SinceUtc
    )
    if ([string]::IsNullOrWhiteSpace($CrashRoot) -or -not (Test-Path -LiteralPath $CrashRoot)) {
        return $null
    }
    $folders = Get-ChildItem -LiteralPath $CrashRoot -Directory -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTimeUtc -Descending
    foreach ($f in $folders) {
        if ($f.LastWriteTimeUtc -ge $SinceUtc) {
            return $f
        }
    }
    return $null
}

function Get-DebugTail {
    param([string]$Path, [int]$Lines = 200)
    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        return @()
    }
    try {
        return Get-Content -LiteralPath $Path -Tail $Lines -ErrorAction Stop
    } catch {
        return @()
    }
}

function Close-BbcfProcess {
    $bbcfs = Get-Process -Name "BBCF" -ErrorAction SilentlyContinue
    if (-not $bbcfs) {
        Write-Host "DEBUG_CYCLE: bbcf_process_closed=false (not running)"
        return
    }
    try {
        $bbcfs | Stop-Process -Force -ErrorAction Stop
        Write-Host "DEBUG_CYCLE: bbcf_process_closed=true (Stop-Process)"
        return
    } catch {
    }
    try {
        & "$env:SystemRoot\System32\taskkill.exe" /IM BBCF.exe /T /F | Out-Null
        Start-Sleep -Milliseconds 300
        if (-not (Get-Process -Name "BBCF" -ErrorAction SilentlyContinue)) {
            Write-Host "DEBUG_CYCLE: bbcf_process_closed=true (taskkill)"
            return
        }
        Write-Host "DEBUG_CYCLE: bbcf_process_closed=failed (taskkill did not terminate)"
    } catch {
        Write-Host "DEBUG_CYCLE: bbcf_process_closed=failed ($($_.Exception.Message))"
    }
}

$crashRoot = $paths.CrashRoot
$monitorStartUtc = $cycleStartUtc
$monitorDeadline = (Get-Date).AddSeconds([Math]::Max(5, $PostAhkMonitorSec))
$lastDebugWriteUtc = $null
if ($debugPath -and (Test-Path -LiteralPath $debugPath)) {
    $lastDebugWriteUtc = (Get-Item -LiteralPath $debugPath).LastWriteTimeUtc
}

$sawSnapshotLoaded = $false
$sawPlaybackStarted = $false
$sawSetupDelayArmed = $false
$sawSetupDelayExpired = $false
$sawCrashLog = $false
$sawSanityCancel = $false
$sawRecordingSaved = $false
$sawPlayAttempt = $false
$sawEntryStart = $false
$detectedCrashFolder = $null
$monitorResult = "timeout"
$unresponsiveStreakSec = 0
$bbcfExitObservedUtc = $null

while ((Get-Date) -lt $monitorDeadline) {
    $tail = Get-DebugTail -Path $debugPath -Lines 4000
    $tailText = ($tail -join "`n")
    if ($tailText -match "\[URT\] Snapshot load succeeded") { $sawSnapshotLoaded = $true }
    if ($tailText -match "\[URT\] Playback started on runtime slot") { $sawPlaybackStarted = $true }
    if ($tailText -match "\[URT\] Setup delay armed") { $sawSetupDelayArmed = $true }
    if ($tailText -match "\[URT\] Setup delay expired; enabling playback now") { $sawSetupDelayExpired = $true }
    if ($tailText -match "Snapshot post-load sanity failed; playback cancelled") { $sawSanityCancel = $true }
    if ($tailText -match "\[URT\] StopRecordingAndSave success") { $sawRecordingSaved = $true }
    if ($tailText -match "\[URT\] PlayEntryByIndex idx=") { $sawPlayAttempt = $true }
    if ($tailText -match "\[URT\] StartEntryByIndex entry id=") { $sawEntryStart = $true }
    if ($tailText -match "\[Crash\] UnhandledExFilter invoked\." -or $tailText -match "\[Crash\] WriteCrashBundle invoked") {
        $sawCrashLog = $true
    }

    if ($debugPath -and (Test-Path -LiteralPath $debugPath)) {
        $lastDebugWriteUtc = (Get-Item -LiteralPath $debugPath).LastWriteTimeUtc
    }

    $newCrashFolder = Get-NewestCrashFolder -CrashRoot $crashRoot -SinceUtc $monitorStartUtc
    if ($newCrashFolder) {
        $detectedCrashFolder = $newCrashFolder.FullName
        $monitorResult = "crash_folder_detected"
        break
    }

    if ($sawCrashLog) {
        $monitorResult = "crash_log_detected"
        break
    }
    if ($sawSanityCancel) {
        $monitorResult = "sanity_cancelled"
        break
    }

    $bbcfAlive = Get-Process -Name "BBCF" -ErrorAction SilentlyContinue
    if (-not $bbcfAlive) {
        if (-not $bbcfExitObservedUtc) {
            $bbcfExitObservedUtc = [datetime]::UtcNow
            Write-Host ("DEBUG_CYCLE: bbcf_exit_observed play_attempt={0} entry_start={1} snapshot_loaded={2}" -f `
                ([int]$sawPlayAttempt), ([int]$sawEntryStart), ([int]$sawSnapshotLoaded))
        }
        $exitAgeSec = [int]([datetime]::UtcNow - $bbcfExitObservedUtc).TotalSeconds
        if ($exitAgeSec -ge [Math]::Max(3, $PostExitGraceSec)) {
            $monitorResult = "bbcf_exited"
            break
        }
        Start-Sleep -Seconds 1
        continue
    } else {
        $bbcfExitObservedUtc = $null
    }

    $bbcfPrimary = $bbcfAlive | Select-Object -First 1
    if ($bbcfPrimary -and ($bbcfPrimary.MainWindowHandle -ne 0)) {
        if (-not $bbcfPrimary.Responding) {
            $unresponsiveStreakSec += 1
        } else {
            $unresponsiveStreakSec = 0
        }
    } else {
        $unresponsiveStreakSec = 0
    }
    if ($unresponsiveStreakSec -ge [Math]::Max(3, $UnresponsiveKillSec)) {
        $monitorResult = "bbcf_not_responding"
        break
    }

    if ($lastDebugWriteUtc -and $sawSnapshotLoaded -and $sawSetupDelayArmed) {
        $stallNow = [int]([datetime]::UtcNow - $lastDebugWriteUtc).TotalSeconds
        if ($stallNow -ge [Math]::Max(5, $LogStallKillSec)) {
            $monitorResult = "bbcf_log_stall_timeout"
            break
        }
    }

    Start-Sleep -Seconds 1
}

$logStallSec = -1
if ($lastDebugWriteUtc) {
    $logStallSec = [int]([datetime]::UtcNow - $lastDebugWriteUtc).TotalSeconds
}

if ($monitorResult -eq "bbcf_exited" -and $sawSnapshotLoaded -and $sawPlaybackStarted -and -not $sawSanityCancel -and -not $sawCrashLog) {
    $monitorResult = "probable_crash_after_playback_start"
}
if ($monitorResult -eq "bbcf_exited" -and ($sawPlayAttempt -or $sawEntryStart) -and -not $sawSnapshotLoaded -and -not $sawCrashLog) {
    $monitorResult = "probable_crash_after_play_attempt"
}
if ($monitorResult -eq "bbcf_exited" -and $sawRecordingSaved -and -not $sawPlayAttempt -and -not $sawEntryStart) {
    $monitorResult = "automation_desync_no_play_attempt"
}
if ($monitorResult -eq "bbcf_exited" -and -not $sawRecordingSaved -and -not $sawPlayAttempt -and -not $sawEntryStart) {
    $monitorResult = "automation_desync_no_urt_flow"
}
if (($monitorResult -eq "timeout" -or $monitorResult -eq "bbcf_not_responding" -or $monitorResult -eq "bbcf_log_stall_timeout") -and
    ($sawPlayAttempt -or $sawEntryStart) -and -not $sawSnapshotLoaded) {
    $monitorResult = "hang_after_play_attempt"
}
if (($monitorResult -eq "timeout" -or $monitorResult -eq "bbcf_not_responding" -or $monitorResult -eq "bbcf_log_stall_timeout") -and
    $sawSnapshotLoaded -and $sawSetupDelayArmed -and -not $sawPlaybackStarted) {
    $monitorResult = "hang_after_snapshot_load"
}
if ($sawSnapshotLoaded -and $sawSetupDelayArmed -and $sawSetupDelayExpired -and $sawPlaybackStarted -and -not $sawCrashLog -and -not $sawSanityCancel) {
    $monitorResult = "full_chain_success"
}

Write-Host ("DEBUG_CYCLE: post_monitor_result={0}" -f $monitorResult)
Write-Host ("DEBUG_CYCLE: post_monitor_flags snapshot_loaded={0} delay_armed={1} delay_expired={2} playback_started={3} sanity_cancel={4} crash_log={5}" -f `
    ([int]$sawSnapshotLoaded), ([int]$sawSetupDelayArmed), ([int]$sawSetupDelayExpired), ([int]$sawPlaybackStarted), ([int]$sawSanityCancel), ([int]$sawCrashLog))
Write-Host ("DEBUG_CYCLE: post_monitor_urt_flow recording_saved={0} play_attempt={1} entry_start={2}" -f `
    ([int]$sawRecordingSaved), ([int]$sawPlayAttempt), ([int]$sawEntryStart))
Write-Host ("DEBUG_CYCLE: post_monitor_unresponsive_streak_sec={0}" -f $unresponsiveStreakSec)
if ($logStallSec -ge 0) {
    Write-Host ("DEBUG_CYCLE: post_monitor_debug_log_stall_sec={0}" -f $logStallSec)
}
if ($detectedCrashFolder) {
    Write-Host ("DEBUG_CYCLE: crash_folder={0}" -f $detectedCrashFolder)
}

if ($monitorResult -eq "timeout" -or $monitorResult -eq "bbcf_not_responding" -or $monitorResult -eq "bbcf_log_stall_timeout") {
    $live = Get-Process -Name "BBCF" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($live -and -not [string]::IsNullOrWhiteSpace($paths.Base)) {
        Capture-BbcfHangSnapshot -TargetPid $live.Id -BbcfImDir $paths.Base
    }
}

Close-BbcfProcess
if ($monitorResult -eq "full_chain_success") {
    Write-Host "DEBUG_CYCLE_STATUS: DONE"
    exit 0
}
if ($monitorResult -eq "automation_desync_no_play_attempt" -or $monitorResult -eq "automation_desync_no_urt_flow") {
    Write-Host "DEBUG_CYCLE_STATUS: AUTOMATION_DESYNC"
    exit 15
}
Write-Host "DEBUG_CYCLE_STATUS: DONE"
exit 0
