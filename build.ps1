# Builds GyroAim.asi (x64 DLL). Uses MSVC (cl) if available, else clang/g++ (llvm-mingw / MinGW).
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$out  = Join-Path $root 'build'
New-Item -ItemType Directory -Force $out | Out-Null
$src  = Join-Path $root 'src\main.cpp'
$asi  = Join-Path $out 'GyroAim.asi'

$local = Join-Path $root '..\tools\llvm-mingw\bin\x86_64-w64-mingw32-clang++.exe'
if (Test-Path $local) {
    & $local -std=c++17 -O2 -Wall -shared -static -o $asi $src
} elseif (Get-Command cl -ErrorAction SilentlyContinue) {
    & cl /nologo /std:c++17 /O2 /EHsc /MT /LD $src /Fo"$out\\" /Fe"$asi" /link /DLL
} elseif (Get-Command clang++ -ErrorAction SilentlyContinue) {
    & clang++ -std=c++17 -O2 -shared -static -o $asi $src
} elseif (Get-Command g++ -ErrorAction SilentlyContinue) {
    & g++ -std=c++17 -O2 -shared -static -static-libgcc -static-libstdc++ -o $asi $src
} else {
    throw 'No C++ compiler found (cl, clang++ or g++). Install VS Build Tools or llvm-mingw.'
}
if ($LASTEXITCODE -ne 0) { throw "Build failed ($LASTEXITCODE)" }
Write-Host "Built $asi"

# External monitor (reads the shared memory the ASI publishes)
if (Test-Path $local) {
    $mon = Join-Path $out 'GyroAimMonitor.exe'
    & $local -std=c++17 -O2 -Wall -static -o $mon (Join-Path $root 'tools\monitor.cpp') -lwinmm
    if ($LASTEXITCODE -ne 0) { throw "Monitor build failed ($LASTEXITCODE)" }
    Write-Host "Built $mon"
}
