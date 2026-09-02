$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$simDir = Join-Path $repoRoot "DeskBot_demo\bin-sim"
$simExe = Join-Path $simDir "main.exe"
$msysBin = "C:\msys64\ucrt64\bin"

if (-not (Test-Path -LiteralPath $simExe)) {
    throw "找不到模拟器程序，请先完成 Windows 模拟器编译：$simExe"
}

if (Test-Path -LiteralPath $msysBin) {
    $env:Path = "$msysBin;$env:Path"
}

Push-Location $simDir
try {
    & .\main.exe
    exit $LASTEXITCODE
}
finally {
    Pop-Location
}
