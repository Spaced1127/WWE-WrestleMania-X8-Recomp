param(
  [Parameter(Mandatory = $true)][string]$Image,
  [string]$Output = 'work\game',
  [string]$Launcher = '',
  [switch]$Force
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

if (-not $Launcher) {
  $Launcher = Join-Path $root 'build\moderngekko\WrestleManiaX8Recomp.exe'
}
if (-not (Test-Path $Launcher)) {
  throw "Launcher not found: $Launcher. Run tools\build_launcher.bat first."
}

$imagePath = (Resolve-Path -LiteralPath $Image).Path
$extension = [IO.Path]::GetExtension($imagePath).ToLowerInvariant()
if ($extension -notin @('.iso', '.rvz', '.wbfs')) {
  throw "Unsupported disc image: $extension. Use .iso, .rvz or .wbfs."
}

$outputPath = Join-Path $root $Output
if (Test-Path $outputPath) {
  if (-not $Force) {
    throw "Output exists: $outputPath. Pass -Force to replace it."
  }
  Remove-Item -LiteralPath $outputPath -Recurse -Force
}

$temp = Join-Path $root 'work\extract-temp'
if (Test-Path $temp) {
  Remove-Item -LiteralPath $temp -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $temp | Out-Null
Copy-Item -LiteralPath $Launcher (Join-Path $temp 'WrestleManiaX8Recomp.exe')

Write-Host "Extracting $imagePath with Dolphin DiscIO (native $extension; no ISO conversion)"
$process = Start-Process -FilePath (Join-Path $temp 'WrestleManiaX8Recomp.exe') `
  -ArgumentList @('--extract', ('"' + $imagePath + '"')) `
  -WorkingDirectory $temp -PassThru -Wait
$process.Refresh()
if ($process.ExitCode -ne 0 -or -not (Test-Path (Join-Path $temp 'Game'))) {
  throw "Extraction failed (exit $($process.ExitCode)). See the output above."
}

Move-Item -LiteralPath (Join-Path $temp 'Game') $outputPath
Remove-Item -LiteralPath $temp -Recurse -Force

$dol = Join-Path $outputPath 'sys\main.dol'
$hash = (Get-FileHash -LiteralPath $dol -Algorithm SHA256).Hash
Write-Host "Extracted: $outputPath"
Write-Host "DOL SHA-256: $hash"
if ($hash -ne '91AF3136F2A48C18A6B96A5726D1F171C9E714BB8D4D504F835070FC230A303E') {
  throw "Unexpected DOL hash. The image is not the supported USA retail disc."
}
Write-Host "OK: retail DOL verified."
