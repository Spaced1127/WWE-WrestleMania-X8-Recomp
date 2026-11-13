param(
  [Parameter(Mandatory = $true)][string]$Label,
  [Parameter(Mandatory = $true)][string]$Module,
  [ValidateSet('static', 'jit')][string]$Core = 'static',
  [string]$State = 'work\states\match.sav',
  [string]$UserDir = 'work\user-3',
  [int]$WarmupSeconds = 35,
  [int]$MeasureSeconds = 15
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$runner = Join-Path $root 'build\moderngekko\WrestleManiaX8RecompRunner.exe'
$auto = Join-Path $root 'work\auto-bench'

# Never touch a running instance from a different benchmark.
Get-Process WrestleManiaX8RecompRunner -ErrorAction SilentlyContinue | Stop-Process -Force

New-Item -ItemType Directory -Force -Path (Join-Path $auto 'commands') | Out-Null
Get-ChildItem (Join-Path $auto 'commands') -File -ErrorAction SilentlyContinue | Remove-Item -Force
Remove-Item (Join-Path $auto 'status.txt') -Force -ErrorAction SilentlyContinue

$env:MODERNGEKKO_STATICRECOMP = if ($Core -eq 'jit') { '0' } else { '1' }

$args = @(
  '--game', ('"' + (Join-Path $root 'work\game') + '"'),
  '--user-dir', ('"' + (Join-Path $root $UserDir) + '"'),
  '--graphics', 'Vulkan',
  '--audio', 'Null',
  '--title', ('"bench-' + $Label + '"'),
  '--automation-dir', ('"' + $auto + '"'),
  '--load-state', ('"' + (Join-Path $root $State) + '"')
)
if ($Core -eq 'static') {
  $args += @('--module', ('"' + $Module + '"'))
} else {
  $args += @('--allow-interpreter')
}

$p = Start-Process -FilePath $runner -ArgumentList $args -PassThru `
  -RedirectStandardOutput (Join-Path $root 'work\bench-stdout.txt') `
  -RedirectStandardError (Join-Path $root 'work\bench-stderr.txt')

Start-Sleep -Seconds $WarmupSeconds

$statusPath = Join-Path $auto 'status.txt'
function Read-Status {
  if (-not (Test-Path $statusPath)) { return $null }
  $map = @{}
  Get-Content $statusPath | ForEach-Object {
    $kv = $_ -split '=', 2
    if ($kv.Count -eq 2) { $map[$kv[0]] = $kv[1] }
  }
  return $map
}

$first = Read-Status
Start-Sleep -Seconds $MeasureSeconds
$second = Read-Status
if (-not $second -or -not $second['fps']) {
  if ($p -and -not $p.HasExited) { $p.Kill() }
  throw "No automation status after $($WarmupSeconds + $MeasureSeconds)s. Check work/bench-stderr.txt."
}

$cpu = $null
if ($p -and -not $p.HasExited) {
  $proc = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
  if ($proc) { $cpu = $proc.CPU }
  $p.Kill()
}

$line = [pscustomobject]@{
  Time      = (Get-Date).ToString('yyyy-MM-dd HH:mm:ss')
  Label     = $Label
  Core      = $Core
  FPS       = $second['fps']
  VPS       = $second['vps']
  Speed     = $second['speed']
  Frames    = $second['frame_count']
  CPUSeconds= $cpu
  State     = $State
  Module    = Split-Path -Leaf $Module
}

$csv = Join-Path $root 'docs\benchmarks.csv'
$line | Export-Csv -Path $csv -NoTypeInformation -Append
$line | Format-List

Start-Sleep -Seconds 3
