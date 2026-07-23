param([string]$Out, [int]$Wait = 6)
$exe = "C:\randy\cf-hrc-perf\build\Release\hrc.exe"
$p = Start-Process -FilePath $exe -WorkingDirectory "C:\randy\cf-hrc-perf\build\Release" -PassThru
Start-Sleep -Seconds $Wait
$p.Refresh()
Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public class W {
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  public struct RECT { public int L, T, R, B; }
}
'@
$hwnd = $p.MainWindowHandle
if ($hwnd -eq [IntPtr]::Zero) { Write-Output "NO WINDOW"; Stop-Process -Id $p.Id -Force; exit 1 }
$r = New-Object W+RECT
[W]::GetWindowRect($hwnd, [ref]$r) | Out-Null
$w = $r.R - $r.L; $h = $r.B - $r.T
$bmp = New-Object System.Drawing.Bitmap($w, $h)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$dc = $g.GetHdc()
[W]::PrintWindow($hwnd, $dc, 2) | Out-Null
$g.ReleaseHdc($dc)
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
Write-Output "saved $w x $h -> $Out"
Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
