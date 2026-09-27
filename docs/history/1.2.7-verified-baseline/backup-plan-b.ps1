$ErrorActionPreference = 'Stop'
$sourceRoot = $PSScriptRoot
$formalRoot = 'D:\备份文件\Vibe Coding\QQ Super Compression\源代码'
$dependencyRoot = 'D:\Codex\Temp\JUCE-8.0.15-src\JUCE-8.0.15'
$documentsOrigin = 'D:\Codex\Workspaces\QQSuperCompression-1.2.7-PlanCD\docs'
$baselineBundle = 'D:\Codex\Outputs\QQ Super Compression\1.2.7 Strict Output Link Checkpoint\Win\QQ Super Compression.vst3'
$installedBundle = 'C:\Program Files\Common Files\VST3\QQ Super Compression.vst3'
$uiEvidence = 'D:\Codex\Temp\QQSC-1.2.7-Strict-Output-Link\verification\final-ui'
$artifact = Get-Content -LiteralPath (Join-Path $sourceRoot 'CANDIDATE_ARTIFACTS.json') -Raw | ConvertFrom-Json
$snapshotName = 'QQ Super Compression 1.2.7-PlanB-' + (Get-Date -Format 'yyyyMMdd-HHmmss')
$snapshot = Join-Path $formalRoot $snapshotName
if (-not (Test-Path -LiteralPath $formalRoot -PathType Container)) { throw 'Formal backup root does not exist.' }
if (Test-Path -LiteralPath $snapshot) { throw 'Snapshot already exists; never overwrite an earlier backup.' }

function Get-Sha([string] $path) { (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash }
function Relative-Path([string] $root, [string] $file) { $file.Substring($root.TrimEnd('\').Length + 1).Replace('\', '/') }

$parity = @()
foreach ($bundle in @($artifact.build, $artifact.delivery, $installedBundle)) {
    $files = @(Get-ChildItem -LiteralPath $bundle -File -Force -Recurse)
    if ($files.Count -ne $artifact.files.Count) { throw "Unexpected bundle inventory: $bundle" }
    foreach ($f in $artifact.files) {
        $p = Join-Path $bundle $f.path
        $hash = Get-Sha $p
        if ($hash -ne $f.sha256 -or (Get-Item -LiteralPath $p).Length -ne $f.length) { throw "Artifact mismatch: $p" }
        $parity += [ordered]@{ Bundle=$bundle; File=$f.path; SHA256=$hash }
    }
}
$baselineDll = Join-Path $baselineBundle 'Contents\x86_64-win\QQ Super Compression.vst3'
if ((Get-Sha $baselineDll) -ne 'EB88D85D88518AF93B9E19D15895E225A10538BF94F9DA300103B968209576D7') { throw 'Wrong compatibility baseline.' }

$testLogs = @('final-ui-check.log','final-unity-check.log','final-lufs-check.log','final-vst-check.log')
foreach ($name in $testLogs) {
    $log = Get-Content -LiteralPath (Join-Path $sourceRoot $name) -Raw
    if ($log -notmatch 'PASS:' -or $log -match '(?m)^FAIL(?:ED)?[: ]') { throw "Final test log is not successful: $name" }
}
$documentFiles = @(Get-ChildItem -LiteralPath $documentsOrigin -Force -Recurse -File)
foreach ($f in $documentFiles) {
    $relative = Relative-Path $documentsOrigin $f.FullName
    $copy = Join-Path (Join-Path $sourceRoot 'docs') $relative
    if ((Get-Sha $f.FullName) -ne (Get-Sha $copy)) { throw "Preserved document mismatch: $relative" }
}

$location = [ordered]@{
    Version='1.2.7'; RequestedStage='Plan B'; Source=$sourceRoot; Snapshot=$snapshot
    VerificationReport=(Join-Path $snapshot 'PLAN_B_VERIFICATION.md')
    Note='The snapshot report is authoritative for completion. No public release or new Stable designation is implied.'
}
$location | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $sourceRoot 'PLAN_B_LOCATION.json') -Encoding utf8

$maps = @(
    @{ Origin=$sourceRoot; Destination='Source' },
    @{ Origin=$dependencyRoot; Destination='Dependencies/JUCE-8.0.15' },
    @{ Origin=$artifact.delivery; Destination='Win/QQ Super Compression.vst3' },
    @{ Origin=$baselineBundle; Destination='Verification/BaselineVST3/QQ Super Compression.vst3' }
)
$payload = [System.Collections.Generic.List[object]]::new()
foreach ($map in $maps) {
    $items = @(Get-ChildItem -LiteralPath $map.Origin -Force -Recurse)
    if (@($items | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint }).Count -gt 0) { throw "Unexpected reparse point: $($map.Origin)" }
    foreach ($f in @($items | Where-Object { -not $_.PSIsContainer })) {
        $relative = $map.Destination + '/' + (Relative-Path $map.Origin $f.FullName)
        $payload.Add([pscustomobject]@{ Path=$relative; Origin=$f.FullName; Bytes=$f.Length; SHA256=(Get-Sha $f.FullName) })
    }
}
foreach ($f in @(Get-ChildItem -LiteralPath $uiEvidence -File -Filter 'algorithms-*.png')) {
    $payload.Add([pscustomobject]@{ Path=('Verification/UI/' + $f.Name); Origin=$f.FullName; Bytes=$f.Length; SHA256=(Get-Sha $f.FullName) })
}
if (@($payload | Group-Object Path | Where-Object Count -gt 1).Count) { throw 'Duplicate payload paths.' }
[IO.Directory]::CreateDirectory($snapshot) | Out-Null
foreach ($f in $payload) {
    $destination = Join-Path $snapshot $f.Path
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination)) | Out-Null
    [IO.File]::Copy($f.Origin, $destination, $false)
}

$destinationFiles = @(Get-ChildItem -LiteralPath $snapshot -Force -Recurse -File)
if ($destinationFiles.Count -ne $payload.Count) { throw 'Copied payload count mismatch.' }
foreach ($f in $payload) {
    $destination = Join-Path $snapshot $f.Path
    if ((Get-Item -LiteralPath $destination).Length -ne $f.Bytes -or (Get-Sha $destination) -ne $f.SHA256) { throw "Copied payload hash mismatch: $($f.Path)" }
    if ((Get-Sha $f.Origin) -ne $f.SHA256) { throw "Source changed during backup: $($f.Origin)" }
}

$manifestPath = Join-Path $snapshot 'PLAN_B_MANIFEST.sha256'
$lines = @($payload | Sort-Object Path | ForEach-Object { $_.SHA256 + '  ' + $_.Path })
[IO.File]::WriteAllLines($manifestPath, $lines, [Text.UTF8Encoding]::new($false))
$manifestHash = Get-Sha $manifestPath
$sourceFiles = @($payload | Where-Object { $_.Path.StartsWith('Source/') })
$dependencyFiles = @($payload | Where-Object { $_.Path.StartsWith('Dependencies/') })
$keys = @('Source/CMakeLists.txt','Source/Source/PluginProcessor.cpp','Source/Source/PluginEditor.cpp','Source/Source/LimiterEditor.cpp','Source/Source/Parameters.h','Source/Source/ABTransfer.h','Source/Source/StaticCompressionEngine.h','Source/LICENSE','Source/docs/manuals/QQ-Super-Compression-1.2.7-User-Manual-Chinese.pdf','Source/docs/manuals/QQ-Super-Compression-1.2.7-User-Manual-English.pdf','Win/QQ Super Compression.vst3/Contents/x86_64-win/QQ Super Compression.vst3')
$record = [ordered]@{
    Status='COMPLETE'; Version='1.2.7'; Stage='Plan B'; CompletedAt=(Get-Date -Format o)
    Source=$sourceRoot; Snapshot=$snapshot
    SourceFiles=$sourceFiles.Count; SourceBytes=[long](($sourceFiles | Measure-Object Bytes -Sum).Sum)
    DependencyFiles=$dependencyFiles.Count; DependencyBytes=[long](($dependencyFiles | Measure-Object Bytes -Sum).Sum)
    PayloadFiles=$payload.Count; PayloadBytes=[long](($payload | Measure-Object Bytes -Sum).Sum)
    AllPayloadHashesVerified=$true; SourceUnchangedDuringCopy=$true
    BuildDeliveryInstalledParity=$true; WindowsSHA256='B9E073E7283AB4C4288FB646FE516866CFE8EC04E27142F3C12EA9CAA78D5704'
    PreservedDocumentsOrigin=$documentsOrigin; PreservedDocumentsFiles=$documentFiles.Count; PreservedDocumentsParity=$true
    ManualsStatus='Preserved previous bilingual 1.2.7 manuals; not revised for this checkpoint.'
    Verification='Reused successful final build/regression results from 2026-09-28; fresh artifact and source-copy checks. No production code edits in Plan B.'
    FinalTestLogs=$testLogs; BundleChecks=$parity
    ManifestSHA256=$manifestHash
    KeyFiles=@($payload | Where-Object { $keys -contains $_.Path } | Select-Object Path,Bytes,SHA256)
    Scope='Local verified source/dependency/document/Windows snapshot only. No new Stable claim, desktop handoff or publication.'
}
$record | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $snapshot 'PLAN_B_VERIFICATION.json') -Encoding utf8
$report = @"
# QQ Super Compression 1.2.7 - Plan B verification

Status: COMPLETE
Completed: $($record.CompletedAt)

- Active source: $sourceRoot
- Formal snapshot: $snapshot
- Source: $($record.SourceFiles) files / $($record.SourceBytes) bytes.
- Bundled JUCE 8.0.15: $($record.DependencyFiles) files / $($record.DependencyBytes) bytes.
- Entire payload: $($record.PayloadFiles) files / $($record.PayloadBytes) bytes.
- Every copied file was checked by size and SHA-256 against its origin; origins were checked again to detect concurrent changes.
- Build, delivered and installed VST3 hashes match the preserved Win bundle.
- Windows DLL SHA-256: $($record.WindowsSHA256)
- Payload manifest SHA-256: $manifestHash

## Contents and validation

Source includes all production code, assets, tests, build scripts, workflows, licences and preserved documentation. Dependencies contains the exact local JUCE tree used by the build. Win contains the installed Windows product. Verification contains the strict-Output-only baseline VST3 for compatibility tests and actual editor screenshots.

The four final regression logs passed before this Plan B: 3,120 unrelated control edits preserve Output; 600 display projections and 48 processor renders cover branch algorithms; 18 editor layouts; 10 ms crossfades at three sample rates; A/B and four-bank migration; MATCH/monitor/LUFS checks; 120 normal and 24 Limiter actual VST3 comparisons with zero residual. See Source/VERIFICATION_FINAL.md and the final logs. Intermediate logs in Source are retained as development history, not final results.

The prior 35-page bilingual manuals and editable files are preserved byte for byte ($($record.PreservedDocumentsFiles) files), but do not yet document the latest changes. See Source/docs/CURRENT_DOCUMENT_STATUS.md. Build instructions are in Source/docs/BUILD_PLAN_B.md.

The manifest covers the payload only. PLAN_B_MANIFEST.sha256 and the generated PLAN_B_VERIFICATION.md/json reports are outside the payload count to avoid self-referential checksums. PLAN_B_VERIFICATION.json records key source hashes and full artifact parity evidence.

This is the requested local Plan B checkpoint. No new Stable designation, GitHub publication, desktop package, manual revision or cloud-sync completion is claimed.
"@
[IO.File]::WriteAllText((Join-Path $snapshot 'PLAN_B_VERIFICATION.md'), $report, [Text.UTF8Encoding]::new($false))

$finalFiles = @(Get-ChildItem -LiteralPath $snapshot -Force -File -Recurse)
if ($finalFiles.Count -ne $payload.Count + 3) { throw 'Final snapshot inventory mismatch.' }
$readback = Get-Content -LiteralPath (Join-Path $snapshot 'PLAN_B_VERIFICATION.json') -Raw | ConvertFrom-Json
if ($readback.Status -ne 'COMPLETE' -or (Get-Sha $manifestPath) -ne $readback.ManifestSHA256) { throw 'Final report verification failed.' }
[ordered]@{ Status='COMPLETE'; Snapshot=$snapshot; SourceFiles=$record.SourceFiles; SourceBytes=$record.SourceBytes; PayloadFiles=$record.PayloadFiles; PayloadBytes=$record.PayloadBytes; TotalSnapshotFiles=$finalFiles.Count; TotalSnapshotBytes=[long](($finalFiles | Measure-Object Length -Sum).Sum); WindowsSHA256=$record.WindowsSHA256; ManifestSHA256=$manifestHash } | ConvertTo-Json
