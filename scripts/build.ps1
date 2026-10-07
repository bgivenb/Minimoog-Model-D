param([ValidateSet('Release','Debug')][string]$Configuration = 'Release')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
cmake -S $projectRoot -B "$projectRoot/build" -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }
cmake --build "$projectRoot/build" --config $Configuration --parallel 6
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
ctest --test-dir "$projectRoot/build" -C $Configuration --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'VST3 validation failed' }
$package = Join-Path $projectRoot 'dist'
New-Item -ItemType Directory -Force $package | Out-Null
Copy-Item -LiteralPath "$projectRoot/build/EmberModel3_artefacts/$Configuration/VST3/Model D 1970.vst3" -Destination $package -Recurse -Force
Copy-Item -LiteralPath "$projectRoot/build/EmberModel3_artefacts/$Configuration/Standalone/Model D 1970.exe" -Destination $package -Force
Copy-Item -LiteralPath "$projectRoot/README.md" -Destination "$package/README.md" -Force
Copy-Item -LiteralPath "$projectRoot/build/Testing/Temporary/LastTest.log" -Destination "$package/Validation.txt" -Force
Write-Host "Validated build: $package"
