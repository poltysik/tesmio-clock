param(
    [string]$Toolchain = "",
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",
    [ValidateSet("all", "24-hour", "12-hour-am-pm")]
    [string]$Variant = "all"
)

$ErrorActionPreference = "Stop"

if (-not $Toolchain) {
    $candidates = @(
        (Join-Path $PSScriptRoot "tools\llvm-mingw"),
        (Join-Path $PSScriptRoot "..\TesmioCatalog-Source\tools\llvm-mingw")
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath (Join-Path $candidate "bin\x86_64-w64-mingw32-clang++.exe")) {
            $Toolchain = $candidate
            break
        }
    }
}

$compiler = Join-Path $Toolchain "bin\x86_64-w64-mingw32-clang++.exe"
if (-not $Toolchain -or -not (Test-Path -LiteralPath $compiler)) {
    throw "llvm-mingw was not found. Put it in tools\llvm-mingw or pass -Toolchain."
}

$optimization = if ($Configuration -eq "Debug") { "-O0" } else { "-O2" }
$source = Join-Path $PSScriptRoot "src\tesmioclock.cpp"
$include = Join-Path $PSScriptRoot "include"
$targets = if ($Variant -eq "all") { @("24-hour", "12-hour-am-pm") } else { @($Variant) }

foreach ($target in $targets) {
    $define = if ($target -eq "24-hour") { "-DGAMECLOCK_24H" } else { "-DGAMECLOCK_12H" }
    $outputDir = Join-Path $PSScriptRoot "variants\$target"
    $output = Join-Path $outputDir "TesmioClock.dll"
    New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

    & $compiler `
        -std=c++17 $optimization -g0 -DNDEBUG $define `
        -fno-exceptions -fno-rtti `
        -Wall -Wextra -Wpedantic `
        -shared -static `
        -I $include `
        $source `
        -o $output `
        -lkernel32 -luser32

    if ($LASTEXITCODE -ne 0) { throw "Compiler failed for $target with code $LASTEXITCODE" }
    Get-FileHash -Algorithm SHA256 -LiteralPath $output | Select-Object Path, Hash
}

