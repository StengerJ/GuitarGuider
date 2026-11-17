<#
.SYNOPSIS
  Builds GuitarGuider with MSVC, installs it to %LOCALAPPDATA%\Programs\GuitarGuider
  and puts a shortcut on the desktop.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File scripts\build-windows.ps1
  powershell -ExecutionPolicy Bypass -File scripts\build-windows.ps1 -NoInstall -RunTests
#>
param(
    [string]$BuildDir = "build",
    [string]$Config = "Release",
    [string]$JuceDir = "",          # optional local JUCE checkout (skips the download)
    [switch]$NoInstall,
    [switch]$NoShortcut,
    [switch]$RunTests
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
if (-not [System.IO.Path]::IsPathRooted($BuildDir)) { $BuildDir = Join-Path $root $BuildDir }

# --- Locate Visual Studio (any edition, including Build Tools) ---------------------------
$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw "Visual Studio (or Build Tools) with the C++ workload is required." }
$vs = & $vswhere -latest -prerelease -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) {
    # Fall back to any install that ships vcvars64.bat
    $vs = & $vswhere -all -prerelease -products * -property installationPath |
          Where-Object { Test-Path (Join-Path $_ "VC\Auxiliary\Build\vcvars64.bat") } | Select-Object -First 1
}
if (-not $vs) { throw "No Visual Studio installation with the C++ x64 tools was found." }
$vcvars = Join-Path $vs "VC\Auxiliary\Build\vcvars64.bat"

# --- Locate CMake and Ninja ----------------------------------------------------------------
function Find-Tool($name, $fallbacks) {
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    foreach ($f in $fallbacks) { if (Test-Path $f) { return $f } }
    return $null
}
$vsCmakeDir = Join-Path $vs "Common7\IDE\CommonExtensions\Microsoft\CMake"
$cmake = Find-Tool "cmake" @((Join-Path $vsCmakeDir "CMake\bin\cmake.exe"), "C:\Program Files\CMake\bin\cmake.exe")
if (-not $cmake) { throw "CMake not found. Install it (winget install Kitware.CMake) or add the VS 'C++ CMake tools' component." }
$ninja = Find-Tool "ninja" @((Join-Path $vsCmakeDir "Ninja\ninja.exe"))
$generator = if ($ninja) { "Ninja" } else { "NMake Makefiles" }

$configureArgs = @("-S", ".", "-B", $BuildDir, "-G", $generator, "-DCMAKE_BUILD_TYPE=$Config")
if ($ninja) { $configureArgs += "-DCMAKE_MAKE_PROGRAM=$ninja" }
if ($JuceDir) { $configureArgs += "-DFETCHCONTENT_SOURCE_DIR_JUCE=$JuceDir" }

$quoted = ($configureArgs | ForEach-Object { '"' + $_ + '"' }) -join " "
$installerDir = Split-Path -Parent $vswhere
$cmd = "set `"PATH=$installerDir;%PATH%`" && call `"$vcvars`" >nul && `"$cmake`" $quoted && `"$cmake`" --build `"$BuildDir`" --config $Config"
Write-Host "Building GuitarGuider ($Config, $generator)..." -ForegroundColor Cyan
$ErrorActionPreference = "Continue"
cmd /c $cmd
$buildResult = $LASTEXITCODE
$ErrorActionPreference = "Stop"
if ($buildResult -ne 0) { throw "Build failed." }

$exe = Join-Path $BuildDir "GuitarGuider.exe"
Write-Host "Built $exe" -ForegroundColor Green

if ($RunTests) {
    & (Join-Path $BuildDir "GuitarGuiderTests.exe")
    if ($LASTEXITCODE -ne 0) { throw "Unit tests failed." }
}

if ($NoInstall) { return }

# --- Install + desktop shortcut ---------------------------------------------------------------
$dest = Join-Path $env:LOCALAPPDATA "Programs\GuitarGuider"
New-Item -ItemType Directory -Force $dest | Out-Null
Copy-Item $exe $dest -Force
Copy-Item (Join-Path $root "assets\GuitarGuider.ico") $dest -Force
Write-Host "Installed to $dest" -ForegroundColor Green

if (-not $NoShortcut) {
    $desktop = [Environment]::GetFolderPath("Desktop")
    $shell = New-Object -ComObject WScript.Shell
    $lnk = $shell.CreateShortcut((Join-Path $desktop "GuitarGuider.lnk"))
    $lnk.TargetPath = Join-Path $dest "GuitarGuider.exe"
    $lnk.WorkingDirectory = $dest
    $lnk.IconLocation = Join-Path $dest "GuitarGuider.ico"
    $lnk.Description = "Learn chords and practise through an amp"
    $lnk.Save()
    Write-Host "Desktop shortcut created." -ForegroundColor Green
}
