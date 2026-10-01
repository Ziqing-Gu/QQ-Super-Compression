[CmdletBinding()]
param(
    [string]$JucePath = '',
    [string]$BuildDirectory = '',
    [switch]$SkipTests
)
$ErrorActionPreference = 'Stop'
$source = $PSScriptRoot

function Get-SourceFileSha256 {
    param([string]$Path)
    $algorithm = [System.Security.Cryptography.SHA256]::Create()
    $stream = [System.IO.File]::OpenRead($Path)
    try { return ([System.BitConverter]::ToString($algorithm.ComputeHash($stream))).Replace('-', '') }
    finally { $stream.Dispose(); $algorithm.Dispose() }
}

function Invoke-Checked {
    param([string]$Program, [string[]]$Arguments)
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}

try {
    if ([Environment]::OSVersion.Platform -ne 'Win32NT') { throw 'This script requires Windows.' }
    $projectText = Get-Content -LiteralPath (Join-Path $source 'CMakeLists.txt') -Raw
    if ($projectText -notmatch 'project\(QQSuperCompression VERSION 1\.2\.41\s') {
        throw 'This build helper is for QQ Super Compression 1.2.42 only.'
    }
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio 2022 C++ Build Tools (and Windows SDK) are required.' }
    $vs = (& $vswhere -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1)
    if ([string]::IsNullOrWhiteSpace($vs)) { throw 'Visual Studio 2022 with x64 C++ tools was not found.' }
    $cmake = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    if (-not (Test-Path -LiteralPath $cmake)) {
        $command = Get-Command cmake.exe -ErrorAction SilentlyContinue
        if ($null -eq $command) { throw 'CMake 3.22 or newer is required.' }
        $cmake = $command.Source
    }
    if ([string]::IsNullOrWhiteSpace($JucePath)) {
        $candidates = @(
            $env:JUCE_PATH,
            (Join-Path $source 'Dependencies\JUCE-8.0.15'),
            'D:\Codex\Temp\JUCE-8.0.15-src\JUCE-8.0.15'
        )
        foreach ($candidate in $candidates) {
            if (-not [string]::IsNullOrWhiteSpace($candidate) -and (Test-Path -LiteralPath (Join-Path $candidate 'modules\juce_core\juce_core.h'))) {
                $JucePath = $candidate; break
            }
        }
    }
    if ([string]::IsNullOrWhiteSpace($JucePath) -or -not (Test-Path -LiteralPath (Join-Path $JucePath 'CMakeLists.txt'))) {
        throw 'JUCE source is not bundled. Supply the project-pinned JUCE 8.0.15 checkout: BUILD_WINDOWS.ps1 -JucePath "D:\path\to\JUCE-8.0.15"'
    }
    $JucePath = (Resolve-Path -LiteralPath $JucePath).Path
    if ([string]::IsNullOrWhiteSpace($BuildDirectory)) { $BuildDirectory = 'D:\Codex\Temp\QQSC1242-Build' }
    $BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)

    # Build-fix archives naturally live in different source folders. CMake pins
    # CMAKE_HOME_DIRECTORY in its cache, so silently reusing that cache would
    # reject the new source tree before compilation starts. Refuse a mismatched
    # cache and preserve all files; use a new build directory for restored source.
    $cacheFile = Join-Path $BuildDirectory 'CMakeCache.txt'
    if (Test-Path -LiteralPath $cacheFile) {
        $cacheText = Get-Content -LiteralPath $cacheFile -Raw
        $homeMatch = [regex]::Match($cacheText, '(?m)^CMAKE_HOME_DIRECTORY:INTERNAL=(.+)$')
        if ($homeMatch.Success) {
            $cachedSource = $homeMatch.Groups[1].Value.Trim()
            try { $cachedSource = [IO.Path]::GetFullPath($cachedSource) } catch { }
            $currentSource = [IO.Path]::GetFullPath($source)
            if (-not [string]::Equals($cachedSource.TrimEnd([char[]]'\/'), $currentSource.TrimEnd([char[]]'\/'), [StringComparison]::OrdinalIgnoreCase)) {
                throw "CMake cache belongs to another source folder: $cachedSource. Choose a new empty -BuildDirectory under D:\Codex\Temp; this helper preserves existing files."
            }
        }
    }

    Write-Host "Source: $source"
    Write-Host "JUCE:   $JucePath"
    Write-Host "Build:  $BuildDirectory"
    Write-Host 'No plugin will be installed or published by this script.'
    Invoke-Checked -Program $cmake -Arguments @('-S', $source, '-B', $BuildDirectory, '-G', 'Visual Studio 17 2022', '-A', 'x64', "-DJUCE_PATH=$JucePath", '-DQQSC_FETCH_JUCE=OFF', '-DQQSC_BUILD_DYNAMICS_CHECK=ON')
    $targets = @('QQSuperCompression_VST3')
    if (-not $SkipTests) { $targets += @('QQSCLimiterCheck','QQSCCeilingCheck','QQSCReviewCheck','QQSCUnityCheck') }
    $buildArgs = @('--build', $BuildDirectory, '--config', 'Release', '--target') + $targets + @('--parallel', '4')
    Invoke-Checked -Program $cmake -Arguments $buildArgs
    $testStatus = 'not run (-SkipTests)'
    if (-not $SkipTests) {
        $testExe = Join-Path $BuildDirectory 'QQSCLimiterCheck_artefacts\Release\QQSCLimiterCheck.exe'
        foreach ($profile in @('revision1211', 'revision1218', 'revision1221', 'revision1225', 'revision1226', 'revision1227', 'revision1228', 'revision1234', 'continuity', 'dual')) {
            $testOutput = Join-Path $BuildDirectory ("verification\" + $profile)
            Invoke-Checked -Program $testExe -Arguments @($testOutput, $profile)
        }
        $ceilingExe = Join-Path $BuildDirectory 'QQSCCeilingCheck_artefacts\Release\QQSCCeilingCheck.exe'
        Invoke-Checked -Program $ceilingExe -Arguments @()
        Invoke-Checked -Program (Join-Path $BuildDirectory 'QQSCReviewCheck_artefacts\Release\QQSCReviewCheck.exe') -Arguments @((Join-Path $BuildDirectory 'verification\review-1235'))
        Invoke-Checked -Program (Join-Path $BuildDirectory 'QQSCUnityCheck_artefacts\Release\QQSCUnityCheck.exe') -Arguments @((Join-Path $BuildDirectory 'verification\unity-1235'))
        $testStatus = 'QQSCLimiterCheck revision1211, revision1218, revision1221, revision1225, revision1226, revision1227, revision1228, revision1234, continuity and dual plus QQSCCeilingCheck, QQSCReviewCheck and QQSCUnityCheck passed'
    }
    $bundle = Join-Path $BuildDirectory 'QQSuperCompression_artefacts\Release\VST3\QQ Super Compression.vst3'
    $dll = Join-Path $bundle 'Contents\x86_64-win\QQ Super Compression.vst3'
    if (-not (Test-Path -LiteralPath $dll)) { throw 'Build returned without the expected Windows VST3 binary.' }
    $record = [ordered]@{
        version = '1.2.42'; status = 'built locally; not installed by this script'
        source = $source; juce = $JucePath; binary = $dll; tests = $testStatus
        timestamp = (Get-Date).ToString('o'); sha256 = (Get-SourceFileSha256 -Path $dll)
    }
    $record | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $BuildDirectory 'WINDOWS_BUILD_RESULT_1.2.42.json') -Encoding UTF8
    Write-Host "Build ready: $bundle"
    Write-Host "Tests: $testStatus"
    Write-Host 'Close Cubase and other plugin hosts before manually replacing an installed VST3. Keep the previously verified source and binary as rollback.'
    exit 0
}
catch {
    Write-Error $_ -ErrorAction Continue
    exit 1
}
