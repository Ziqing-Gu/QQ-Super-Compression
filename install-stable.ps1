$ErrorActionPreference = 'Stop'
$outputRoot = 'D:\Codex\Outputs\QQ Super Compression\1.2.6 Stable'
$proofRoot = Join-Path $outputRoot 'Verification'
$sourceBundle = Join-Path $outputRoot 'Win\QQ Super Compression.vst3'
$systemRoot = 'C:\Program Files\Common Files\VST3'
$installedBundle = Join-Path $systemRoot 'QQ Super Compression.vst3'
$binaryRelative = 'Contents\x86_64-win\QQ Super Compression.vst3'
$oldHash = 'A88D8781740058C2E74D3C90E880CF59C48313278D7407011F8D2B9DCC014294'
$previewHash = '3E2E2836138D4227D070B4FFFCD2D8C987166B73374F4E144014CD6513FEAEF1'
$previewRelative = 'Contents\x86_64-win\QQ Super Compression Preview.vst3'
$previewTargets = @(
    'C:\Program Files\Common Files\VST3\QQ Super Compression Preview.vst3',
    'D:\Codex\Outputs\QQ Super Compression\1.2.6 Preview\QQ Super Compression Preview.vst3'
)
$hosts = @(Get-Process -ErrorAction SilentlyContinue | Where-Object {
    $_.ProcessName -match 'Cubase|Nuendo|REAPER|Studio One|Ableton|FL64|FL Studio|PluginDoctor|Audacity|Bitwig|QQHost'
})
if ($hosts.Count -gt 0) { throw ('Close audio hosts first: '+($hosts.ProcessName -join ', ')) }
if (Test-Path -LiteralPath (Join-Path $proofRoot 'INSTALL_VERIFICATION.json')) { throw 'Installation already recorded; refusing blind repeat' }
$release = Get-Content -LiteralPath (Join-Path $proofRoot 'RELEASE_INPUTS.json') -Raw | ConvertFrom-Json
if ($release.Status -ne 'VALIDATED_READY_TO_INSTALL' -or $release.Version -ne '1.2.6') { throw 'Missing release validation' }
foreach ($entry in $release.Bundle) {
    if ((Get-FileHash -LiteralPath (Join-Path $sourceBundle $entry.Path)).Hash -ne $entry.SHA256) { throw 'Staged bundle changed' }
}
if ((Get-FileHash -LiteralPath (Join-Path $installedBundle $binaryRelative)).Hash -ne $oldHash) { throw 'Installed formal baseline changed' }
if (@(Get-ChildItem -LiteralPath $installedBundle -File -Recurse).Count -ne 2) { throw 'Unexpected existing formal bundle contents' }

# Resolve and restrict both recursive deletion targets before changing anything.
# No computed path is passed to another shell or batch deletion command.
foreach ($target in $previewTargets) {
    $item = Get-Item -LiteralPath $target
    $resolved = (Resolve-Path -LiteralPath $target).ProviderPath
    if (-not $item.PSIsContainer -or $resolved -ne [IO.Path]::GetFullPath($target)) { throw 'Unexpected Preview target resolution' }
    if ($resolved -notin $previewTargets) { throw 'Preview deletion escaped the two explicitly named bundle paths' }
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw 'Refusing reparse-point bundle' }
    $entries = @(Get-ChildItem -LiteralPath $target -Recurse -Force)
    if (@($entries | Where-Object { ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 }).Count -ne 0) { throw 'Refusing nested reparse point' }
    if (@($entries | Where-Object { -not $_.PSIsContainer }).Count -ne 2) { throw 'Unexpected Preview files; do not recursively delete' }
    if ((Get-FileHash -LiteralPath (Join-Path $target $previewRelative)).Hash -ne $previewHash) { throw 'Preview binary differs from the accepted build' }
}

$rollbackRoot = Join-Path 'D:\Codex\Archives' ('QQSuperCompression-1.2.5-before-1.2.6-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
if (Test-Path -LiteralPath $rollbackRoot) { throw 'Rollback destination exists' }
New-Item -ItemType Directory -Path $rollbackRoot | Out-Null
$rollbackBundle = Join-Path $rollbackRoot 'QQ Super Compression.vst3'
Copy-Item -LiteralPath $installedBundle -Destination $rollbackBundle -Recurse
foreach ($file in Get-ChildItem -LiteralPath $installedBundle -File -Recurse) {
    $relative = $file.FullName.Substring($installedBundle.Length+1)
    if ((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath (Join-Path $rollbackBundle $relative)).Hash) { throw 'Rollback copy failed verification' }
}

try {
    foreach ($entry in $release.Bundle) {
        $target = Join-Path $installedBundle $entry.Path
        Copy-Item -LiteralPath (Join-Path $sourceBundle $entry.Path) -Destination $target -Force
        if ((Get-FileHash -LiteralPath $target).Hash -ne $entry.SHA256) { throw ('Installed bundle mismatch: '+$entry.Path) }
    }
} catch {
    foreach ($file in Get-ChildItem -LiteralPath $rollbackBundle -File -Recurse) {
        $relative = $file.FullName.Substring($rollbackBundle.Length+1)
        Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $installedBundle $relative) -Force
    }
    throw
}

$removed = @()
foreach ($target in $previewTargets) {
    # Recheck the final literal target immediately before recursive removal.
    $resolved = (Resolve-Path -LiteralPath $target).ProviderPath
    if ($resolved -notin $previewTargets -or $resolved -ne [IO.Path]::GetFullPath($target)) { throw 'Preview removal target changed' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
    if (Test-Path -LiteralPath $resolved) { throw 'Preview bundle still exists' }
    $removed += $resolved
}
$newHash = (Get-FileHash -LiteralPath (Join-Path $installedBundle $binaryRelative)).Hash
if ($newHash -ne $release.BinarySHA256) { throw 'Final binary hash mismatch' }
$record = [PSCustomObject]@{
    Status='INSTALLED_STABLE_PREVIEW_REMOVED'
    Version='1.2.6'
    InstalledAt=(Get-Date).ToString('o')
    InstalledBundle=$installedBundle
    OutputBundle=$sourceBundle
    BinarySHA256=$newHash
    Bundle=$release.Bundle
    PreviousBinarySHA256=$oldHash
    PreviousBundleRollback=$rollbackBundle
    RemovedPreviewBundles=$removed
    PreviewSourceAndResearchPreserved=$true
    HostsClosed=$true
    BuildOutputInstalledParity=$true
}
$record | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $proofRoot 'INSTALL_VERIFICATION.json') -Encoding UTF8
$record | ConvertTo-Json -Depth 6
