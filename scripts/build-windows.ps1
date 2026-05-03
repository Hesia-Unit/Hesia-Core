param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [switch]$SkipDeps,
    [switch]$NoProviders
)

$ErrorActionPreference = "Stop"

function Invoke-Checked {
    param([string]$FilePath, [string[]]$Arguments, [string]$WorkingDirectory)
    Write-Host ">> $FilePath $($Arguments -join ' ')"
    Push-Location $WorkingDirectory
    try {
        & $FilePath @Arguments
        if ($LASTEXITCODE -ne 0) {
            throw "Command failed with exit code ${LASTEXITCODE}: $FilePath"
        }
    } finally {
        Pop-Location
    }
}

if (-not $SkipDeps -and -not $NoProviders) {
    & (Join-Path $PSScriptRoot "build-deps-windows.ps1") -Root $Root
}

$build = Join-Path $Root "build/windows-x86_64"
$obj = Join-Path $build "hesia_core.o"
$bin = Join-Path $build "bin"
$lib = Join-Path $build "lib"
New-Item -ItemType Directory -Force $build,$bin,$lib | Out-Null

$defines = @()
$includes = @("-Iinclude")
$libs = @("-lbcrypt")

if (-not $NoProviders) {
    $deps = Join-Path $Root "third_party/install/windows-x86_64"
    $openssl = Join-Path $deps "openssl"
    $liboqs = Join-Path $deps "liboqs"
    $defines += "-DHESIA_CORE_WITH_OPENSSL", "-DHESIA_CORE_WITH_LIBOQS"
    $includes += "-I$openssl/include", "-I$liboqs/include"
    $opensslCrypto = Join-Path $openssl "lib/libcrypto.a"
    if (-not (Test-Path $opensslCrypto)) {
        $opensslCrypto = Join-Path $openssl "lib64/libcrypto.a"
    }
    $libs = @(
        (Join-Path $liboqs "lib/liboqs.a"),
        $opensslCrypto,
        "-lws2_32",
        "-lgdi32",
        "-lcrypt32",
        "-lbcrypt"
    )
}

$compileArgs = @(
    "-std=c11",
    "-O2",
    "-Wall",
    "-Wextra",
    "-Wpedantic"
) + $defines + $includes + @(
    "-c", "src/hesia_core.c",
    "-o", $obj
)
Invoke-Checked "gcc" $compileArgs $Root

Invoke-Checked "ar" @("rcs", (Join-Path $lib "libhesia_core.a"), $obj) $Root

$sharedArgs = @(
    "-std=c11",
    "-O2",
    "-Wall",
    "-Wextra",
    "-Wpedantic",
    "-shared"
) + $defines + $includes + @(
    "src/hesia_core.c",
    "-Wl,--export-all-symbols",
    "-o", (Join-Path $bin "hesia_core.dll")
 ) + $libs
Invoke-Checked "gcc" $sharedArgs $Root

$testArgs = @(
    "-std=c11",
    "-O2",
    "-Wall",
    "-Wextra",
    "-Wpedantic"
) + $defines + $includes + @(
    "tests/test_core.c",
    "src/hesia_core.c",
    "-o", (Join-Path $bin "test_core.exe")
) + $libs
Invoke-Checked "gcc" $testArgs $Root

& (Join-Path $bin "test_core.exe")
if (-not $NoProviders) {
    $providerTestArgs = @(
        "-std=c11",
        "-O2",
        "-Wall",
        "-Wextra",
        "-Wpedantic"
    ) + $defines + $includes + @(
        "tests/test_providers.c",
        "src/hesia_core.c",
        "-o", (Join-Path $bin "test_providers.exe")
    ) + $libs
    Invoke-Checked "gcc" $providerTestArgs $Root
    & (Join-Path $bin "test_providers.exe")
}

Write-Host "Windows build output: $build"
