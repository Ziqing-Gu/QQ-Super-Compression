$ErrorActionPreference='Stop'
$desktop='C:\Users\86189\Desktop\QQ Super Compression 1.2.4 Candidate'
$candidate='D:\Codex\Outputs\QQ Super Compression\1.2.4 Candidate'
$proof=Join-Path $candidate 'Verification\CANDIDATE.json'
$record=Get-Content -LiteralPath $proof -Raw -Encoding utf8 | ConvertFrom-Json
if((Resolve-Path -LiteralPath $desktop).Path -cne $desktop){throw 'Unexpected resolved desktop target'}
if([IO.Path]::GetDirectoryName($desktop) -cne 'C:\Users\86189\Desktop'){throw 'Unexpected desktop parent'}
if((Get-Item -LiteralPath $desktop).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Linked desktop target'}
$items=@(Get-ChildItem -LiteralPath $desktop -Recurse -Force)
if($items | Where-Object {$_.Attributes -band [IO.FileAttributes]::ReparsePoint}){throw 'Linked desktop contents'}
$expected=@('试用说明.md')+@($record.Bundle | ForEach-Object {'Win\QQ Super Compression.vst3\'+$_.Path})
$files=@($items | Where-Object {-not $_.PSIsContainer})
if($files.Count -ne $expected.Count){throw 'Desktop contains additional files; preserve them'}
foreach($rel in $expected){
    $a=Join-Path $desktop $rel;$b=Join-Path $candidate $rel
    if((Get-FileHash -LiteralPath $a).Hash -ne (Get-FileHash -LiteralPath $b).Hash){throw 'Desktop file changed; preserve it'}
}
Remove-Item -LiteralPath $desktop -Recurse -Force
if(Test-Path -LiteralPath $desktop){throw 'Desktop cleanup failed'}
$record.Desktop=$null
$record | Add-Member NoteProperty DesktopPolicy 'Only create desktop deliverables when the user requests Plan C.' -Force
$record | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $proof -Encoding utf8
[pscustomobject]@{Removed=$desktop;OutputRetained=$candidate;Installed=$record.Installed} | ConvertTo-Json
