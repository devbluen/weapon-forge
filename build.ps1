<#
.SYNOPSIS
    Builds weapon-forge for Windows (weapon-forge.dll) and Linux (weapon-forge.so).

.DESCRIPTION
    Windows: CMake + Visual Studio (32-bit, static runtime).
    Linux:   cross-compiled with Zig (32-bit, glibc 2.17+, static C++ runtime).
             Zig is downloaded automatically into build\tools on first use, so
             no WSL, Docker or Linux machine is needed. Also runs on Linux (pwsh).

    The open.mp SDK is cloned once into build\deps (requires git).
    Output goes to dist\: plugins\ (SA-MP) and components\ (open.mp) hold the
    same binary, plus include\weapon-forge.inc.

.EXAMPLE
    .\build.ps1                          # Windows + Linux, Release
    .\build.ps1 -Platform Linux          # only weapon-forge.so
    .\build.ps1 -Clean -Package          # clean build + zip/tar.gz in dist\
    .\build.ps1 -Version 1.2.0 -Package
#>
[CmdletBinding()]
param(
    [ValidateSet('All', 'Windows', 'Linux')]
    [string]$Platform = 'All',

    [ValidateSet('Release', 'Debug', 'RelWithDebInfo')]
    [string]$Config = 'Release',

    # Defaults to the contents of the VERSION file
    [string]$Version = '',

    # CMake generator for Windows; defaults to the newest Visual Studio found
    [string]$Generator = '',

    [switch]$Clean,
    [switch]$Package
)

$ErrorActionPreference = 'Stop'
$Root = $PSScriptRoot
$BuildRoot = Join-Path $Root 'build'
$DistDir = Join-Path $Root 'dist'
$DepsDir = Join-Path $BuildRoot 'deps'
$ToolsDir = Join-Path $BuildRoot 'tools'
$ZigVersion = '0.13.0'
$OnWindows = [System.Environment]::OSVersion.Platform -eq 'Win32NT'

function Write-Step([string]$Message) {
    Write-Host "==> $Message" -ForegroundColor Cyan
}

function Invoke-Native {
    param([string]$Exe, [string[]]$Arguments, [string]$What)
    # Out-Host: keep the tool output out of the function's return value
    & $Exe @Arguments | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "$What failed (exit code $LASTEXITCODE)" }
}

function Get-VersionNumber([string]$Text) {
    if ($Text -match '^(\d+)\.(\d+)\.(\d+)') {
        return [int]$Matches[1] * 10000 + [int]$Matches[2] * 100 + [int]$Matches[3]
    }
    return 0
}

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

function Get-OmpSdk {
    $commit = (Select-String -Path (Join-Path $Root 'CMakeLists.txt') -Pattern 'WF_OMP_SDK_COMMIT "([0-9a-f]+)"').Matches[0].Groups[1].Value
    $sdk = Join-Path $DepsDir 'omp-sdk'
    $stamp = Join-Path $sdk '.weapon-forge-commit'

    if ((Test-Path $stamp) -and ((Get-Content $stamp -Raw).Trim() -eq $commit)) {
        return $sdk
    }

    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        throw 'git not found: it is needed once to download the open.mp SDK.'
    }

    Write-Step "Downloading open.mp SDK ($($commit.Substring(0, 8)))"
    if (Test-Path $sdk) { Remove-Item -Recurse -Force $sdk }
    New-Item -ItemType Directory -Force $DepsDir | Out-Null

    Invoke-Native git @('clone', '--quiet', 'https://github.com/openmultiplayer/open.mp-sdk.git', $sdk) 'git clone'
    Invoke-Native git @('-C', $sdk, 'checkout', '--quiet', $commit) 'git checkout'
    Invoke-Native git @('-C', $sdk, 'submodule', 'update', '--init', '--recursive', '--quiet') 'git submodule update'

    Set-Content -Path $stamp -Value $commit
    return $sdk
}

function Get-Zig {
    $os = if ($OnWindows) { 'windows' } elseif ($IsMacOS) { 'macos' } else { 'linux' }
    $ext = if ($OnWindows) { 'zip' } else { 'tar.xz' }
    $name = "zig-$os-x86_64-$ZigVersion"
    $dir = Join-Path $ToolsDir $name
    $exe = Join-Path $dir ($(if ($OnWindows) { 'zig.exe' } else { 'zig' }))

    if (Test-Path $exe) {
        return $exe
    }

    Write-Step "Downloading Zig $ZigVersion (Linux cross-compiler)"
    New-Item -ItemType Directory -Force $ToolsDir | Out-Null
    $archive = Join-Path $ToolsDir "$name.$ext"
    Invoke-WebRequest "https://ziglang.org/download/$ZigVersion/$name.$ext" -OutFile $archive

    if ($OnWindows) {
        Expand-Archive $archive -DestinationPath $ToolsDir -Force
    } else {
        Invoke-Native tar @('-xJf', $archive, '-C', $ToolsDir) 'tar'
    }

    Remove-Item $archive
    return $exe
}

# ---------------------------------------------------------------------------
# Windows
# ---------------------------------------------------------------------------

function Find-CMake {
    $cmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    # CMake bundled with Visual Studio
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $vs = & $vswhere -latest -products * -property installationPath
        if ($vs) {
            $bundled = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
            if (Test-Path $bundled) { return $bundled }
        }
    }

    throw 'CMake not found. Install it (winget install Kitware.CMake) or the "C++ CMake tools" Visual Studio component.'
}

function Find-Generator {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path $vswhere)) {
        throw 'Visual Studio not found. Install Visual Studio 2019/2022 (or Build Tools) with the "Desktop development with C++" workload.'
    }

    $major = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property catalog_productLineVersion
    switch ($major) {
        '2022' { return 'Visual Studio 17 2022' }
        '2019' { return 'Visual Studio 16 2019' }
        default {
            # Newer Visual Studio versions: let CMake pick its default generator
            if ($major) { return '' }
            throw 'No Visual Studio installation with the C++ x86/x64 tools was found.'
        }
    }
}

function Build-Windows([string]$Sdk) {
    if (-not $OnWindows) { throw 'The Windows build needs Visual Studio (run it on Windows).' }

    $buildDir = Join-Path $BuildRoot 'win32'
    if ($Clean -and (Test-Path $buildDir)) { Remove-Item -Recurse -Force $buildDir }

    $cmake = Find-CMake
    $generator = if ($Generator) { $Generator } else { Find-Generator }

    Write-Step "Windows: configuring ($Config, Win32)"
    $configureArgs = @('-S', $Root, '-B', $buildDir, '-A', 'Win32', "-DWF_VERSION=$Version", "-DFETCHCONTENT_SOURCE_DIR_OMP_SDK=$Sdk")
    if ($generator) { $configureArgs = @('-G', $generator) + $configureArgs }
    Invoke-Native $cmake $configureArgs 'CMake configure'

    Write-Step 'Windows: building'
    Invoke-Native $cmake @('--build', $buildDir, '--config', $Config, '--parallel') 'Windows build'

    $dll = Join-Path $buildDir "$Config\weapon-forge.dll"
    if (-not (Test-Path $dll)) { throw "Build output not found: $dll" }
    return $dll
}

# ---------------------------------------------------------------------------
# Linux (Zig cross-compiler)
# ---------------------------------------------------------------------------

function Build-Linux([string]$Sdk) {
    $buildDir = Join-Path $BuildRoot 'linux'
    if ($Clean -and (Test-Path $buildDir)) { Remove-Item -Recurse -Force $buildDir }
    New-Item -ItemType Directory -Force $buildDir | Out-Null

    $zig = Get-Zig

    # The version goes in a forced include to avoid quoting a string macro on the command line
    $versionHeader = Join-Path $buildDir 'wf_build_version.h'
    Set-Content -Path $versionHeader -Value @(
        '#pragma once'
        "#define WF_VERSION `"$Version`""
        "#define WF_VERSION_NUMBER $(Get-VersionNumber $Version)"
    )

    $optimize = switch ($Config) {
        'Debug' { @('-O0', '-g') }
        'RelWithDebInfo' { @('-O2', '-g') }
        default { @('-O2', '-s') }
    }

    $output = Join-Path $buildDir 'weapon-forge.so'
    $sources = Get-ChildItem (Join-Path $Root 'src') -Filter '*.cpp' | ForEach-Object { $_.FullName }

    $zigArgs = @(
        'c++', '-target', 'x86-linux-gnu.2.17', '-std=c++17', '-shared', '-fPIC', '-fvisibility=hidden',
        '-msse2', '-mfpmath=sse', '-Wall', '-Wextra', '-Wl,--no-undefined'
    ) + $optimize + @(
        '-include', $versionHeader,
        '-DGLM_FORCE_QUAT_DATA_WXYZ', '-DGLM_FORCE_SSE2', '-DNOMINMAX',
        '-Dnssv_CONFIG_SELECT_STRING_VIEW=nssv_STRING_VIEW_NONSTD',
        '-Dspan_CONFIG_SELECT_SPAN=span_SPAN_NONSTD',
        '-isystem', (Join-Path $Root 'src/sdk/omp'),
        '-isystem', (Join-Path $Sdk 'include'),
        '-isystem', (Join-Path $Sdk 'lib/glm'),
        '-isystem', (Join-Path $Sdk 'lib/robin-hood-hashing/src/include'),
        '-isystem', (Join-Path $Sdk 'lib/span-lite/include'),
        '-isystem', (Join-Path $Sdk 'lib/string-view-lite/include'),
        '-o', $output
    ) + $sources

    Write-Step "Linux: building with Zig $ZigVersion (x86-linux-gnu, glibc 2.17)"
    Invoke-Native $zig $zigArgs 'Linux build'

    $candidates = if ($OnWindows) { @('python', 'py') } else { @('python3', 'python') }
    $python = Get-Command $candidates -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($python) {
        Invoke-Native $python.Source @((Join-Path $Root 'tools/elfcheck.py'), $output) 'ELF check'
    }

    return $output
}

# ---------------------------------------------------------------------------

if (-not $Version) {
    $Version = (Get-Content (Join-Path $Root 'VERSION') -TotalCount 1).Trim()
}

$platforms = if ($Platform -eq 'All') { if ($OnWindows) { @('Windows', 'Linux') } else { @('Linux') } } else { @($Platform) }

Write-Step "weapon-forge $Version ($($platforms -join ', '), $Config)"
$sdk = Get-OmpSdk

foreach ($dir in 'plugins', 'components', 'include') {
    New-Item -ItemType Directory -Force (Join-Path $DistDir $dir) | Out-Null
}
Copy-Item (Join-Path $Root 'include/weapon-forge.inc') (Join-Path $DistDir 'include/weapon-forge.inc') -Force

$outputs = @{}

foreach ($target in $platforms) {
    $binary = if ($target -eq 'Windows') { Build-Windows $sdk } else { Build-Linux $sdk }
    $name = Split-Path $binary -Leaf

    # Same binary: plugins\ for SA-MP, components\ for open.mp
    Copy-Item $binary (Join-Path $DistDir "plugins/$name") -Force
    Copy-Item $binary (Join-Path $DistDir "components/$name") -Force
    $outputs[$target] = $name
}

if ($Package) {
    foreach ($target in $outputs.Keys) {
        $name = $outputs[$target]
        $stage = Join-Path $BuildRoot "package-$($target.ToLower())"
        if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }

        foreach ($dir in 'plugins', 'components', 'include') {
            New-Item -ItemType Directory -Force (Join-Path $stage $dir) | Out-Null
        }
        Copy-Item (Join-Path $DistDir "plugins/$name") (Join-Path $stage "plugins/$name")
        Copy-Item (Join-Path $DistDir "components/$name") (Join-Path $stage "components/$name")
        Copy-Item (Join-Path $DistDir 'include/weapon-forge.inc') (Join-Path $stage 'include/weapon-forge.inc')

        if ($target -eq 'Windows') {
            $archive = Join-Path $DistDir "weapon-forge-$Version-windows.zip"
            if (Test-Path $archive) { Remove-Item -Force $archive }
            Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $archive
        } else {
            $archive = Join-Path $DistDir "weapon-forge-$Version-linux.tar.gz"
            # Relative paths: GNU tar reads "D:\..." as a remote host
            Push-Location $stage
            try {
                Invoke-Native tar @('-czf', 'package.tar.gz', 'plugins', 'components', 'include') 'tar'
            } finally {
                Pop-Location
            }
            Move-Item (Join-Path $stage 'package.tar.gz') $archive -Force
        }

        Write-Step "Packaged $archive"
    }
}

Write-Host ''
foreach ($target in $outputs.Keys) {
    $name = $outputs[$target]
    Write-Host "$target`: dist/plugins/$name (SA-MP) | dist/components/$name (open.mp)" -ForegroundColor Green
}
