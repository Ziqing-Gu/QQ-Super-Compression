$ErrorActionPreference = 'Stop'
$buildRoot = 'D:\Codex\Temp\QQSuperCompression-1.2.0-UpDown'
$outputRoot = 'D:\Codex\Outputs\QQ Super Compression 1.2.0 UpDown Experiment'
$sourceRoot = 'D:\Codex\Workspaces\QQSuperCompression-1.2.0-UpDown'
$bundleName = 'QQ Super Compression.vst3'
$buildBundle = Join-Path $buildRoot "QQSuperCompression_artefacts\Release\VST3\$bundleName"
$installedBundle = Join-Path 'C:\Program Files\Common Files\VST3' $bundleName
$binaryRelative = 'Contents\x86_64-win\QQ Super Compression.vst3'
$verification = Join-Path $outputRoot 'Verification'
$encoding = [System.Text.UTF8Encoding]::new($false)
function Inventory([string]$root) {
    @(Get-ChildItem -LiteralPath $root -Recurse -File | ForEach-Object {
        [pscustomobject]@{ RelativePath=$_.FullName.Substring($root.Length).TrimStart('\'); Bytes=$_.Length; SHA256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
    } | Sort-Object RelativePath)
}
function AssertEqual([string]$left,[string]$right) {
    $a=Inventory $left; $b=Inventory $right
    if (($a | ConvertTo-Json -Compress) -cne ($b | ConvertTo-Json -Compress)) { throw "Bundle mismatch: $left vs $right" }
}
function Hosts {
    @(Get-Process | Where-Object { $_.ProcessName -match 'Cubase|Nuendo|REAPER|Ableton|Studio One|FL64|Bitwig|WaveLab|PluginDoctor|AudioPluginHost|QQ.?Host' })
}
if (!(Test-Path -LiteralPath $buildBundle)) { throw 'Missing built bundle' }
$version=(Get-Item -LiteralPath (Join-Path $buildBundle $binaryRelative)).VersionInfo.FileVersion
if ($version -notmatch '^1\.2\.0') { throw "Unexpected binary version $version" }
if (Test-Path -LiteralPath $outputRoot) { throw 'Output exists; inspect before retrying, never overwrite blindly' }
New-Item -ItemType Directory -Path $outputRoot,$verification | Out-Null
Copy-Item -LiteralPath $buildBundle -Destination $outputRoot -Recurse
$deliveredBundle=Join-Path $outputRoot $bundleName
AssertEqual $buildBundle $deliveredBundle
Copy-Item -LiteralPath (Join-Path $sourceRoot 'LICENSE'),(Join-Path $sourceRoot 'EXPERIMENT_1.2.0.md') -Destination $outputRoot
Copy-Item -LiteralPath (Join-Path $buildRoot 'Validation-Final') -Destination (Join-Path $outputRoot 'Preview') -Recurse
$zip=Join-Path $outputRoot 'QQ Super Compression 1.2.0 Windows VST3.zip'
Compress-Archive -LiteralPath $deliveredBundle -DestinationPath $zip -CompressionLevel Optimal
$manifest=Inventory $deliveredBundle
[System.IO.File]::WriteAllText((Join-Path $verification 'BUNDLE_SHA256.json'),($manifest | ConvertTo-Json -Depth 5),$encoding)
$hostsNow=Hosts
if($hostsNow.Count -gt 0) {
    [System.IO.File]::WriteAllText((Join-Path $verification 'INSTALL_PENDING.txt'),('Close hosts: '+(($hostsNow.ProcessName|Sort-Object -Unique)-join ', ')),$encoding)
    Write-Output ('DELIVERED; INSTALL PENDING: '+(($hostsNow.ProcessName|Sort-Object -Unique)-join ', '))
    exit 2
}
$priorVersion='none'
if(Test-Path -LiteralPath $installedBundle) {
    $priorVersion=(Get-Item -LiteralPath (Join-Path $installedBundle $binaryRelative)).VersionInfo.FileVersion
    $rollbackRoot=Join-Path $outputRoot ('Rollback - '+$priorVersion)
    New-Item -ItemType Directory -Path $rollbackRoot | Out-Null
    Copy-Item -LiteralPath $installedBundle -Destination $rollbackRoot -Recurse
    AssertEqual $installedBundle (Join-Path $rollbackRoot $bundleName)
}
if((Hosts).Count -gt 0) { throw 'Host started after rollback capture; installation deferred' }
foreach($file in $manifest) {
    $destination=Join-Path $installedBundle $file.RelativePath
    $parent=Split-Path -Parent $destination
    if(!(Test-Path -LiteralPath $parent)) { New-Item -ItemType Directory -Path $parent | Out-Null }
    Copy-Item -LiteralPath (Join-Path $deliveredBundle $file.RelativePath) -Destination $destination -Force
}
AssertEqual $deliveredBundle $installedBundle
$record=[ordered]@{ Version=$version; PreviousVersion=$priorVersion; Source=$sourceRoot; Build=$buildBundle; Output=$deliveredBundle; Installed=$installedBundle; VerifiedAt=(Get-Date -Format o); Files=$manifest; ZipSHA256=(Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash }
[System.IO.File]::WriteAllText((Join-Path $verification 'INSTALLATION.json'),($record|ConvertTo-Json -Depth 6),$encoding)
Write-Output ($record|ConvertTo-Json -Depth 6)
