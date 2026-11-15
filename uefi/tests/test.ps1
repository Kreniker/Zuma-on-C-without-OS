#requires -Version 5.1
param([string]$Python = 'python',[string]$OutDir)
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
if (-not $OutDir) { $OutDir = Join-Path $projectRoot 'build-uefi' }
$OutDir = [IO.Path]::GetFullPath($OutDir)
$tools = Get-ChildItem "$env:ProgramFiles\Microsoft Visual Studio\2022\*\VC\Tools\MSVC\*" -Directory |
    Sort-Object Name -Descending | Where-Object { Test-Path (Join-Path $_.FullName 'bin\Hostx64\x64\cl.exe') } | Select-Object -First 1
$sdk = Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\10\Lib" -Directory | Sort-Object Name -Descending | Select-Object -First 1
if (-not $tools -or -not $sdk) { throw 'Host parser tests require VS C++ tools and Windows SDK libraries.' }
$testDir = Join-Path $OutDir 'tests'
$null = New-Item -ItemType Directory -Force -Path $testDir
$dll = Join-Path $testDir ('xmlhost-' + [Guid]::NewGuid().ToString('N') + '.dll')
$compiler = Join-Path $tools.FullName 'bin\Hostx64\x64\cl.exe'
& $compiler /nologo /LD /MD /TC /std:c11 /O2 /D ZUMA_UEFI /wd4244 /wd4305 ('/I'+(Join-Path $projectRoot 'uefi\include')) ('/I'+(Join-Path $tools.FullName 'include')) `
    ('/Fo'+$testDir+'\') ('/Fe'+$dll) (Join-Path $projectRoot 'uefi\Xml.c') (Join-Path $projectRoot 'uefi\Format.c') (Join-Path $projectRoot 'src\gameplay\Level.c') (Join-Path $PSScriptRoot 'level-host.c') /link `
    ('/LIBPATH:'+(Join-Path $tools.FullName 'lib\x64')) ('/LIBPATH:'+(Join-Path $sdk.FullName 'ucrt\x64')) `
    ('/LIBPATH:'+(Join-Path $sdk.FullName 'um\x64')) kernel32.lib `
    /EXPORT:XML_ParserCreate /EXPORT:XML_SetElementHandler /EXPORT:XML_Parse /EXPORT:XML_ParserFree /EXPORT:ZumaTestLevels /EXPORT:ZumaTestFormat
if ($LASTEXITCODE -ne 0) { throw 'Host XML test build failed' }
& $Python (Join-Path $PSScriptRoot 'check-xml.py') $dll
if ($LASTEXITCODE -ne 0) { throw 'XML tests failed' }
& $Python (Join-Path $PSScriptRoot 'check-build.py') $OutDir
if ($LASTEXITCODE -ne 0) { throw 'EFI/resource checks failed' }
& (Join-Path $PSScriptRoot 'check-textures.ps1') -OutDir $OutDir
