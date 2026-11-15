#requires -Version 5.1
[CmdletBinding()]
param(
    [string]$Edk2,
    [string]$OutDir,
    [switch]$SkipAssets
)
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not $OutDir) { $OutDir = Join-Path $projectRoot 'build-uefi' }
$OutDir = [IO.Path]::GetFullPath($OutDir)
if (-not $Edk2) {
    $candidates = @(
        $env:EDK2_PATH,
        (Join-Path $projectRoot '..\VisualUEFI\edk2'),
        (Join-Path $projectRoot '..\edk2-vscode-starter-v3\edk2-vscode-starter\third_party\edk2')
    )
    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path -LiteralPath (Join-Path $candidate 'MdePkg\Include\Uefi.h'))) { $Edk2 = $candidate; break }
    }
}
if (-not $Edk2 -or -not (Test-Path -LiteralPath (Join-Path $Edk2 'MdePkg\Include\Uefi.h'))) {
    throw 'EDK II headers not found. Pass -Edk2 C:\path\to\edk2 or set EDK2_PATH.'
}
$Edk2 = [IO.Path]::GetFullPath($Edk2)
# Direct MSVC build: no BaseTools, Windows SDK, SDL, Expat or BASS libraries.
$toolRoots = @()
foreach ($pf in @($env:ProgramFiles, ${env:ProgramFiles(x86)})) {
    if ($pf) {
        $toolRoots += Get-ChildItem (Join-Path $pf 'Microsoft Visual Studio\2022\*\VC\Tools\MSVC\*') -Directory -ErrorAction SilentlyContinue
    }
}
$tools = $toolRoots | Sort-Object Name -Descending | Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'bin\Hostx64\x64\cl.exe') } | Select-Object -First 1
if (-not $tools) { throw 'Visual Studio 2022 C++ x64 tools are required.' }
$compiler = Join-Path $tools.FullName 'bin\Hostx64\x64\cl.exe'
$linker = Join-Path $tools.FullName 'bin\Hostx64\x64\link.exe'
$objDir = Join-Path $OutDir 'obj'
$espDir = Join-Path $OutDir 'esp'
$bootDir = Join-Path $espDir 'EFI\BOOT'
$null = New-Item -ItemType Directory -Force -Path $objDir,$bootDir
$includes = @(
    ('/I' + (Join-Path $PSScriptRoot 'include')),
    ('/I' + (Join-Path $projectRoot 'src')),
    ('/I' + (Join-Path $Edk2 'MdePkg\Include')),
    ('/I' + (Join-Path $Edk2 'MdePkg\Include\X64')),
    ('/I' + (Join-Path $tools.FullName 'include'))
)
$sources = @(Get-ChildItem (Join-Path $projectRoot 'src') -Recurse -Filter '*.c' | Where-Object { $_.Name -ne 'main.c' })
$sources += @(Get-ChildItem $PSScriptRoot -Filter '*.c')
$objects = @()
Push-Location $projectRoot
try {
    foreach ($source in $sources) {
        $relative = $source.FullName.Substring($projectRoot.Length + 1)
        $object = Join-Path $objDir (($relative -replace '[\\/]', '_') + '.obj')
        $objects += $object
        Write-Host "Compiling $relative"
        $warnings = @()
        if ($relative.StartsWith('src\')) { $warnings = @('/wd4244','/wd4305','/wd4018','/wd4101') }
        & $compiler /nologo /c /TC /std:c11 /O2 /Oi- /GS- /Gs9999999 /Zl /W3 @warnings /D ZUMA_UEFI /D _CRT_SECURE_NO_WARNINGS @includes ('/Fo' + $object) $source.FullName
        if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $relative" }
    }
    $binary = Join-Path $bootDir 'BOOTX64.EFI'
    & $linker /nologo /MACHINE:X64 /SUBSYSTEM:EFI_APPLICATION /ENTRY:EfiMain /NODEFAULTLIB /INCREMENTAL:NO /FIXED:NO /OPT:REF /OPT:ICF ('/OUT:' + $binary) ('/MAP:' + (Join-Path $OutDir 'Zuma.map')) @objects
    if ($LASTEXITCODE -ne 0) { throw 'UEFI link failed' }
    if (-not $SkipAssets) { & (Join-Path $PSScriptRoot 'prepare-assets.ps1') -Destination $bootDir }
    elseif (-not (Test-Path -LiteralPath (Join-Path $bootDir 'images\disclaimer.jpg.ztex'))) { throw '-SkipAssets requires an already prepared resource directory.' }
    Write-Host "UEFI application: $binary"
    Write-Host "Copy the CONTENTS of $espDir to a FAT32 volume, preserving the EFI directory."
} finally { Pop-Location }
