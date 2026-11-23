#requires -Version 5.1
[CmdletBinding()]
param([string]$Qemu,[string]$Firmware,[string]$Vars,[string]$OutDir,[string]$QemuImg,[string]$DiskImage,[switch]$RefreshImage)
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not $OutDir) { $OutDir = Join-Path $projectRoot 'build-uefi' }
$OutDir = [IO.Path]::GetFullPath($OutDir)
if (-not $Qemu) {
    $command = Get-Command qemu-system-x86_64.exe -ErrorAction SilentlyContinue
    if ($command) { $Qemu = $command.Source }
    else { $Qemu = Join-Path $env:ProgramFiles 'qemu\qemu-system-x86_64.exe' }
}
if (-not (Test-Path -LiteralPath $Qemu)) { throw 'QEMU not found. Pass -Qemu with the path to qemu-system-x86_64.exe.' }
if (-not $Firmware) {
    if ($env:OVMF_CODE) { $Firmware = $env:OVMF_CODE }
    else { $Firmware = Join-Path (Split-Path $Qemu) 'share\edk2-x86_64-code.fd' }
}
if (-not (Test-Path -LiteralPath $Firmware)) { throw 'OVMF not found. Pass -Firmware with the path to the x64 CODE .fd file.' }
if (-not $Vars) {
    if ($env:OVMF_VARS) { $Vars = $env:OVMF_VARS }
    else {
        $candidate = Join-Path (Split-Path $Firmware) 'edk2-i386-vars.fd'
        if (Test-Path -LiteralPath $candidate) { $Vars = $candidate }
    }
}
$espDir = Join-Path $OutDir 'esp'
if (-not (Test-Path -LiteralPath (Join-Path $espDir 'EFI\BOOT\BOOTX64.EFI'))) { throw 'Build first: .\uefi\build.ps1' }
if (-not $DiskImage) { $DiskImage = Join-Path $OutDir 'Zuma.img' }
$DiskImage = [IO.Path]::GetFullPath($DiskImage)
if (-not (Test-Path -LiteralPath $DiskImage) -or $RefreshImage) {
    if (-not $QemuImg) { $QemuImg = Join-Path (Split-Path $Qemu) 'qemu-img.exe' }
    if (-not (Test-Path -LiteralPath $QemuImg)) { throw 'qemu-img.exe not found. Pass -QemuImg or supply an existing -DiskImage.' }
    & $QemuImg convert -f raw ('fat:'+$espDir) -O raw $DiskImage
    if ($LASTEXITCODE -ne 0) { throw 'Could not prepare the FAT disk image' }
}
$qemuArgs = @('-accel','whpx','-machine','q35','-m','512','-net','none','-device','qemu-xhci','-device','usb-mouse',
    '-drive',('if=pflash,format=raw,readonly=on,file=' + [IO.Path]::GetFullPath($Firmware)),
    '-drive',('format=raw,file=' + $DiskImage))
if ($Vars) {
    if (-not (Test-Path -LiteralPath $Vars)) { throw "VARS file not found: $Vars" }
    $runtimeVars = Join-Path $OutDir 'OVMF_VARS.fd'
    if (-not (Test-Path -LiteralPath $runtimeVars)) { Copy-Item -LiteralPath $Vars -Destination $runtimeVars }
    $qemuArgs += @('-drive',('if=pflash,format=raw,file=' + $runtimeVars))
}
& $Qemu @qemuArgs
if ($LASTEXITCODE -ne 0) { throw "QEMU exited with code $LASTEXITCODE" }
