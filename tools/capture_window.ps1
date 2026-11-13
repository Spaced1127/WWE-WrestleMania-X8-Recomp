param([string]$Out = 'work\evidence\launcher.png', [string]$ProcessName = 'WrestleManiaX8Recomp')

Add-Type @"
using System;
using System.Text;
using System.Runtime.InteropServices;
public class CaptureWin {
  public delegate bool EnumProc(IntPtr hWnd, IntPtr lParam);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr lp);
  [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr hWnd, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);
  public static IntPtr Find(uint target) {
    IntPtr found = IntPtr.Zero;
    EnumWindows((h,l) => {
      uint pid; GetWindowThreadProcessId(h, out pid);
      if (pid == target) {
        var sb = new StringBuilder(512); GetWindowText(h, sb, 512);
        if (sb.ToString().StartsWith("WWE WrestleMania X8 Recomp")) { found = h; return false; }
      }
      return true;
    }, IntPtr.Zero);
    return found;
  }
}
"@

Add-Type -AssemblyName System.Drawing
$p = Get-Process $ProcessName -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $p) { throw 'launcher is not running' }
$h = [CaptureWin]::Find([uint32]$p.Id)
if ($h -eq [IntPtr]::Zero) { throw 'launcher window not found' }
$rect = New-Object CaptureWin+RECT
[CaptureWin]::GetWindowRect($h, [ref]$rect) | Out-Null
$w = $rect.Right - $rect.Left
$hh = $rect.Bottom - $rect.Top
$bmp = New-Object System.Drawing.Bitmap $w, $hh
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc()
[CaptureWin]::PrintWindow($h, $hdc, 2) | Out-Null
$g.ReleaseHdc($hdc)
$g.Dispose()
$bmp.Save((Join-Path (Get-Location) $Out), [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
