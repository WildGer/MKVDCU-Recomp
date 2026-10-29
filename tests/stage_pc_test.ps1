[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$script = Join-Path $root 'scripts/stage-pc.ps1'
$temporary = Join-Path ([IO.Path]::GetTempPath()) ('mkvsdcu-stage-test-' + [Guid]::NewGuid().ToString('N'))
$script:checks = 0
function Check([bool]$Condition, [string]$Label) {
    if (-not $Condition) { throw "FAIL: $Label" }
    $script:checks++
}
function Expect-Failure([scriptblock]$Action, [string]$Label) {
    $failed = $false
    try { & $Action } catch { $failed = $true }
    Check $failed $Label
}
try {
    $build = Join-Path $temporary 'developer build'
    New-Item -ItemType Directory -Path $build | Out-Null
    # Deliberately fake binaries: this tests staging isolation, not gameplay.
    foreach ($name in @('mkvsdcu.exe', 'rexruntime.dll', 'rexgpu-xenos.dll', 'amd_fidelityfx_dx12.dll',
                        'rexglue.exe', 'CMakeCache.txt', 'port.json', 'play.log', 'rexruntime.lib')) {
        Set-Content -LiteralPath (Join-Path $build $name) -Value "fixture $name"
    }
    $output = Join-Path $temporary 'MKVSDC-PC'
    & $script -BuildDir $build -OutputDir $output
    foreach ($name in @('mkvsdcu.exe', 'rexruntime.dll', 'rexgpu-xenos.dll', 'amd_fidelityfx_dx12.dll', 'game', 'mods')) {
        Check (Test-Path -LiteralPath (Join-Path $output $name)) "runtime includes $name"
    }
    foreach ($name in @('mkvsdcu.toml', 'rexglue.exe', 'CMakeCache.txt', 'port.json', 'play.log', 'rexruntime.lib')) {
        Check (-not (Test-Path -LiteralPath (Join-Path $output $name))) "runtime excludes $name"
    }
    Check (@(Get-ChildItem -LiteralPath (Join-Path $output 'game') -Force).Count -eq 0) 'game data never bundled implicitly'
    $withConfig = Join-Path $temporary 'with config'
    & $script -BuildDir $build -OutputDir $withConfig -IncludeConfig
    Check ((Get-Content -LiteralPath (Join-Path $withConfig 'mkvsdcu.toml') -Raw) -eq
           (Get-Content -LiteralPath (Join-Path $root 'config/mkvsdcu.toml') -Raw)) 'optional clean config template'
    $sentinel = Join-Path $output 'mods/user.txt'
    Set-Content -LiteralPath $sentinel -Value 'preserve me'
    Expect-Failure { & $script -BuildDir $build -OutputDir $output } 'existing output rejected'
    Check ((Get-Content -LiteralPath $sentinel -Raw).Trim() -eq 'preserve me') 'user mods preserved'
    Expect-Failure { & $script -BuildDir $build -OutputDir (Join-Path $build 'runtime') } 'nested build destination rejected'
    $badGame = Join-Path $temporary 'bad game'
    New-Item -ItemType Directory -Path $badGame | Out-Null
    Set-Content -LiteralPath (Join-Path $badGame 'default.xex') -Value 'XEX2 is not sufficient'
    $badOutput = Join-Path $temporary 'bad output'
    Expect-Failure { & $script -BuildDir $build -OutputDir $badOutput -GameDataRoot $badGame } 'wrong XEX hash rejected'
    Check (-not (Test-Path -LiteralPath $badOutput)) 'validation failure leaves no runtime output'
    Remove-Item -LiteralPath (Join-Path $build 'rexgpu-xenos.dll')
    Expect-Failure { & $script -BuildDir $build -OutputDir $badOutput } 'missing GPU plugin rejected'
    Check (@(Get-ChildItem -LiteralPath $temporary -Filter '.mkvsdcu-stage-*' -Force).Count -eq 0) 'no temporary staging residue'
    Write-Host "$script:checks staging checks passed"
} finally {
    if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary -Recurse -Force }
}
