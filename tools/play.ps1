param(
  [string]$State = '',
  [ValidateSet('Vulkan', 'OpenGL')][string]$Graphics = 'Vulkan',
  [ValidateSet('Cubeb', 'Null')][string]$Audio = 'Cubeb',
  [string]$UserDir = 'work\user-play',
  [ValidateSet('auto', 'dualsense', 'xbox', 'switch', 'keyboard')][string]$Layout = 'auto',
  [string]$Device = '',
  [switch]$MenuDpad,
  [switch]$StockDpad,
  [switch]$Keyboard
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$runner = Join-Path $root 'build\moderngekko\WrestleManiaX8RecompRunner.exe'

if ($Keyboard) { $Layout = 'keyboard' }

$activeFile = Join-Path $root 'build\modules\GW3E78\active-module.txt'
if (-not (Test-Path $activeFile)) {
  throw "No active module. Build one with moderngekko-port first."
}
$module = (Get-Content $activeFile -Raw).Trim()
if (-not [System.IO.Path]::IsPathRooted($module)) {
  $module = Join-Path $root $module
}
if (-not (Test-Path $module)) {
  throw "Active module not found: $module"
}

$userPath = Join-Path $root $UserDir
New-Item -ItemType Directory -Force -Path $userPath | Out-Null

if (-not (Test-Path (Join-Path $userPath 'config.ini'))) {
  Copy-Item (Join-Path $root 'defaults\config.play.ini') (Join-Path $userPath 'config.ini')
}
$seedCard = Join-Path $root 'work\user-3\GC'
if ((Test-Path $seedCard) -and -not (Test-Path (Join-Path $userPath 'GC\USA\Card A'))) {
  Copy-Item $seedCard (Join-Path $userPath 'GC') -Recurse -Force
}

$padName = $null
if ($Layout -eq 'auto') {
  $padName = (Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue |
    Where-Object { $_.FriendlyName -match 'DualSense|DualShock|Xbox|Gamepad|Wireless Controller' } |
    Select-Object -First 1).FriendlyName
  if (-not $padName) {
    $Layout = 'keyboard'
  } elseif ($padName -match 'DualSense') {
    $Layout = 'dualsense'
  } elseif ($padName -match 'Xbox') {
    $Layout = 'xbox'
  } elseif ($padName -match 'Pro Controller|Nintendo') {
    $Layout = 'switch'
  } else {
    $Layout = 'dualsense'
  }
}

$profileFile = switch ($Layout) {
  'dualsense' { 'GCPadNew.dualsense.ini' }
  'xbox'      { 'GCPadNew.xbox.ini' }
  'switch'    { 'GCPadNew.switch-pro.ini' }
  default     { 'GCPadNew.keyboard.ini' }
}

$padConfig = Join-Path $userPath 'Config\GCPadNew.ini'
New-Item -ItemType Directory -Force -Path (Split-Path $padConfig) | Out-Null
$profileText = Get-Content (Join-Path $root ('defaults\' + $profileFile)) -Raw
if (-not $StockDpad -and $Layout -ne 'keyboard') {
  # WrestleMania X8 menus read only the main stick. Fold the physical D-pad
  # into the stick for menu navigation, and keep taunts on L + D-pad.
  $profileText = $profileText -replace '(?m)^Main Stick/Up = .*$', 'Main Stick/Up = `Pad N` | `Left Y+`'
  $profileText = $profileText -replace '(?m)^Main Stick/Down = .*$', 'Main Stick/Down = `Pad S` | `Left Y-`'
  $profileText = $profileText -replace '(?m)^Main Stick/Left = .*$', 'Main Stick/Left = `Pad W` | `Left X-`'
  $profileText = $profileText -replace '(?m)^Main Stick/Right = .*$', 'Main Stick/Right = `Pad E` | `Left X+`'
  $profileText = $profileText -replace '(?m)^D-Pad/Up = .*$', 'D-Pad/Up = `Pad N` & `Shoulder L`'
  $profileText = $profileText -replace '(?m)^D-Pad/Down = .*$', 'D-Pad/Down = `Pad S` & `Shoulder L`'
  $profileText = $profileText -replace '(?m)^D-Pad/Left = .*$', 'D-Pad/Left = `Pad W` & `Shoulder L`'
  $profileText = $profileText -replace '(?m)^D-Pad/Right = .*$', 'D-Pad/Right = `Pad E` & `Shoulder L`'
  Write-Host "Input  : D-pad navigates menus; L + D-pad taunts"
}
if ($Device) {
  $profileText = $profileText -replace '(?m)^Device = .*$', ('Device = ' + $Device)
} elseif ($Layout -ne 'keyboard' -and $padName) {
  # Use the controller name Windows reports unless an explicit device is given.
  $sdlName = switch ($Layout) {
    'dualsense' { 'DualSense Wireless Controller' }
    'xbox'      { ($padName -replace '^Xbox ', 'Xbox ') }
    'switch'    { 'Nintendo Switch Pro Controller' }
  }
  $profileText = $profileText -replace '(?m)^Device = .*$', ('Device = SDL/0/' + $sdlName)
}
Set-Content $padConfig $profileText -Encoding ascii -NoNewline
Write-Host "Input  : $Layout layout ($profileFile)"

$args = @(
  '--game', ('"' + (Join-Path $root 'work\game') + '"'),
  '--module', ('"' + $module + '"'),
  '--user-dir', ('"' + $userPath + '"'),
  '--graphics', $Graphics,
  '--audio', $Audio,
  '--title', '"WWE WrestleMania X8"'
)
if ($State) {
  $statePath = Join-Path $root $State
  if (-not (Test-Path $statePath)) { throw "State not found: $statePath" }
  $args += @('--load-state', ('"' + $statePath + '"'))
}

Write-Host "Module : $module"
Write-Host "Game   : $(Join-Path $root 'work\game')"
Write-Host "User   : $userPath"
if ($State) { Write-Host "State  : $State" }
Write-Host "Close the window to quit. -StockDpad restores D-pad taunts; -Layout forces a pad layout."

Start-Process -FilePath $runner -ArgumentList $args | Out-Null
