# Job runner for building, testing and packaging NeuroScope on Windows, the counterpart of the Makefile used on
# Linux and macOS, locally and in CI. CMake does the actual work; the GitHub workflows only set up the
# runners and call these targets. Run it from a Developer PowerShell for Visual Studio, with Qt on the
# PATH or in CMAKE_PREFIX_PATH.
#
# Development
#   ./make.ps1                 build (configures on first use)
#   ./make.ps1 test            build and run the unit tests
#   ./make.ps1 install         install to PREFIX
#   ./make.ps1 smoke           install and start neuroscope.exe --version on the offscreen platform
#   ./make.ps1 check           test, install and smoke
#   ./make.ps1 package         NSIS installer and zip in PACKAGE_DIR
#   ./make.ps1 reconfigure     rerun CMake, e.g. after changing the variables below
#   ./make.ps1 clean           empty the build directory and remove the release build directory
#
# Release packages, in PACKAGE_DIR, versioned after the checked-out v* tag
#   ./make.ps1 windows-packages        NSIS installer and zip with a bundled libneurosuite, without Qt WebEngine
#   ./make.ps1 windows-packages-check  start the application from the zip without Qt on the PATH
#
# Variables, as NAME=value arguments or environment variables, e.g. ./make.ps1 check BUILD_TYPE=Debug:
#   BUILD_DIR, BUILD_TYPE, PREFIX, PACKAGE_DIR, GENERATOR, CMAKE_ARGS, PKG_VERSION
#   BUNDLE_NEUROSUITE=ON   build libneurosuite at LIBNEUROSUITE_REF as part of NeuroScope instead of
#                          using an installed copy
#   WITH_WEBENGINE=OFF     build the bundled libneurosuite without Qt WebEngine
#   LIBNEUROSUITE_DIR      libneurosuite source tree for BUNDLE_NEUROSUITE; fetched by CMake if missing

$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot

$Vars = [ordered]@{
    BUILD_DIR         = 'build'
    BUILD_TYPE        = 'Release'
    PREFIX            = "$PSScriptRoot/install"
    PACKAGE_DIR       = "$PSScriptRoot/packages"
    GENERATOR         = 'Ninja'
    BUNDLE_NEUROSUITE = 'OFF'
    LIBNEUROSUITE_REF = 'main'
    LIBNEUROSUITE_DIR = "$PSScriptRoot/libneurosuite"
    WITH_WEBENGINE    = 'ON'
    CMAKE_ARGS        = ''
    PKG_VERSION       = ''
}
foreach ($name in @($Vars.Keys)) {
    $value = [Environment]::GetEnvironmentVariable($name)
    if ($value) { $Vars[$name] = $value }
}
$Targets = @()
foreach ($argument in $args) {
    if ($argument -match '^(\w+)=(.*)$') { $Vars[$Matches[1]] = $Matches[2] } else { $Targets += $argument }
}
if (-not $Targets) { $Targets = @('build') }

function ConvertTo-CMakePath([string] $Path) {
    return $Path -replace '\\', '/'
}

function Invoke-Native([string] $Command, [string[]] $Arguments) {
    Write-Host "$Command $($Arguments -join ' ')"
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Command exited with $LASTEXITCODE" }
}

# Output of a command, or '' if it fails.
function Get-Output([string] $Command, [string[]] $Arguments) {
    $ErrorActionPreference = 'Continue'
    $output = & $Command @Arguments 2>$null
    if ($LASTEXITCODE -eq 0) { return "$output".Trim() }
    return ''
}

# neuroscope.exe is a GUI application, which PowerShell does not wait for when it is called directly.
function Start-NeuroScope([string] $Exe) {
    Write-Host "$Exe --version"
    $process = Start-Process -FilePath $Exe -ArgumentList '--version' -Wait -PassThru -NoNewWindow
    if ($process.ExitCode -ne 0) { throw "$Exe exited with $($process.ExitCode)" }
}

# Release versions: v3.0.0-rc1 gives 3.0.0-rc1; untagged builds are dev-<commit>.
function Get-PackageVersion {
    if ($Vars.PKG_VERSION) { return $Vars.PKG_VERSION }
    if ($env:GITHUB_REF -like 'refs/tags/v*') { $tag = $env:GITHUB_REF_NAME }
    else { $tag = Get-Output git @('describe', '--tags', '--exact-match', '--match', 'v*') }
    if ($tag) { return $tag.Substring(1) }
    return 'dev-' + (Get-Output git @('rev-parse', '--short=7', 'HEAD'))
}

# Bundled libneurosuite: from LIBNEUROSUITE_DIR if it exists, otherwise fetched by CMake.
function Get-BundleArguments {
    $result = @('-DNEUROSCOPE_BUNDLE_NEUROSUITE=ON', "-DNEUROSUITE_GIT_TAG=$($Vars.LIBNEUROSUITE_REF)")
    if (Test-Path "$($Vars.LIBNEUROSUITE_DIR)/CMakeLists.txt") {
        $result += "-DFETCHCONTENT_SOURCE_DIR_NEUROSUITE=$(ConvertTo-CMakePath $Vars.LIBNEUROSUITE_DIR)"
    }
    return $result
}

function Invoke-Configure([string] $Dir, [string[]] $Arguments) {
    $cmakeArguments = @('-S', '.', '-B', $Dir, '-G', $Vars.GENERATOR, "-DCMAKE_BUILD_TYPE=$($Vars.BUILD_TYPE)") +
        $Arguments + @($Vars.CMAKE_ARGS -split '\s+' | Where-Object { $_ })
    Invoke-Native cmake $cmakeArguments
}

$Tasks = [ordered]@{
    configure = {
        if (-not (Test-Path "$($Vars.BUILD_DIR)/CMakeCache.txt")) {
            $arguments = @("-DCMAKE_INSTALL_PREFIX=$(ConvertTo-CMakePath $Vars.PREFIX)", "-DWITH_WEBENGINE=$($Vars.WITH_WEBENGINE)")
            if ($Vars.BUNDLE_NEUROSUITE -eq 'ON') { $arguments += Get-BundleArguments }
            Invoke-Configure $Vars.BUILD_DIR $arguments
        }
    }
    reconfigure = {
        Remove-Item -Force -ErrorAction SilentlyContinue "$($Vars.BUILD_DIR)/CMakeCache.txt"
        Invoke-Target configure
    }
    build = {
        Invoke-Target configure
        Invoke-Native cmake @('--build', $Vars.BUILD_DIR)
    }
    test = {
        Invoke-Target build
        Invoke-Native ctest @('--test-dir', $Vars.BUILD_DIR, '--output-on-failure')
    }
    install = {
        Invoke-Target build
        Invoke-Native cmake @('--install', $Vars.BUILD_DIR)
    }
    smoke = {
        Invoke-Target install
        $env:QT_QPA_PLATFORM = 'offscreen'
        Start-NeuroScope "$($Vars.PREFIX)/bin/neuroscope.exe"
    }
    check = {
        Invoke-Target test
        Invoke-Target smoke
    }
    package = {
        Invoke-Target build
        Invoke-Native cpack @('--config', "$($Vars.BUILD_DIR)/CPackConfig.cmake", '-B', (ConvertTo-CMakePath $Vars.PACKAGE_DIR))
    }
    # The build directory itself is kept, with its .gitkeep.
    clean = {
        if (Test-Path $Vars.BUILD_DIR) {
            Get-ChildItem -Force $Vars.BUILD_DIR | Where-Object Name -ne '.gitkeep' | Remove-Item -Recurse -Force
        }
        if (Test-Path "$($Vars.BUILD_DIR)-windows") { Remove-Item -Recurse -Force "$($Vars.BUILD_DIR)-windows" }
    }
    'windows-packages' = {
        $dir = "$($Vars.BUILD_DIR)-windows"
        Invoke-Configure $dir (@("-DCPACK_PACKAGE_VERSION=$(Get-PackageVersion)", '-DWITH_WEBENGINE=OFF') + (Get-BundleArguments))
        Invoke-Native cmake @('--build', $dir)
        Invoke-Native cpack @('--config', "$dir/CPackConfig.cmake", '-G', 'NSIS;ZIP', '-B', (ConvertTo-CMakePath $Vars.PACKAGE_DIR))
    }
    'windows-packages-check' = {
        $unzipped = "$($Vars.BUILD_DIR)-windows/unzipped"
        if (Test-Path $unzipped) { Remove-Item -Recurse -Force $unzipped }
        foreach ($zip in Get-ChildItem "$($Vars.PACKAGE_DIR)/*.zip") {
            Expand-Archive $zip.FullName -DestinationPath $unzipped
        }
        $exe = Get-ChildItem $unzipped -Recurse -Filter neuroscope.exe | Select-Object -First 1
        if (-not $exe) { throw 'neuroscope.exe not found in the zip' }
        if (-not (Test-Path (Join-Path $exe.DirectoryName 'vcruntime140.dll'))) { throw 'MSVC runtime missing' }
        # Hide the Qt installation of the build from the PATH, so that missing DLLs show up.
        $path = $env:PATH
        $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
        try { Start-NeuroScope $exe.FullName } finally { $env:PATH = $path }
    }
}

$Done = @{}
function Invoke-Target([string] $Name) {
    if (-not $Tasks.Contains($Name)) { throw "Unknown target '$Name'; the targets are: $($Tasks.Keys -join ', ')" }
    if ($Done[$Name]) { return }
    $Done[$Name] = $true
    & $Tasks[$Name]
}

foreach ($target in $Targets) { Invoke-Target $target }
