# git apply --check returning nonzero is an expected probe result. In Windows
# PowerShell 5.1, redirected native stderr can otherwise terminate the script
# under ErrorActionPreference=Stop before the reverse check runs.
function Test-SdkPatch {
    param(
        [Parameter(Mandatory)][string]$Git,
        [Parameter(Mandatory)][string]$Sdk,
        [Parameter(Mandatory)][string]$Patch,
        [switch]$Reverse
    )
    # Function-local preferences: do not relax errors in the build itself.
    $ErrorActionPreference = 'Continue'
    $PSNativeCommandUseErrorActionPreference = $false
    $arguments = @('-C', $Sdk, 'apply', '--check')
    if ($Reverse) { $arguments += '--reverse' }
    $arguments += $Patch
    & $Git @arguments *> $null
    return ($LASTEXITCODE -eq 0)
}
