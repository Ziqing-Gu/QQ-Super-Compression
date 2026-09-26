$ErrorActionPreference='Stop'
$source=$PSScriptRoot
$build='D:\Codex\Temp\QQSC-1.2.5-ThresholdFloor\QQSuperCompression_artefacts\Release\VST3\QQ Super Compression.vst3'
$candidate='D:\Codex\Outputs\QQ Super Compression\1.2.5 Candidate-Classic90'
$logs=@{
    'Dynamics.log'='D:\Codex\Temp\QQSC-1.2.5-dynamics.log'
    'UI.log'='D:\Codex\Temp\QQSC-1.2.5-ui-final.log'
    'VST3-reference.log'='D:\Codex\Temp\QQSC-1.2.5-vst-reference.log'
}
foreach($entry in $logs.GetEnumerator()){
    $last=Get-Content -LiteralPath $entry.Value -Tail 1
    if($last -notmatch '^PASS:'){throw ('Incomplete validation: '+$entry.Value)}
}
if(Test-Path -LiteralPath $candidate){throw 'Candidate output already exists'}
$win=Join-Path $candidate 'Win'
$proof=Join-Path $candidate 'Verification'
New-Item -ItemType Directory -Path $win,$proof | Out-Null
Copy-Item -LiteralPath $build -Destination $win -Recurse
Copy-Item -LiteralPath (Join-Path $source 'TRY_1.2.5.md') -Destination (Join-Path $candidate '试用说明.md')
foreach($entry in $logs.GetEnumerator()){Copy-Item -LiteralPath $entry.Value -Destination (Join-Path $proof $entry.Key)}
Copy-Item -LiteralPath 'D:\Codex\Temp\QQSC-1.2.5-ThresholdFloor\UI-final\Algorithm-Light-1008-Classic.png' -Destination (Join-Path $proof 'Classic.png')
Copy-Item -LiteralPath 'D:\Codex\Temp\QQSC-1.2.5-ThresholdFloor\UI-final\Algorithm-Dark-1008-Super.png' -Destination (Join-Path $proof 'Super.png')
Copy-Item -LiteralPath 'D:\Codex\Temp\QQSC-1.2.5-ThresholdFloor\UI-final\Floor-1-Classic.png' -Destination (Join-Path $proof 'Classic-minimum.png')
Copy-Item -LiteralPath 'D:\Codex\Temp\QQSC-1.2.5-ThresholdFloor\UI-final\Deep-Match-0.png' -Destination (Join-Path $proof 'Deep-Match.png')
$bundle=Join-Path $win 'QQ Super Compression.vst3'
$records=@(Get-ChildItem -LiteralPath $build -File -Recurse | ForEach-Object {
    $relative=[IO.Path]::GetRelativePath($build,$_.FullName)
    $sha=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    if((Get-FileHash -LiteralPath (Join-Path $bundle $relative)).Hash -ne $sha){throw 'Output copy mismatch'}
    [pscustomobject]@{Path=$relative;Bytes=$_.Length;SHA256=$sha}
})
$dll=Get-Item -LiteralPath (Join-Path $bundle 'Contents\x86_64-win\QQ Super Compression.vst3')
if($dll.VersionInfo.FileVersion -ne '1.2.5'){throw 'Wrong native version'}
$sourceRecords=@(Get-ChildItem -LiteralPath (Join-Path $source 'Source') -File | ForEach-Object {
    [pscustomobject]@{Path=$_.Name;SHA256=(Get-FileHash -LiteralPath $_.FullName).Hash}
})
$result=[pscustomobject]@{
    Version='1.2.5';Revision='Classic90';Status='Candidate';Source=$source;Output=$candidate;Desktop=$null
    DesktopPolicy='Only create desktop deliverables when the user requests Plan C.'
    Bundle=$records;SourceFiles=$sourceRecords;Tests='DSP including 216 matched A/B cases and gate-free deep MATCH, UI, actual released VST3 reference comparisons passed'
    Installed=$false;Installation='Pending host closure';Multiband='Unchanged; awaiting user confirmation';Publication='Paused';CreatedAt=(Get-Date -Format s)
}
$result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $proof 'CANDIDATE.json') -Encoding utf8
$result | Select-Object Version,Status,Output,Desktop,Installed | ConvertTo-Json
