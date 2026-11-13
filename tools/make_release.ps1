param(
  [string]$Version = '0.1.0',
  [string]$Module = '',
  [switch]$Force
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root 'build\moderngekko'
$out = Join-Path $root ("dist\WrestleManiaX8Recomp-v$Version-Windows-x64")
$launcher = Join-Path $build 'WrestleManiaX8Recomp.exe'
$runner = Join-Path $build 'WrestleManiaX8RecompRunner.exe'
$release = Join-Path $root 'docs\release'

foreach ($required in @($launcher, $runner, (Join-Path $build 'Sys'))) {
  if (-not (Test-Path $required)) {
    throw "Missing build output: $required. Build moderngekko-launcher and moderngekko-run first."
  }
}

if (-not $Module) {
  $active = Join-Path $root 'build\modules\GW3E78\active-module.txt'
  if (-not (Test-Path $active)) { throw 'No active module; pass -Module <gGW3E78_recomp.dll>.' }
  $Module = (Get-Content $active -Raw).Trim()
  if (-not [System.IO.Path]::IsPathRooted($Module)) { $Module = Join-Path $root $Module }
}
if (-not (Test-Path $Module)) { throw "Module not found: $Module" }

if (Test-Path $out) {
  if (-not $Force) { throw "$out already exists. Use -Force to replace it." }
  Remove-Item $out -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $out, (Join-Path $out 'LICENSES') | Out-Null

Copy-Item $launcher $out
Copy-Item $runner $out
Copy-Item $Module (Join-Path $out 'gGW3E78_recomp.dll')
Copy-Item (Join-Path $build 'Sys') (Join-Path $out 'Sys') -Recurse -Force

# Keep only the system data this port uses. The stock Sys tree carries
# GameSettings and wiitdb entries for hundreds of other titles, which would
# leak other games' names into the release and its theme audit.
$sys = Join-Path $out 'Sys'
$keep_dirs = @('GC', 'Shaders', 'Resources')
foreach ($dir in Get-ChildItem $sys -Directory) {
  if ($dir.Name -notin $keep_dirs) {
    Remove-Item $dir.FullName -Recurse -Force
  }
}
Get-ChildItem $sys -File | Remove-Item -Force
$sys_resources = Join-Path $sys 'Resources'
if (Test-Path $sys_resources) {
  Get-ChildItem $sys_resources -File |
    Where-Object { $_.Name -notin @('OSD_Font.ttf', 'OSD_Font_VeraMono_Copyright.txt') } |
    Remove-Item -Force
}

$resources = Join-Path $root 'resources'
if (Test-Path $resources) {
  Copy-Item $resources (Join-Path $out 'resources') -Recurse -Force
}

Copy-Item (Join-Path $release 'README.txt') $out
Copy-Item (Join-Path $release 'PLAYERS.md') $out
Copy-Item (Join-Path $release 'recomplauncher.json') $out
Copy-Item (Join-Path $release 'release-notes-v0.1.0.md') $out
Copy-Item (Join-Path $release 'THIRD_PARTY.md') $out

$defaults = Join-Path $root 'defaults'
if (Test-Path $defaults) {
  Copy-Item $defaults (Join-Path $out 'defaults') -Recurse -Force
  if (Test-Path (Join-Path $defaults 'theme.ini')) {
    Copy-Item (Join-Path $defaults 'theme.ini') (Join-Path $out 'theme.ini') -Force
  }
  $game_ini = Join-Path $defaults 'GameSettings\GW3E78.ini'
  if (Test-Path $game_ini) {
    $settings_dir = Join-Path $out 'Sys\GameSettings'
    New-Item -ItemType Directory -Force -Path $settings_dir | Out-Null
    Copy-Item $game_ini (Join-Path $settings_dir 'GW3E78.ini') -Force
  }
}

$licenses = @(
  @{ src = 'lib\ModernGekko-Template\lib\ModernGekko\LICENSE'; dst = 'ModernGekko-GPL-3.0.txt' },
  @{ src = 'lib\ModernGekko-Template\lib\ModernGekko\vendor\dolphin\COPYING'; dst = 'Dolphin-GPL-2.0.txt' },
  @{ src = 'lib\ModernGekko-Template\lib\ModernGekko\vendor\dolphin\DolRecomp\LICENSE'; dst = 'DolRecomp-GPL-3.0.txt' },
  @{ src = 'lib\ModernGekko-Template\lib\ModernGekko\vendor\dolphin\Externals\SDL\SDL\LICENSE.txt'; dst = 'SDL3-Zlib.txt' },
  @{ src = 'lib\ModernGekko-Template\lib\ModernGekko\vendor\dolphin\Externals\imgui\imgui\LICENSE.txt'; dst = 'ImGui-MIT.txt' }
)
foreach ($license in $licenses) {
  $src = Join-Path $root $license.src
  if (Test-Path $src) {
    Copy-Item $src (Join-Path $out ('LICENSES\' + $license.dst))
  } else {
    Write-Warning "License missing: $src"
  }
}

# Refuse to package game data.
$blocked = Get-ChildItem $out -Recurse -File | Where-Object {
  $_.Extension -in '.iso', '.rvz', '.gcm', '.wbfs', '.dol', '.fst', '.elf', '.rel', '.gci', '.sav' -or
  $_.FullName -match '\\Game\\' -or $_.FullName -match '\\work\\'
}
if ($blocked) {
  throw ("Game data found in release: " + ($blocked.FullName -join ', '))
}

Write-Host "Release folder: $out"
Get-ChildItem $out -Recurse -File | Sort-Object FullName | ForEach-Object {
  "{0,10:N0}  {1}" -f $_.Length, $_.FullName.Substring($out.Length + 1)
}
