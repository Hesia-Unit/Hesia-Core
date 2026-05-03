param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [switch]$Force,
    [switch]$SkipOpenSSL,
    [switch]$SkipLiboqs
)

$ErrorActionPreference = "Stop"

function Require-Command {
    param([string]$Name)
    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if (-not $cmd) {
        throw "Missing required command: $Name"
    }
    return $cmd.Source
}

function Invoke-Checked {
    param(
        [string]$FilePath,
        [string[]]$Arguments,
        [string]$WorkingDirectory
    )
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

function Download-File {
    param([string]$Url, [string]$OutFile)
    if ((Test-Path $OutFile) -and -not $Force) {
        return
    }
    New-Item -ItemType Directory -Force (Split-Path $OutFile) | Out-Null
    Write-Host "Downloading $Url"
    Invoke-WebRequest -Uri $Url -OutFile $OutFile -UseBasicParsing
}

function Expand-TarGz {
    param([string]$Archive, [string]$Destination, [string]$Marker)
    if ((Test-Path $Marker) -and -not $Force) {
        return
    }
    if (Test-Path $Destination) {
        Remove-Item -LiteralPath $Destination -Recurse -Force
    }
    New-Item -ItemType Directory -Force $Destination | Out-Null
    tar -xzf $Archive -C $Destination --strip-components 1
    if ($LASTEXITCODE -ne 0) {
        throw "tar failed for $Archive"
    }
    New-Item -ItemType File -Force $Marker | Out-Null
}

function Ensure-GitPerlCompatibilityShim {
    param([string]$ShimRoot)
    $shimDir = Join-Path $ShimRoot "Locale/Maketext"
    $shimFile = Join-Path $shimDir "Simple.pm"
    $makeMakerDir = Join-Path $ShimRoot "ExtUtils"
    $makeMakerFile = Join-Path $makeMakerDir "MakeMaker.pm"
    $podDir = Join-Path $ShimRoot "Pod"
    $podUsageFile = Join-Path $podDir "Usage.pm"
    New-Item -ItemType Directory -Force $shimDir | Out-Null
    New-Item -ItemType Directory -Force $makeMakerDir | Out-Null
    New-Item -ItemType Directory -Force $podDir | Out-Null
    @'
package Locale::Maketext::Simple;
use strict;
use warnings;

sub import {
    my $class = shift;
    my $caller = caller;
    no strict 'refs';
    *{"${caller}::loc"} = \&loc;
}

sub loc {
    my ($message, @args) = @_;
    return '' unless defined $message;
    for (my $i = 0; $i < @args; $i++) {
        my $n = $i + 1;
        my $value = defined $args[$i] ? $args[$i] : '';
        $message =~ s/%$n/$value/g;
    }
    return $message;
}

1;
'@ | Set-Content -Encoding ASCII $shimFile
    @'
package ExtUtils::MakeMaker;
use strict;
use warnings;

package MM;
use strict;
use warnings;

sub maybe_command {
    my ($path) = @_;
    return undef unless defined $path && -f $path;
    return $path;
}

1;
'@ | Set-Content -Encoding ASCII $makeMakerFile
    @'
package Pod::Usage;
use strict;
use warnings;
use Exporter 'import';
our @EXPORT_OK = qw(pod2usage);
our @EXPORT = qw(pod2usage);

sub pod2usage {
    my %args = @_ == 1 ? ( -message => $_[0] ) : @_;
    my $message = $args{-message} || $args{message} || '';
    print STDERR "$message\n" if $message ne '';
    exit($args{-exitval}) if defined $args{-exitval};
}

1;
'@ | Set-Content -Encoding ASCII $podUsageFile
    if ($env:PERL5LIB) {
        $env:PERL5LIB = "$ShimRoot;$env:PERL5LIB"
    } else {
        $env:PERL5LIB = $ShimRoot
    }
}

$manifest = Get-Content (Join-Path $Root "third_party/dependencies.lock.json") -Raw | ConvertFrom-Json
$thirdParty = Join-Path $Root "third_party"
$downloads = Join-Path $thirdParty "downloads"
$sources = Join-Path $thirdParty "src"
$tools = Join-Path $thirdParty "tools"
$install = Join-Path $thirdParty "install/windows-x86_64"
$opensslPrefix = Join-Path $install "openssl"
$liboqsPrefix = Join-Path $install "liboqs"

Require-Command gcc | Out-Null
Require-Command perl | Out-Null
Require-Command mingw32-make | Out-Null
Ensure-GitPerlCompatibilityShim (Join-Path $thirdParty "perl_shims")

$cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source
if (-not $cmake) {
    $cmakeZip = Join-Path $downloads "cmake-$($manifest.buildTools.cmake.version)-windows-x86_64.zip"
    $cmakeRoot = Join-Path $tools "cmake-$($manifest.buildTools.cmake.version)-windows-x86_64"
    Download-File $manifest.buildTools.cmake.windowsUrl $cmakeZip
    if (-not (Test-Path (Join-Path $cmakeRoot "bin/cmake.exe")) -or $Force) {
        if (Test-Path $cmakeRoot) {
            Remove-Item -LiteralPath $cmakeRoot -Recurse -Force
        }
        Expand-Archive -Path $cmakeZip -DestinationPath $tools -Force
    }
    $cmake = Join-Path $cmakeRoot "bin/cmake.exe"
}

if (-not $SkipOpenSSL) {
    $opensslArchive = Join-Path $downloads "openssl-$($manifest.dependencies.openssl.version).tar.gz"
    $opensslSource = Join-Path $sources "openssl-$($manifest.dependencies.openssl.version)"
    $opensslMarker = Join-Path $opensslSource ".hesia-extracted"
    $opensslLib = Join-Path $opensslPrefix "lib/libcrypto.a"
    $opensslLib64 = Join-Path $opensslPrefix "lib64/libcrypto.a"
    Download-File $manifest.dependencies.openssl.url $opensslArchive
    Expand-TarGz $opensslArchive $opensslSource $opensslMarker
    if (((-not (Test-Path $opensslLib)) -and (-not (Test-Path $opensslLib64))) -or $Force) {
        $opensslPrefixArg = $opensslPrefix.Replace("\", "/")
        $opensslDirArg = "$opensslPrefixArg/ssl"
        Push-Location $opensslSource
        try {
            if (Test-Path "Makefile") {
                mingw32-make clean | Out-Null
            }
            Invoke-Checked "perl" @(
                "Configure",
                $manifest.dependencies.openssl.windowsConfigureTarget,
                "no-shared",
                "no-tests",
                "no-module",
                "no-asm",
                "--prefix=$opensslPrefixArg",
                "--openssldir=$opensslDirArg"
            ) $opensslSource
            Invoke-Checked "mingw32-make" @("-j$([Environment]::ProcessorCount)") $opensslSource
            Invoke-Checked "mingw32-make" @("install_sw") $opensslSource
        } finally {
            Pop-Location
        }
    }
}

if (-not $SkipLiboqs) {
    $liboqsArchive = Join-Path $downloads "liboqs-$($manifest.dependencies.liboqs.version).tar.gz"
    $liboqsSource = Join-Path $sources "liboqs-$($manifest.dependencies.liboqs.version)"
    $liboqsMarker = Join-Path $liboqsSource ".hesia-extracted"
    $liboqsBuild = Join-Path $thirdParty "build/windows-x86_64/liboqs"
    $liboqsLib = Join-Path $liboqsPrefix "lib/liboqs.a"
    Download-File $manifest.dependencies.liboqs.url $liboqsArchive
    Expand-TarGz $liboqsArchive $liboqsSource $liboqsMarker
    if ((-not (Test-Path $liboqsLib)) -or $Force) {
        if (Test-Path $liboqsBuild) {
            Remove-Item -LiteralPath $liboqsBuild -Recurse -Force
        }
        New-Item -ItemType Directory -Force $liboqsBuild | Out-Null
        Invoke-Checked $cmake @(
            "-S", $liboqsSource,
            "-B", $liboqsBuild,
            "-G", "MinGW Makefiles",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DCMAKE_INSTALL_PREFIX=$liboqsPrefix",
            "-DBUILD_SHARED_LIBS=OFF",
            "-DOQS_BUILD_ONLY_LIB=ON",
            "-DOQS_USE_OPENSSL=OFF",
            "-DOQS_MINIMAL_BUILD=$($manifest.dependencies.liboqs.minimalAlgorithms -join ';')"
        ) $Root
        Invoke-Checked $cmake @("--build", $liboqsBuild, "--config", "Release", "--parallel") $Root
        Invoke-Checked $cmake @("--install", $liboqsBuild) $Root
    }
}

Write-Host "Dependency install root: $install"
