param(
    [string]$Tag = "",
    [string]$Configuration = "Release",
    [string]$OutputDir = "",
    [ValidateSet("stable", "prerelease")][string]$Channel = "stable"
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")

if ([string]::IsNullOrWhiteSpace($Tag)) {
    $infoPath = Join-Path $repoRoot "src\Core\info.h"
    $infoText = Get-Content -Raw $infoPath
    if ($infoText -notmatch '#define\s+MOD_VERSION\s+"([^"]+)"') {
        throw "Could not infer MOD_VERSION from $infoPath"
    }
    $Tag = $Matches[1]
}

if ($Tag -notmatch '^v?\d+\.\d+(\.\d+)?(\.\d+)?$') {
    throw "Tag must be semantic version text, e.g. v5.0"
}

$tagWithV = if ($Tag.StartsWith("v")) { $Tag } else { "v$Tag" }
$version = $tagWithV.Substring(1)
$outRoot = if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    Join-Path $repoRoot "bin\$Configuration"
} else {
    if ([System.IO.Path]::IsPathRooted($OutputDir)) { $OutputDir } else { Join-Path $repoRoot $OutputDir }
}
$stage = Join-Path $repoRoot "build\release_package_stage\$tagWithV"
$zipName = "BBCF.IM.win-x86.$tagWithV.zip"
$zipPath = Join-Path $outRoot $zipName
$manifestPath = Join-Path $outRoot "update-manifest.json"

Remove-Item -Recurse -Force $stage -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $stage | Out-Null
New-Item -ItemType Directory -Force (Join-Path $stage "BBCF_IM\Updater\defaults") | Out-Null
New-Item -ItemType Directory -Force $outRoot | Out-Null

Copy-Item (Join-Path $repoRoot "bin\$Configuration\dinput8.dll") (Join-Path $stage "dinput8.dll") -Force
Copy-Item (Join-Path $repoRoot "bin\$Configuration\BBCFIMUpdater.exe") (Join-Path $stage "BBCFIMUpdater.exe") -Force
Copy-Item (Join-Path $repoRoot "resource\settings.ini") (Join-Path $stage "settings.ini.default") -Force
Copy-Item (Join-Path $repoRoot "resource\palettes.ini") (Join-Path $stage "palettes.ini.default") -Force
Copy-Item (Join-Path $repoRoot "resource\settings.ini") (Join-Path $stage "BBCF_IM\Updater\defaults\settings.ini.default") -Force
Copy-Item (Join-Path $repoRoot "resource\palettes.ini") (Join-Path $stage "BBCF_IM\Updater\defaults\palettes.ini.default") -Force
Copy-Item (Join-Path $repoRoot "USER_README.txt") (Join-Path $stage "USER_README.txt") -Force

Remove-Item -Force $zipPath -ErrorAction SilentlyContinue
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zipPath -Force
$sha = (Get-FileHash -Algorithm SHA256 $zipPath).Hash.ToLowerInvariant()
$size = (Get-Item $zipPath).Length

$manifest = [ordered]@{
    schemaVersion = 1
    version = $version
    tag = $tagWithV
    channel = $Channel
    os = "win"
    arch = "x86"
    assetName = $zipName
    sha256 = $sha
    entryDll = "dinput8.dll"
    minimumSupportedVersion = "v3.110"
}

$manifestJson = $manifest | ConvertTo-Json -Depth 4
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($manifestPath, $manifestJson, $utf8NoBom)

Remove-Item -Force (Join-Path $outRoot "BBCFIMUpdater.exe") -ErrorAction SilentlyContinue
Remove-Item -Force (Join-Path $outRoot "BBCFIMUpdater.pdb") -ErrorAction SilentlyContinue
Remove-Item -Force (Join-Path $outRoot "dinput8.dll") -ErrorAction SilentlyContinue
Remove-Item -Force (Join-Path $outRoot "settings.ini") -ErrorAction SilentlyContinue
Remove-Item -Force (Join-Path $outRoot "palettes.ini") -ErrorAction SilentlyContinue

Write-Host "Wrote $zipPath"
Write-Host "Wrote $manifestPath"
Write-Host "SHA-256 $sha"
Write-Host "Size $size"
Write-Host "Upload everything in $outRoot to the GitHub release."
