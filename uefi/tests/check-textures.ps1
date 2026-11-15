#requires -Version 5.1
param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$contentRoot = Join-Path $projectRoot 'content'
$boot = Join-Path $OutDir 'esp\EFI\BOOT'
$count = 0
foreach ($source in Get-ChildItem $contentRoot -Recurse -File | Where-Object { $_.Extension.ToLowerInvariant() -in @('.png','.jpg','.jpeg') }) {
    $relative = $source.FullName.Substring($contentRoot.Length+1)
    $target = Join-Path $boot ($relative+'.ztex')
    $bytes = [IO.File]::ReadAllBytes($target)
    $bitmap = [Drawing.Bitmap]::FromFile($source.FullName)
    try {
        for ($sample=0; $sample -lt 32; $sample++) {
            $x = ($sample*7919+17) % $bitmap.Width
            $y = ($sample*3571+13) % $bitmap.Height
            $pixel = $bitmap.GetPixel($x,$y)
            $offset = 12+4*($y*$bitmap.Width+$x)
            if ($bytes[$offset+3] -ne $pixel.A) { throw "Alpha/pixel position mismatch: $relative ($x,$y)" }
            # GDI+ can round RGB when converting partially transparent pixels.
            if ($pixel.A -ge 128 -and ([Math]::Abs($bytes[$offset]-$pixel.B) -gt 2 -or [Math]::Abs($bytes[$offset+1]-$pixel.G) -gt 2 -or [Math]::Abs($bytes[$offset+2]-$pixel.R) -gt 2)) {
                throw "RGB/pixel position mismatch: $relative ($x,$y)"
            }
        }
    } finally { $bitmap.Dispose() }
    $count++
}
Write-Host "PASS: $count textures sampled against source images at original pixel coordinates"
