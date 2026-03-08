param(
    [Parameter(Mandatory = $false, Position = 0)]
    [string]$Exe,

    [Parameter(Position = 1, ValueFromRemainingArguments = $true)]
    [string[]]$Args,

    [Parameter(Mandatory = $false)]
    [string]$RequestFile
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Normalize-Exe([string]$value) {
    if ([string]::IsNullOrWhiteSpace($value)) {
        return ''
    }
    $trimmed = $value.Trim().Trim('"')
    return $trimmed.ToLowerInvariant()
}

function Contains-Any([string]$text, [string[]]$needles) {
    if ([string]::IsNullOrEmpty($text)) {
        return $false
    }
    $lower = $text.ToLowerInvariant()
    foreach ($n in $needles) {
        if ($lower.Contains($n.ToLowerInvariant())) {
            return $true
        }
    }
    return $false
}

if (-not [string]::IsNullOrWhiteSpace($RequestFile)) {
    if (-not (Test-Path -LiteralPath $RequestFile)) {
        throw "safe_readonly_exec: request file not found: $RequestFile"
    }
    $requestRaw = Get-Content -LiteralPath $RequestFile -Raw -Encoding UTF8
    $request = $requestRaw | ConvertFrom-Json
    $Exe = [string]$request.exe
    if ($null -eq $request.args) {
        $Args = @()
    } else {
        $Args = @($request.args | ForEach-Object { [string]$_ })
    }
}

$exeNorm = Normalize-Exe $Exe
if ($exeNorm -eq '') {
    throw 'safe_readonly_exec: empty executable path (direct args or request file).'
}

# Strict allowlist: only commands known to be read-only in our RE workflow.
$allowedExe = @(
    'cdb.exe',
    'tasklist.exe',
    'where.exe',
    'findstr.exe',
    'cmd.exe'
)

$allowedExePathPatterns = @(
    '*\\windows kits\\10\\debuggers\\x86\\cdb.exe',
    '*/windows kits/10/debuggers/x86/cdb.exe',
    '*\\windows\\system32\\tasklist.exe',
    '*\\windows\\system32\\where.exe',
    '*\\windows\\system32\\findstr.exe',
    '*\\windows\\system32\\cmd.exe'
)

$basename = [System.IO.Path]::GetFileName($exeNorm)
$isAllowed = $false
if ($allowedExe -contains $basename) {
    $isAllowed = $true
}
if (-not $isAllowed) {
    foreach ($pattern in $allowedExePathPatterns) {
        if ($exeNorm -like $pattern) {
            $isAllowed = $true
            break
        }
    }
}
if (-not $isAllowed) {
    throw "safe_readonly_exec: executable not allowlisted: $Exe"
}

# Hard prohibition on destructive intents.
$joinedArgs = ($Args -join ' ')
$globalDanger = @(
    ' rm ', 'del ', 'erase ', ' rmdir ', ' rd ',
    ' move ', ' mv ', ' copy ', ' cp ', ' xcopy ', ' robocopy ',
    ' remove-item', ' set-content', ' add-content', ' out-file',
    ' ren ', ' rename-item', ' format ', ' reg add', ' reg delete',
    ' net stop', ' net start'
)
if (Contains-Any " $joinedArgs " $globalDanger) {
    throw 'safe_readonly_exec: rejected by destructive-token policy.'
}

# cdb-specific guardrails to keep this runner read-only.
if ($basename -eq 'cdb.exe') {
    $cdbDanger = @(
        '.writemem', '.dump', '.dumpcab', '.logopen',
        ' eb ', ' ed ', ' eq ', ' ea ',
        ' f ', ' fp ', ' fm '
    )
    if (Contains-Any " $joinedArgs " $cdbDanger) {
        throw 'safe_readonly_exec: rejected by cdb write-command policy.'
    }
}

Write-Host "[safe_readonly_exec] exe=$Exe"
if ($Args.Count -gt 0) {
    Write-Host ("[safe_readonly_exec] args=" + ($Args -join ' '))
}

$proc = Start-Process -FilePath $Exe -ArgumentList $Args -NoNewWindow -Wait -PassThru
exit $proc.ExitCode
