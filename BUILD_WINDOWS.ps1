[CmdletBinding()]
param(
    [string]$JucePath = '',
    [string]$BuildDirectory = '',
    [switch]$SkipTests
)
$ErrorActionPreference = 'Stop'
$source = $PSScriptRoot

function Invoke-Checked {
    param([string]$Program, [string[]]$Arguments)
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}

try {
    if ([Environment]::OSVersion.Platform -ne 'Win32NT') { throw 'This script requires Windows.' }
    $projectText = Get-Content -LiteralPath (Join-Path $source 'CMakeLists.txt') -Raw
    if ($projectText -notmatch 'project\(QQSuperCompression VERSION 1\.2\.8\s') {
        throw 'This build helper is for the 1.2.8 source candidate only.'
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
        throw 'JUCE source is not bundled. Supply the JUCE 8.0.15 checkout used for 1.2.7: BUILD_WINDOWS.ps1 -JucePath "D:\path\to\JUCE-8.0.15"'
    }
    $JucePath = (Resolve-Path -LiteralPath $JucePath).Path
    if ([string]::IsNullOrWhiteSpace($BuildDirectory)) { $BuildDirectory = Join-Path $source 'build-1.2.8-windows' }
    $BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
    Write-Host "Source: $source"
    Write-Host "JUCE:   $JucePath"
    Write-Host "Build:  $BuildDirectory"
    Write-Host 'No plugin will be installed or published by this script.'
    Invoke-Checked -Program $cmake -Arguments @('-S', $source, '-B', $BuildDirectory, '-G', 'Visual Studio 17 2022', '-A', 'x64', "-DJUCE_PATH=$JucePath", '-DQQSC_FETCH_JUCE=OFF', '-DQQSC_BUILD_DYNAMICS_CHECK=ON')
    $targets = @('QQSuperCompression_VST3')
    if (-not $SkipTests) { $targets += 'QQSCLimiterCheck' }
    $buildArgs = @('--build', $BuildDirectory, '--config', 'Release', '--target') + $targets + @('--parallel', '4')
    Invoke-Checked -Program $cmake -Arguments $buildArgs
    $testStatus = 'not run (-SkipTests)'
    if (-not $SkipTests) {
        $testExe = Join-Path $BuildDirectory 'QQSCLimiterCheck_artefacts\Release\QQSCLimiterCheck.exe'
        $testOutput = Join-Path $BuildDirectory 'verification\continuity'
        Invoke-Checked -Program $testExe -Arguments @($testOutput, 'continuity')
        $testStatus = 'QQSCLimiterCheck continuity passed'
    }
    $bundle = Join-Path $BuildDirectory 'QQSuperCompression_artefacts\Release\VST3\QQ Super Compression.vst3'
    $dll = Join-Path $bundle 'Contents\x86_64-win\QQ Super Compression.vst3'
    if (-not (Test-Path -LiteralPath $dll)) { throw 'Build returned without the expected Windows VST3 binary.' }
    $record = [ordered]@{
        version = '1.2.8'; status = 'built locally; not installed by this script'
        source = $source; juce = $JucePath; binary = $dll; tests = $testStatus
        timestamp = (Get-Date).ToString('o'); sha256 = (Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash
    }
    $record | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $BuildDirectory 'WINDOWS_BUILD_RESULT_1.2.8.json') -Encoding UTF8
    Write-Host "Build ready: $bundle"
    Write-Host "Tests: $testStatus"
    Write-Host 'Close Cubase and other plugin hosts before manually replacing an installed VST3. Keep the verified 1.2.7 binary for rollback.'
    exit 0
}
catch {
    Write-Error $_ -ErrorAction Continue
    exit 1
}
