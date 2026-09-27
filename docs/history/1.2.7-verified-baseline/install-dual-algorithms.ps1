$ErrorActionPreference = 'Stop'
$taskSource = 'D:\Codex\Outputs\QQ Super Compression\1.2.7 Dual Algorithm Output Fix Candidate\Win\QQ Super Compression.vst3'
$taskTarget = 'C:\Program Files\Common Files\VST3\QQ Super Compression.vst3'
$taskManifest = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'CANDIDATE_ARTIFACTS.json') -Raw | ConvertFrom-Json
$taskHosts = @(Get-CimInstance Win32_Process | Where-Object { $_.Name -match '(?i)cubase|nuendo|reaper|studio.?one|ableton|^live[ -]|fl64|fl32|flstudio|bitwig|plugin.?doctor|audition|cakewalk|waveform|mixcraft|samplitude|sequoia|wavelab|protools|qqhost|audio.?plugin.?host|vst.*host|cantabile|gig.?performer|^reason|ardour|maschine' })
if ($taskHosts.Count) { throw ('Host still running: ' + ($taskHosts.Name -join ', ')) }
foreach ($taskFile in $taskManifest.files) {
    $taskActual = Get-FileHash -LiteralPath (Join-Path $taskSource $taskFile.path) -Algorithm SHA256
    if ($taskActual.Hash -ne $taskFile.sha256) { throw ('Candidate hash mismatch: ' + $taskFile.path) }
}
$taskOldDll = Join-Path $taskTarget 'Contents\x86_64-win\QQ Super Compression.vst3'
$taskPreviousHash = (Get-FileHash -LiteralPath $taskOldDll -Algorithm SHA256).Hash
if ($taskPreviousHash -ne $taskManifest.installedBaselineSha256) { throw 'Installed version has changed since verification' }
$taskStream = [IO.File]::Open($taskOldDll,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
$taskStream.Dispose()
$taskBackupRoot = Join-Path 'D:\Codex\Archives' ('QQSuperCompression-1.2.7-before-Dual-Algorithms-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
$taskBackup = Join-Path $taskBackupRoot 'QQ Super Compression.vst3'
New-Item -ItemType Directory -Path $taskBackupRoot | Out-Null
Copy-Item -LiteralPath $taskTarget -Destination $taskBackupRoot -Recurse
$taskOldFiles = @(Get-ChildItem -LiteralPath $taskTarget -File -Recurse)
foreach ($taskFile in $taskOldFiles) {
    $taskRelative = [IO.Path]::GetRelativePath($taskTarget,$taskFile.FullName)
    if ((Get-FileHash -LiteralPath $taskFile.FullName -Algorithm SHA256).Hash -ne (Get-FileHash -LiteralPath (Join-Path $taskBackup $taskRelative) -Algorithm SHA256).Hash) {
        throw ('Backup verification failed: ' + $taskRelative)
    }
}
if (@(Get-ChildItem -LiteralPath $taskBackup -File -Recurse).Count -ne $taskOldFiles.Count) { throw 'Backup file-count mismatch' }
try {
    foreach ($taskFile in $taskManifest.files) {
        Copy-Item -LiteralPath (Join-Path $taskSource $taskFile.path) -Destination (Join-Path $taskTarget $taskFile.path) -Force
    }
    foreach ($taskFile in $taskManifest.files) {
        if ((Get-FileHash -LiteralPath (Join-Path $taskTarget $taskFile.path) -Algorithm SHA256).Hash -ne $taskFile.sha256) {
            throw ('Installation hash mismatch: ' + $taskFile.path)
        }
    }
    if (@(Get-ChildItem -LiteralPath $taskTarget -File -Recurse).Count -ne $taskManifest.files.Count) { throw 'Installation file-count mismatch' }
}
catch {
    foreach ($taskFile in Get-ChildItem -LiteralPath $taskBackup -File -Recurse) {
        $taskRelative = [IO.Path]::GetRelativePath($taskBackup,$taskFile.FullName)
        Copy-Item -LiteralPath $taskFile.FullName -Destination (Join-Path $taskTarget $taskRelative) -Force
    }
    throw
}
$taskReceipt = [ordered]@{
    timestamp = Get-Date -Format o
    status = 'Installed; all bundle hashes verified'
    installed = $taskTarget
    source = $taskSource
    backup = $taskBackup
    previousDllSha256 = $taskPreviousHash
    files = $taskManifest.files
}
$taskReceipt | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'INSTALLATION_DUAL_ALGORITHMS.json') -Encoding utf8
$taskManifest.status = 'Validated and installed; see INSTALLATION_DUAL_ALGORITHMS.json'
$taskManifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'CANDIDATE_ARTIFACTS.json') -Encoding utf8
$taskReceipt | ConvertTo-Json -Depth 6

