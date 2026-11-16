param(
  [Parameter(Mandatory = $true)][string]$Module,
  [string]$Version = '0.1.3'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not [System.IO.Path]::IsPathRooted($Module)) { $Module = Join-Path $root $Module }
if (-not (Test-Path $Module)) { throw "Module not found: $Module" }

$active = Join-Path $root 'build\modules\GW3E78\active-module.txt'
New-Item -ItemType Directory -Force -Path (Split-Path $active) | Out-Null
$rootFull = (Resolve-Path $root).Path
$moduleFull = (Resolve-Path $Module).Path
if ($moduleFull.StartsWith($rootFull, [System.StringComparison]::OrdinalIgnoreCase)) {
  $relative = $moduleFull.Substring($rootFull.Length).TrimStart('\', '/')
} else {
  $relative = $moduleFull
}
Set-Content $active $relative -Encoding ascii -NoNewline
Write-Host "Active module: $relative"

Get-Process WrestleManiaX8RecompRunner,WrestleManiaX8Recomp -ErrorAction SilentlyContinue | Stop-Process -Force
& (Join-Path $root 'tools\make_release.ps1') -Version $Version -Force | Out-Null

$rel = Join-Path $root ("dist\WrestleManiaX8Recomp-v$Version-Windows-x64")
$zip = Join-Path $root ("dist\WrestleManiaX8Recomp-v$Version-Windows-x64.zip")
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path "$rel\*" -DestinationPath $zip -CompressionLevel Optimal
$hash = (Get-FileHash $zip -Algorithm SHA256).Hash
"$hash  $(Split-Path $zip -Leaf)" | Set-Content (Join-Path $root 'dist\SHA256SUMS.txt')

Write-Host "Release: $zip"
Write-Host "SHA-256: $hash"
Write-Host "Size:    $((Get-Item $zip).Length) bytes"
