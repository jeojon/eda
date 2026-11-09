param(
    [string]$WxRoot = "D:\wxWeigets\wxWidgets-3.3.3",
    [string]$VcVars = "E:\VS\VC\Auxiliary\Build\vcvars64.bat",
    [string]$Ninja = "E:\VS\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath (Join-Path $WxRoot "include\wx\wx.h"))) {
    throw "wxWidgets root not found: $WxRoot"
}
if (-not (Test-Path -LiteralPath $VcVars)) {
    throw "Visual C++ environment script not found: $VcVars"
}
if (-not (Test-Path -LiteralPath $Ninja)) {
    $ninjaCommand = Get-Command ninja.exe -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -eq $ninjaCommand) {
        throw "Ninja was not found."
    }
    $Ninja = $ninjaCommand.Source
}

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildRoot = Join-Path $projectRoot "build-msvc-userwx"

$command = "call `"$VcVars`" && " +
    "cmake -S `"$projectRoot`" -B `"$buildRoot`" -G Ninja " +
    "-DCMAKE_MAKE_PROGRAM=`"$Ninja`" -DCMAKE_BUILD_TYPE=Release " +
    "-DwxWidgets_ROOT_DIR=`"$WxRoot`" && " +
    "cmake --build `"$buildRoot`" --parallel 8 && " +
    "ctest --test-dir `"$buildRoot`" --output-on-failure"

& cmd.exe /d /s /c $command
if ($LASTEXITCODE -ne 0) {
    throw "MSVC build failed with exit code $LASTEXITCODE."
}

Write-Host ""
Write-Host "Built: $buildRoot\EdaLab1.exe"
