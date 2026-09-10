$ErrorActionPreference = 'Stop'
$sourceRoot = 'D:\Codex\Workspaces\QQSuperCompression-1.1.8-DarkMaterial'
$buildRoot = 'D:\Codex\Temp\QQSuperCompression-1.1.8-DarkMaterial'
$outputRoot = 'D:\Codex\Outputs\QQ Super Compression 1.1.8 Rev2 Pointer Plan A'
$buildBundle = Join-Path $buildRoot 'QQSuperCompression_artefacts\Release\VST3\QQ Super Compression.vst3'
$installedBundle = 'C:\Program Files\Common Files\VST3\QQ Super Compression.vst3'
$binaryRelative = 'Contents\x86_64-win\QQ Super Compression.vst3'
$moduleRelative = 'Contents\Resources\moduleinfo.json'
$expected = '7509DB02BADAAF10CDF72DDB54215C76EBF483B8A469AA5B1C4DCAB01F08F5EA'
$prior = '7301F20BD7CDD68CB33444E71A9FB9CB5D4ED43784EC7B311A93F2E9756E1731'
function HashOf([string]$path) { (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash }
function CheckHosts {
    $hosts = @(Get-Process | Where-Object { $_.ProcessName -match 'Cubase|Nuendo|REAPER|Ableton|Studio One|FL64|Bitwig|WaveLab|PluginDoctor|AudioPluginHost' })
    if ($hosts.Count -gt 0) { throw ('Close hosts before install: ' + (($hosts | ForEach-Object ProcessName) -join ', ')) }
}
if (Test-Path -LiteralPath $outputRoot) { throw 'Output already exists; inspect before retrying, do not overwrite blindly.' }
if ((HashOf (Join-Path $buildBundle $binaryRelative)) -ne $expected) { throw 'Unexpected built binary.' }
New-Item -ItemType Directory -Path $outputRoot | Out-Null
Copy-Item -LiteralPath $buildBundle -Destination $outputRoot -Recurse
$deliveredBundle = Join-Path $outputRoot 'QQ Super Compression.vst3'
foreach ($relative in @($binaryRelative, $moduleRelative)) {
    if ((HashOf (Join-Path $buildBundle $relative)) -ne (HashOf (Join-Path $deliveredBundle $relative))) { throw 'Delivery hash mismatch.' }
}
$preview = Join-Path $outputRoot 'Preview'
New-Item -ItemType Directory -Path $preview | Out-Null
foreach ($name in @('dark-ST.png','dark-LR.png','dark-MS.png','dark-sidechain.png','dark-bottom.png','dark-bottom-2x.png',
                    'warm-ST.png','warm-LR.png','warm-MS.png','warm-sidechain.png','warm-bottom.png','warm-bottom-2x.png',
                    'classic-ST.png','dark-control-detail.png','warm-control-detail.png')) {
    Copy-Item -LiteralPath (Join-Path "$buildRoot\ValidationRev2" $name) -Destination $preview
}
Copy-Item -LiteralPath "$sourceRoot\LICENSE", "$sourceRoot\PLAN_A_VERIFICATION_1.1.8_REV2.md" -Destination $outputRoot
$zip = Join-Path $outputRoot 'QQ Super Compression 1.1.8 Rev2 Windows VST3.zip'
Compress-Archive -LiteralPath $deliveredBundle -DestinationPath $zip -CompressionLevel Optimal
Write-Output ('ZIP SHA256: ' + (HashOf $zip))
CheckHosts
if ((HashOf (Join-Path $installedBundle $binaryRelative)) -ne $prior) { throw 'Installed baseline changed; stop before overwrite.' }
$rollbackRoot = Join-Path $outputRoot 'Rollback - 1.1.8 Revision 1'
New-Item -ItemType Directory -Path $rollbackRoot | Out-Null
Copy-Item -LiteralPath $installedBundle -Destination $rollbackRoot -Recurse
$rollbackBundle = Join-Path $rollbackRoot 'QQ Super Compression.vst3'
foreach ($relative in @($binaryRelative, $moduleRelative)) {
    if ((HashOf (Join-Path $installedBundle $relative)) -ne (HashOf (Join-Path $rollbackBundle $relative))) { throw 'Rollback hash mismatch.' }
}
CheckHosts
foreach ($relative in @($binaryRelative, $moduleRelative)) {
    Copy-Item -LiteralPath (Join-Path $deliveredBundle $relative) -Destination (Join-Path $installedBundle $relative) -Force
}
foreach ($relative in @($binaryRelative, $moduleRelative)) {
    if ((HashOf (Join-Path $deliveredBundle $relative)) -ne (HashOf (Join-Path $installedBundle $relative))) { throw 'Installation hash mismatch.' }
}
Write-Output 'PASS: build/output/install hash parity for both files; prior installed Revision 1 copied and verified; no host at install.'
Write-Output ('Output: ' + $outputRoot)
