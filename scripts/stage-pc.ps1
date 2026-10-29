<#
.SYNOPSIS
Stages an already-built direct-launch PC game without the launcher or build tools.
#>
[CmdletBinding()]
param(
    [string]$BuildDir = '',
    [string]$OutputDir = '',
    [string]$GameDataRoot = '',
    [switch]$IncludeConfig
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $BuildDir) { $BuildDir = Join-Path $root 'targets/mkvsdcu/private/rexglue-host/out/build/win-amd64-release' }
if (-not $OutputDir) { $OutputDir = Join-Path $root 'dist/MKVSDC-PC' }
$BuildDir = [IO.Path]::GetFullPath($BuildDir)
$OutputDir = [IO.Path]::GetFullPath($OutputDir)

function Test-Overlap([string]$First, [string]$Second) {
    $a = $First.TrimEnd([char[]]'\/')
    $b = $Second.TrimEnd([char[]]'\/')
    $separator = [IO.Path]::DirectorySeparatorChar
    return $a.Equals($b, [StringComparison]::OrdinalIgnoreCase) -or
        $a.StartsWith($b + $separator, [StringComparison]::OrdinalIgnoreCase) -or
        $b.StartsWith($a + $separator, [StringComparison]::OrdinalIgnoreCase)
}
if (Test-Overlap $BuildDir $OutputDir) {
    throw 'Keep the runtime destination separate from the build directory.'
}

# Never merge a new distribution into someone's game, settings, or mod files.
# Use another destination for each package; no destructive -Force mode.
if (Test-Path -LiteralPath $OutputDir) {
    throw "Output already exists: $OutputDir. Choose a new -OutputDir."
}
foreach ($name in @('mkvsdcu.exe', 'rexruntime.dll', 'rexgpu-xenos.dll')) {
    if (-not (Test-Path -LiteralPath (Join-Path $BuildDir $name) -PathType Leaf)) {
        throw "Missing runtime file: $name. Build with scripts/build-pc.ps1 first."
    }
}
if ($GameDataRoot) {
    $GameDataRoot = (Resolve-Path -LiteralPath $GameDataRoot).Path
    if (Test-Overlap $GameDataRoot $OutputDir) {
        throw 'Keep the runtime destination separate from the source game directory.'
    }
    $xex = Join-Path $GameDataRoot 'default.xex'
    if (-not (Test-Path -LiteralPath $xex -PathType Leaf) -or
        (Get-FileHash -LiteralPath $xex -Algorithm SHA256).Hash -ne
        '2955F2E2BE61EC1948CD2FD3538AD45BEB04772F5EBD5E0FB1BFDE484748E5A7') {
        throw 'Game files must contain the supported original retail default.xex.'
    }
    foreach ($name in @('Asset', 'Config', 'Localization', 'Movies')) {
        if (-not (Test-Path -LiteralPath (Join-Path $GameDataRoot $name) -PathType Container)) {
            throw "Incomplete game folder: missing $name."
        }
    }
}
if ($IncludeConfig -and -not (Test-Path -LiteralPath (Join-Path $root 'config/mkvsdcu.toml') -PathType Leaf)) {
    throw 'Missing config/mkvsdcu.toml template.'
}

$parent = Split-Path -Parent $OutputDir
New-Item -ItemType Directory -Force -Path $parent | Out-Null
$stage = Join-Path $parent ('.mkvsdcu-stage-' + [Guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory -Path $stage | Out-Null
    Copy-Item -LiteralPath (Join-Path $BuildDir 'mkvsdcu.exe') -Destination $stage
    # CMake colocates linked DLLs, the Xenos plugin, and FidelityFX DLLs here.
    # Include all runtime DLLs, but no SDK CLI, import libs, caches, or sources.
    Get-ChildItem -LiteralPath $BuildDir -File -Filter '*.dll' | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $stage
    }
    $game = Join-Path $stage 'game'
    New-Item -ItemType Directory -Path $game, (Join-Path $stage 'mods') | Out-Null
    if ($GameDataRoot) {
        Get-ChildItem -LiteralPath $GameDataRoot -Force | ForEach-Object {
            Copy-Item -LiteralPath $_.FullName -Destination $game -Recurse
        }
    }
    if ($IncludeConfig) {
        Copy-Item -LiteralPath (Join-Path $root 'config/mkvsdcu.toml') -Destination $stage
    }
    # Directory.Move fails instead of nesting into an existing destination if
    # another process creates OutputDir during staging.
    [IO.Directory]::Move($stage, $OutputDir)
} finally {
    if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
}
Write-Host "[OK] Staged direct-launch runtime: $OutputDir"
if (-not $GameDataRoot) { Write-Host 'Copy your original retail game files into game/ before playing.' }
Write-Host 'No launcher, release manifest, SDK CLI, or compiler is included.'
