# Requires a Windows native Clang toolchain with MSVC/Windows SDK headers.
# Exercises the production CNG implementation without ReXGlue or game files.
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$source = Join-Path $root 'targets/mkvsdcu/private/rexglue-host/src'
$compiler = (Get-Command clang++.exe -ErrorAction Stop).Source
$temporary = Join-Path ([IO.Path]::GetTempPath()) ('mkvsdcu-test-' + [Guid]::NewGuid().ToString('N'))
try {
    $fixtures = Join-Path $temporary 'fixtures'
    New-Item -ItemType Directory -Path $fixtures | Out-Null
    $exe = Join-Path $temporary 'direct-launch-test.exe'
    & $compiler -std=c++20 -Wall -Wextra -Werror "-I$source" `
        (Join-Path $root 'tests/direct_launch_test.cpp') `
        (Join-Path $source 'direct_launch.cpp') (Join-Path $source 'direct_launch_win.cpp') `
        -lbcrypt -o $exe
    if ($LASTEXITCODE -ne 0) { throw 'Direct-launch test compilation failed.' }
    & $exe $fixtures
    if ($LASTEXITCODE -ne 0) { throw 'Direct-launch tests failed.' }
} finally {
    if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary -Recurse -Force }
}
