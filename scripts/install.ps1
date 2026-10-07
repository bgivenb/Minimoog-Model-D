param([string]$Destination = "$env:ProgramFiles\Common Files\VST3")
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$bundle = Join-Path $projectRoot 'dist\Model D 1970.vst3'
if (-not (Test-Path -LiteralPath $bundle)) { throw 'Build first: run scripts/build.ps1' }
New-Item -ItemType Directory -Path $Destination -Force | Out-Null
Copy-Item -LiteralPath $bundle -Destination $Destination -Recurse -Force
Write-Host "Installed to $Destination\Model D 1970.vst3"
Write-Host 'In Ableton Live: Settings > Plug-Ins > Use VST3 Plug-In System Folders: On, then Rescan.'
