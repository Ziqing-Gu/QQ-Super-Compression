$ErrorActionPreference='Stop'
$candidate='D:\Codex\Outputs\QQ Super Compression\1.2.5 Candidate-Classic90'
$record=Get-Content -LiteralPath (Join-Path $candidate 'Verification\CANDIDATE.json') -Raw -Encoding utf8 | ConvertFrom-Json
$bundle=Join-Path $candidate 'Win\QQ Super Compression.vst3'
$installed='C:\Program Files\Common Files\VST3\QQ Super Compression.vst3'
function HostsClosed {
    $hosts=@(Get-Process | Where-Object {$_.ProcessName -match 'reaper|Cubase|Nuendo|Ableton|Studio One|FL64|PluginDoctor|Bitwig|QQHost|ProTools|WaveLab|Audition|Cakewalk|^Live'})
    if($hosts.Count){throw ('Audio hosts still open: '+(($hosts.ProcessName)-join ', '))}
}
HostsClosed
if((Resolve-Path -LiteralPath $installed).Path -cne $installed){throw 'Unexpected resolved installation path'}
$items=@(Get-ChildItem -LiteralPath $installed -Recurse -Force)
if((Get-Item -LiteralPath $installed).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Linked install root'}
if($items | Where-Object {$_.Attributes -band [IO.FileAttributes]::ReparsePoint}){throw 'Linked installed contents'}
$oldFiles=@($items | Where-Object {-not $_.PSIsContainer})
if($oldFiles.Count -ne $record.Bundle.Count){throw 'Unexpected installed bundle layout'}
$oldDll=Join-Path $installed 'Contents\x86_64-win\QQ Super Compression.vst3'
if((Get-FileHash -LiteralPath $oldDll).Hash -ne '6F4448378EB5FC1C5AB9F440203D6C06EA8B02BEE9A6E6575657D19593EEA280'){throw 'Installed plugin changed since candidate preparation'}
foreach($r in $record.Bundle){
    if((Get-FileHash -LiteralPath (Join-Path $bundle $r.Path)).Hash -ne $r.SHA256){throw 'Candidate hash mismatch'}
    if(-not(Test-Path -LiteralPath (Join-Path $installed $r.Path))){throw 'Installed layout mismatch'}
}
$rollback=Join-Path 'D:\Codex\Archives' ('QQSC-Before-1.2.5-Classic90-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $rollback | Out-Null
Copy-Item -LiteralPath $installed -Destination $rollback -Recurse
$rollbackBundle=Join-Path $rollback 'QQ Super Compression.vst3'
foreach($file in $oldFiles){
    $relative=[IO.Path]::GetRelativePath($installed,$file.FullName)
    if((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath (Join-Path $rollbackBundle $relative)).Hash){throw 'Rollback copy mismatch'}
}
HostsClosed
$lock=[IO.File]::Open($oldDll,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
$lock.Dispose()
try{
    foreach($r in $record.Bundle){Copy-Item -LiteralPath (Join-Path $bundle $r.Path) -Destination (Join-Path $installed $r.Path) -Force}
    foreach($r in $record.Bundle){if((Get-FileHash -LiteralPath (Join-Path $installed $r.Path)).Hash -ne $r.SHA256){throw 'Installed hash mismatch'}}
}catch{
    foreach($r in $record.Bundle){Copy-Item -LiteralPath (Join-Path $rollbackBundle $r.Path) -Destination (Join-Path $installed $r.Path) -Force}
    throw
}
$record.Installed=$true;$record.Installation=$installed
$record | Add-Member NoteProperty Rollback $rollback
$record | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $candidate 'Verification\CANDIDATE.json') -Encoding utf8
$record | Select-Object Version,Status,Installed,Installation,Rollback | ConvertTo-Json
