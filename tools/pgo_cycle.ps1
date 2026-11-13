param(
  [int]$TrainSeconds = 75
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$port = Join-Path $root 'build\moderngekko\moderngekko-port.exe'
$game = Join-Path $root 'work\game'
$out = Join-Path $root 'build\modules-pgo'
$profile = Join-Path $root 'work\pgo'
$profdata = 'C:\Program Files\LLVM\bin\llvm-profdata.exe'
$log = Join-Path $root 'work\pgo-run.log'
$auto = Join-Path $root 'work\pgo-auto'

New-Item -ItemType Directory -Force -Path (Join-Path $auto 'commands') | Out-Null
Get-ChildItem (Join-Path $auto 'commands') -File -ErrorAction SilentlyContinue | Remove-Item -Force
Remove-Item (Join-Path $auto 'status.txt') -Force -ErrorAction SilentlyContinue
Remove-Item $log -Force -ErrorAction SilentlyContinue

$args = @(
  'pgo-run', ('"' + $game + '"'),
  '--backend', 'c', '--toolchain', 'clang', '--opt-level', '3',
  '--output', ('"' + $out + '"'),
  '--profile-dir', ('"' + $profile + '"'),
  '--llvm-profdata', ('"' + $profdata + '"'),
  '--keep-work',
  '--',
  '--graphics', 'Vulkan', '--audio', 'Null',
  '--user-dir', ('"' + (Join-Path $root 'work\user-f4pgo') + '"'),
  '--automation-dir', ('"' + $auto + '"'),
  '--load-state', ('"' + (Join-Path $root 'work\states\fatal4.sav') + '"'),
  '--headless'
)

$p = Start-Process -FilePath $port -ArgumentList $args -PassThru `
  -RedirectStandardOutput $log -RedirectStandardError ($log + '.err')

$started = $false
for ($i = 0; $i -lt 360; $i++) {
  Start-Sleep -Seconds 5
  if (Test-Path $log) {
    if (Select-String -Path $log -Pattern 'PGO training has started' -Quiet) { $started = $true; break }
  }
  if ($p.HasExited) { break }
}
if (-not $started) {
  if (-not $p.HasExited) { $p.Kill() }
  throw "PGO training did not start. See $log"
}

Start-Sleep -Seconds $TrainSeconds
Set-Content (Join-Path $auto 'commands\stop.txt') "command=stop" -Encoding ascii

$p.WaitForExit(300000) | Out-Null
if (-not $p.HasExited) { $p.Kill(); throw "PGO run did not finish after stop. See $log" }

Write-Host "PGO exit=$($p.ExitCode)"
Get-Content $log -Tail 30
