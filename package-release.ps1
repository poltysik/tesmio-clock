param([string]$Version = "1.1.4")

$ErrorActionPreference = "Stop"
$dist = Join-Path $PSScriptRoot "dist"
$releaseRoot = Join-Path $dist "Tesmio Clock"
$zip = Join-Path $dist "TesmioClock-$Version.zip"

foreach ($variant in @("24-hour", "12-hour-am-pm")) {
    $dll = Join-Path $PSScriptRoot "variants\$variant\TesmioClock.dll"
    if (-not (Test-Path -LiteralPath $dll)) {
        throw "Missing $dll. Run .\build-portable.ps1 first."
    }
}

if (Test-Path -LiteralPath $releaseRoot) { Remove-Item -LiteralPath $releaseRoot -Recurse -Force }
New-Item -ItemType Directory -Force -Path $releaseRoot | Out-Null

foreach ($variant in @("24-hour", "12-hour-am-pm")) {
    $target = Join-Path $releaseRoot "variants\$variant"
    New-Item -ItemType Directory -Force -Path $target | Out-Null
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot "variants\$variant\TesmioClock.dll") -Destination $target
}

foreach ($file in @(
    "INSTALL-TESMIO-CLOCK.bat", "install.ps1", "clock-format.ini",
    "README.md", "GUIDE-RU.md", "CHANGELOG.md", "LICENSE",
    "WORKSHOP-DESCRIPTION.txt", "TESMIO-CLOCK-VERSION.txt"
)) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $file) -Destination $releaseRoot
}

if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip -Force }
Compress-Archive -LiteralPath $releaseRoot -DestinationPath $zip -CompressionLevel Optimal

Get-FileHash -Algorithm SHA256 -LiteralPath `
    (Join-Path $releaseRoot "variants\24-hour\TesmioClock.dll"),
    (Join-Path $releaseRoot "variants\12-hour-am-pm\TesmioClock.dll"),
    $zip | Select-Object Path, Hash
