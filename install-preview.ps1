$ErrorActionPreference = 'Stop'
$bundleName = 'QQ Super Compression Preview.vst3'
$sourceRoot = 'D:\Codex\Workspaces\QQSuperCompression-1.2.6-Preview'
$builtBundle = Join-Path 'D:\Codex\Temp\QQSC-1.2.6-Preview\QQSuperCompression_artefacts\Release\VST3' $bundleName
$outputRoot = 'D:\Codex\Outputs\QQ Super Compression\1.2.6 Preview'
$outputBundle = Join-Path $outputRoot $bundleName
$installedBundle = Join-Path 'C:\Program Files\Common Files\VST3' $bundleName
$stableBinary = 'C:\Program Files\Common Files\VST3\QQ Super Compression.vst3\Contents\x86_64-win\QQ Super Compression.vst3'
$stableHashExpected = 'A88D8781740058C2E74D3C90E880CF59C48313278D7407011F8D2B9DCC014294'
$binaryRelative = 'Contents\x86_64-win\QQ Super Compression Preview.vst3'
$hashExpected = '3E2E2836138D4227D070B4FFFCD2D8C987166B73374F4E144014CD6513FEAEF1'

$hosts = @(Get-Process -ErrorAction SilentlyContinue | Where-Object {
    $_.ProcessName -match 'Cubase|Nuendo|REAPER|Studio One|Ableton|FL64|FL Studio|PluginDoctor|Audacity|Bitwig|QQHost'
})
if ($hosts.Count -gt 0) { throw ('Close audio hosts before installing: ' + ($hosts.ProcessName -join ', ')) }
if ((Get-FileHash -LiteralPath $stableBinary -Algorithm SHA256).Hash -ne $stableHashExpected) { throw 'Stable baseline hash changed' }
if ((Get-FileHash -LiteralPath (Join-Path $builtBundle $binaryRelative) -Algorithm SHA256).Hash -ne $hashExpected) { throw 'Preview build hash changed; revalidate before installation' }
if (Test-Path -LiteralPath $outputBundle) { throw 'Output bundle already exists; do not overwrite silently' }
if (Test-Path -LiteralPath $installedBundle) { throw 'Installed Preview already exists; do not overwrite silently' }

New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
Copy-Item -LiteralPath $builtBundle -Destination $outputBundle -Recurse
Copy-Item -LiteralPath $builtBundle -Destination $installedBundle -Recurse
Copy-Item -LiteralPath (Join-Path $sourceRoot 'PREVIEW_1.2.6.md') -Destination (Join-Path $outputRoot '测试说明.md')
Copy-Item -LiteralPath (Join-Path $sourceRoot 'LICENSE') -Destination $outputRoot
$verifyRoot = Join-Path $outputRoot 'Verification'
New-Item -ItemType Directory -Path $verifyRoot -Force | Out-Null
foreach ($log in @('dynamics-final','vst','ui','modulation')) {
    Copy-Item -LiteralPath ('D:\Codex\Temp\QQSC-1.2.6-Preview-' + $log + '.log') -Destination $verifyRoot
}
foreach ($report in @('step-results.csv','steady-thd.csv','modulation-results.csv','modulation-summary.json')) {
    Copy-Item -LiteralPath (Join-Path 'D:\Codex\Temp\QQSC-1.2.6-Preview-results' $report) -Destination $verifyRoot
}

$checks = @()
foreach ($file in Get-ChildItem -LiteralPath $builtBundle -File -Recurse) {
    $relative = $file.FullName.Substring($builtBundle.Length + 1)
    $originalHash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
    foreach ($copy in @($outputBundle,$installedBundle)) {
        if ((Get-FileHash -LiteralPath (Join-Path $copy $relative) -Algorithm SHA256).Hash -ne $originalHash) { throw ('Copy hash mismatch: ' + $relative) }
    }
    $checks += [PSCustomObject]@{File=$relative;Bytes=$file.Length;SHA256=$originalHash}
}
$validated = Get-Content -LiteralPath 'D:\Codex\Outputs\QQ Super Compression\1.2.5 Stable\Verification\VALIDATED_INPUTS.json' -Raw | ConvertFrom-Json
foreach ($record in $validated) {
    $file = Join-Path 'D:\Codex\Workspaces\QQSuperCompression-1.2.5-Stable' $record.Path
    if ((Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -ne $record.SHA256) { throw ('Stable source changed: ' + $record.Path) }
}
$stableAfter = (Get-FileHash -LiteralPath $stableBinary -Algorithm SHA256).Hash
if ($stableAfter -ne $stableHashExpected) { throw 'Stable binary changed during Preview install' }
$record = [PSCustomObject]@{
    Status='INSTALLED_INDEPENDENT_PREVIEW_NOT_STABLE'
    Installed=(Get-Date).ToString('o')
    Source=$sourceRoot
    Product='QQ Super Compression Preview'
    Version='1.2.6'
    LookaheadMs=26
    Output=$outputBundle
    InstalledBundle=$installedBundle
    BuildOutputInstalledHashesMatch=$true
    PreviewFiles=$checks
    StableBinarySHA256=$stableAfter
    StableValidatedInputCount=@($validated).Count
    StableValidatedInputsUnchanged=$true
    ModulationTradeoff='See modulation-summary.json and the preview notes; not a zero-distortion guarantee.'
}
$record | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $verifyRoot 'INSTALL_VERIFICATION.json') -Encoding UTF8
$record | ConvertTo-Json -Depth 6
