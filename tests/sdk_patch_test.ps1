$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $true
. (Join-Path (Split-Path -Parent $PSScriptRoot) 'scripts/sdk-patch.ps1')
$git = (Get-Command git -ErrorAction Stop).Source
$temporary = Join-Path ([IO.Path]::GetTempPath()) ('sdk-patch-test-' + [Guid]::NewGuid().ToString('N'))
$checks = 0
function Check([bool]$Condition, [string]$Label) {
    if (-not $Condition) { throw "FAIL: $Label" }
    $script:checks++
}
try {
    New-Item -ItemType Directory -Path $temporary | Out-Null
    & $git -C $temporary init --quiet
    if ($LASTEXITCODE -ne 0) { throw 'Fixture Git init failed.' }
    $source = Join-Path $temporary 'source.txt'
    $patch = Join-Path $temporary 'fixture.patch'
    $utf8 = [Text.UTF8Encoding]::new($false)
    [IO.File]::WriteAllText($source, "original`n", $utf8)
    [IO.File]::WriteAllText($patch, "diff --git a/source.txt b/source.txt`n--- a/source.txt`n+++ b/source.txt`n@@ -1 +1 @@`n-original`n+patched`n", $utf8)
    Check (Test-SdkPatch -Git $git -Sdk $temporary -Patch $patch) 'fresh patch accepted'
    Check (-not (Test-SdkPatch -Git $git -Sdk $temporary -Patch $patch -Reverse)) 'fresh reverse rejected without throwing'
    & $git -C $temporary apply $patch
    if ($LASTEXITCODE -ne 0) { throw 'Fixture Git apply failed.' }
    Check (-not (Test-SdkPatch -Git $git -Sdk $temporary -Patch $patch)) 'already-applied forward rejected without throwing'
    Check (Test-SdkPatch -Git $git -Sdk $temporary -Patch $patch -Reverse) 'already-applied reverse accepted'
    Check (([IO.File]::ReadAllText($source)) -eq "patched`n") 'probes do not modify applied patch'
    [IO.File]::WriteAllText($source, "local edits`n", $utf8)
    Check (-not (Test-SdkPatch -Git $git -Sdk $temporary -Patch $patch)) 'conflict forward rejected'
    Check (-not (Test-SdkPatch -Git $git -Sdk $temporary -Patch $patch -Reverse)) 'conflict reverse rejected'
    Check (([IO.File]::ReadAllText($source)) -eq "local edits`n") 'conflicting user edits preserved'
    Check ($ErrorActionPreference -eq 'Stop') 'caller error preference preserved'
    Check $PSNativeCommandUseErrorActionPreference 'caller native error preference preserved'
    Write-Host "$checks SDK patch checks passed"
} finally {
    if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary -Recurse -Force }
}
