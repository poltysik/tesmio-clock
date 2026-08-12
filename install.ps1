param(
    [string]$Game = "",
    [ValidateSet("prompt", "24-hour", "12-hour-am-pm")]
    [string]$Format = "prompt"
)

$ErrorActionPreference = "Stop"
$packageRoot = $PSScriptRoot
$packageVersionFile = Join-Path $packageRoot "TESMIO-CLOCK-VERSION.txt"
$packageVersion = "unknown"
if (Test-Path -LiteralPath $packageVersionFile) {
    $versionMatch = [regex]::Match(
        (Get-Content -Raw -LiteralPath $packageVersionFile),
        '(?im)^\s*Package version\s*:\s*([0-9]+(?:\.[0-9]+)*)\s*$'
    )
    if ($versionMatch.Success) { $packageVersion = $versionMatch.Groups[1].Value }
}

function Find-GameDirectory {
    param([string]$Requested)

    $candidates = [System.Collections.Generic.List[string]]::new()
    if ($Requested) { $candidates.Add($Requested) }

    try {
        $fromWorkshop = [IO.Path]::GetFullPath((Join-Path $packageRoot "..\..\..\..\common\SovietRepublic"))
        $candidates.Add($fromWorkshop)
    } catch {}

    if (${env:ProgramFiles(x86)}) {
        $candidates.Add((Join-Path ${env:ProgramFiles(x86)} "Steam\steamapps\common\SovietRepublic"))
    }

    try {
        $steamPath = (Get-ItemProperty -LiteralPath "HKCU:\Software\Valve\Steam" -ErrorAction Stop).SteamPath
        if ($steamPath) {
            $candidates.Add((Join-Path $steamPath "steamapps\common\SovietRepublic"))
            $libraryFile = Join-Path $steamPath "steamapps\libraryfolders.vdf"
            if (Test-Path -LiteralPath $libraryFile) {
                foreach ($match in [regex]::Matches((Get-Content -Raw -LiteralPath $libraryFile), '"path"\s+"([^"]+)"')) {
                    $library = $match.Groups[1].Value -replace '\\\\', '\'
                    $candidates.Add((Join-Path $library "steamapps\common\SovietRepublic"))
                }
            }
        }
    } catch {}

    foreach ($candidate in $candidates | Select-Object -Unique) {
        if (-not $candidate) { continue }
        $resolved = [IO.Path]::GetFullPath($candidate)
        if (Test-Path -LiteralPath (Join-Path $resolved "tesmioloader\build\tesmioloader.dll")) {
            return $resolved
        }
    }
    return $null
}

function Read-PreferredFormat {
    $preference = Join-Path $packageRoot "clock-format.ini"
    if (Test-Path -LiteralPath $preference) {
        $match = [regex]::Match((Get-Content -Raw -LiteralPath $preference), '(?im)^\s*format\s*=\s*(24-hour|12-hour-am-pm)\s*$')
        if ($match.Success) { return $match.Groups[1].Value.ToLowerInvariant() }
    }
    return "24-hour"
}

function Select-ClockFormat {
    param([string]$Requested)
    if ($Requested -ne "prompt") { return $Requested }

    $current = Read-PreferredFormat
    Write-Host ""
    Write-Host "Tesmio Clock display format / Формат часов:"
    Write-Host "  1 - 24-hour:  16:00  (default / по умолчанию)"
    Write-Host "  2 - AM/PM:     4:00 PM"
    Write-Host ""
    $answer = Read-Host "Choose 1 or 2; Enter keeps '$current'"
    if (-not $answer) { return $current }
    if ($answer -eq "1") { return "24-hour" }
    if ($answer -eq "2") { return "12-hour-am-pm" }
    throw "Unknown selection '$answer'. Run the installer again and choose 1 or 2."
}

function Set-IniValue {
    param([string]$Text, [string]$Key, [string]$Value)
    $pattern = "(?im)^\s*" + [regex]::Escape($Key) + "\s*=.*$"
    if ([regex]::IsMatch($Text, $pattern)) {
        return [regex]::Replace($Text, $pattern, "$Key = $Value", 1)
    }
    return $Text.TrimEnd() + "`r`n$Key = $Value`r`n"
}

$gameRoot = Find-GameDirectory -Requested $Game
if (-not $gameRoot) {
    throw "SovietRepublic with TesmioLoader was not found. Pass the game folder to INSTALL-TESMIO-CLOCK.bat or install TesmioLoader first."
}

$selected = Select-ClockFormat -Requested $Format
$savedPreference = @"
; Tesmio Clock installer preference.
; Use exactly one value: 24-hour or 12-hour-am-pm.

[clock]
format = $selected
"@
Set-Content -LiteralPath (Join-Path $packageRoot "clock-format.ini") -Value $savedPreference -Encoding ASCII
$sourceDll = Join-Path $packageRoot "variants\$selected\TesmioClock.dll"
if (-not (Test-Path -LiteralPath $sourceDll)) {
    throw "The selected DLL is missing: $sourceDll"
}

$pluginDir = Join-Path $gameRoot "tesmioloader\build\plugins"
$synchronizerDll = Join-Path $pluginDir "daynight.dll"
$synchronizerIni = Join-Path $pluginDir "daynight.ini"
if (-not (Test-Path -LiteralPath $synchronizerDll)) {
    throw "Calendar Synchronizer (daynight.dll) is not installed. Install Workshop item 3779646468 first."
}
if (-not (Test-Path -LiteralPath $synchronizerIni)) {
    throw "Calendar Synchronizer configuration is missing: $synchronizerIni"
}

New-Item -ItemType Directory -Force -Path $pluginDir | Out-Null

# Disable the development filename so only one clock hook can load.
$legacyDll = Join-Path $pluginDir "GameClock.dll"
if (Test-Path -LiteralPath $legacyDll) {
    $legacyBackup = Join-Path $pluginDir "GameClock.dll.disabled-by-tesmioclock"
    Move-Item -LiteralPath $legacyDll -Destination $legacyBackup -Force
    Write-Host "Disabled legacy GameClock.dll to prevent duplicate hooks."
}

Copy-Item -LiteralPath $sourceDll -Destination (Join-Path $pluginDir "TesmioClock.dll") -Force

# Keep a one-time backup, then apply the tested clock calibration while
# preserving all comments and unrelated Calendar Synchronizer settings.
$syncBackup = "$synchronizerIni.tesmioclock-backup"
if (-not (Test-Path -LiteralPath $syncBackup)) {
    Copy-Item -LiteralPath $synchronizerIni -Destination $syncBackup
}
$syncText = Get-Content -Raw -LiteralPath $synchronizerIni
$syncText = Set-IniValue $syncText "enabled" "1"
$syncText = Set-IniValue $syncText "cycle_days" "1"
$syncText = Set-IniValue $syncText "day_scale" "auto"
$syncText = Set-IniValue $syncText "offset" "0.5825"
$syncText = Set-IniValue $syncText "fade" "1"
$syncText = Set-IniValue $syncText "probe" "0"
Set-Content -LiteralPath $synchronizerIni -Value $syncText -Encoding ASCII

$installedPreference = @"
; Generated by INSTALL-TESMIO-CLOCK.bat.
[clock]
format = $selected
"@
Set-Content -LiteralPath (Join-Path $pluginDir "TesmioClock.ini") -Value $installedPreference -Encoding ASCII

$sourceHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $sourceDll).Hash
$installedHash = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $pluginDir "TesmioClock.dll")).Hash
if ($sourceHash -ne $installedHash) { throw "Installed DLL verification failed." }

$installedVersion = @"
Tesmio Clock
Installed version: $packageVersion
Format: $selected
Installed at: $([DateTime]::Now.ToString('yyyy-MM-dd HH:mm:ss'))
Workshop source: $packageRoot
SHA256: $installedHash
"@
Set-Content -LiteralPath (Join-Path $pluginDir "TesmioClock.version.txt") -Value $installedVersion -Encoding UTF8

Write-Host ""
Write-Host "Tesmio Clock installed successfully."
Write-Host "Game:   $gameRoot"
Write-Host "Version: $packageVersion"
Write-Host "Format: $selected"
Write-Host "SHA256: $installedHash"
Write-Host "Start the game through tesmiolauncher.exe and enable TesmioClock.dll."
