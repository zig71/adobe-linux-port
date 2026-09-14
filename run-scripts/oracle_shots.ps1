$i = 0
Add-Type -AssemblyName System.Windows.Forms,System.Drawing
while ($true) {
  try {
    $b = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $bmp = New-Object System.Drawing.Bitmap($b.Width, $b.Height)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($b.Location, [System.Drawing.Point]::Empty, $b.Size)
    $bmp.Save(("C:\AdobeCap\shot_{0:D4}.png" -f $i))
    $g.Dispose(); $bmp.Dispose()
  } catch { }
  $i++
  Start-Sleep -Seconds 15
}
