param(
    [Parameter(Mandatory=$true)][string]$Tag,
    [string]$Configuration = "Release",
    [string]$OutputDir = "release"
)

$ErrorActionPreference = "Stop"

if ($Tag -notmatch '^v?\d+\.\d+(\.\d+)?(\.\d+)?$') {
    throw "Tag must be semantic version text, e.g. v5.0"
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$tagWithV = if ($Tag.StartsWith("v")) { $Tag } else { "v$Tag" }
$version = $tagWithV.Substring(1)
$outRoot = Join-Path $repoRoot $OutputDir
$stage = Join-Path $outRoot "stage-$tagWithV"
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
    channel = "stable"
    os = "win"
    arch = "x86"
    assetName = $zipName
    sha256 = $sha
    entryDll = "dinput8.dll"
    minimumSupportedVersion = "v3.110"
}

$manifest | ConvertTo-Json -Depth 4 | Set-Content -Encoding UTF8 $manifestPath

Write-Host "Wrote $zipPath"
Write-Host "Wrote $manifestPath"
Write-Host "SHA-256 $sha"
Write-Host "Size $size"
