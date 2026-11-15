#requires -Version 5.1
param([Parameter(Mandatory=$true)][string]$Destination)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$contentRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\content'))
$Destination = [IO.Path]::GetFullPath($Destination)
$images = 0
# These directories contain every visual resource, font definition and level.
foreach ($folder in @('images','fonts','levels')) {
    foreach ($source in Get-ChildItem (Join-Path $contentRoot $folder) -Recurse -File) {
        $relative = $source.FullName.Substring($contentRoot.Length + 1)
        $target = Join-Path $Destination $relative
        $null = New-Item -ItemType Directory -Force -Path ([IO.Path]::GetDirectoryName($target))
        if ($source.Extension.ToLowerInvariant() -in @('.png','.jpg','.jpeg')) {
            $target += '.ztex'
            if ((Test-Path -LiteralPath $target) -and (Get-Item -LiteralPath $target).LastWriteTimeUtc -ge $source.LastWriteTimeUtc -and (Get-Item -LiteralPath $target).LastWriteTimeUtc -ge (Get-Item -LiteralPath $PSCommandPath).LastWriteTimeUtc) { continue }
            $original = [Drawing.Bitmap]::FromFile($source.FullName)
            $bitmap = [Drawing.Bitmap]::new($original.Width,$original.Height,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
            $graphics = [Drawing.Graphics]::FromImage($bitmap)
            try {
                $graphics.CompositingMode = [Drawing.Drawing2D.CompositingMode]::SourceCopy
                $rect = [Drawing.Rectangle]::new(0,0,$bitmap.Width,$bitmap.Height)
                # Explicit pixel units avoid rescaling 72-DPI sprite atlases to 96 DPI.
                $graphics.DrawImage($original,$rect,0,0,$original.Width,$original.Height,[Drawing.GraphicsUnit]::Pixel)
                $data = $bitmap.LockBits($rect,[Drawing.Imaging.ImageLockMode]::ReadOnly,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
                try {
                    $stream = [IO.File]::Create($target)
                    $writer = [IO.BinaryWriter]::new($stream)
                    try {
                        $writer.Write([uint32]0x58544d5a) # ZMTX, width, height, top-down BGRA8.
                        $writer.Write([uint32]$bitmap.Width)
                        $writer.Write([uint32]$bitmap.Height)
                        $row = [byte[]]::new($bitmap.Width * 4)
                        for ($y=0; $y -lt $bitmap.Height; $y++) {
                            [Runtime.InteropServices.Marshal]::Copy([IntPtr]::Add($data.Scan0,$y*$data.Stride),$row,0,$row.Length)
                            $writer.Write($row)
                        }
                    } finally { $writer.Dispose() }
                } finally { $bitmap.UnlockBits($data) }
            } finally { $graphics.Dispose(); $bitmap.Dispose(); $original.Dispose() }
            $images++
        } else { Copy-Item -LiteralPath $source.FullName -Destination $target -Force }
    }
}
$settings = Join-Path $Destination 'settings.txt'
if (-not (Test-Path -LiteralPath $settings)) { [IO.File]::WriteAllText($settings,"MUSIC 0`nSOUNDS 0`nFULLSCREEN 1",[Text.Encoding]::ASCII) }
Write-Host "Prepared resources ($images images converted)."
