param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [ValidateSet("openssl", "liboqs", "cmake", "wix")]
    [string[]]$Name = @("openssl", "liboqs"),
    [switch]$Force
)

# Fetch and verify the third-party archives pinned in third_party/dependencies.lock.json.
# Sources are extracted under third_party/src/, Windows build tools under third_party/tools/.

$ErrorActionPreference = "Stop"

$manifest = Get-Content (Join-Path $Root "third_party/dependencies.lock.json") -Raw | ConvertFrom-Json
$thirdParty = Join-Path $Root "third_party"
$downloads = Join-Path $thirdParty "downloads"
$sources = Join-Path $thirdParty "src"
$tools = Join-Path $thirdParty "tools"

# Prefer the Windows bsdtar: a GNU tar picked up from Git's usr/bin reads "C:\..." as a remote host.
$tar = Join-Path $env:SystemRoot "System32\tar.exe"
if (-not (Test-Path $tar)) {
    $tar = "tar"
}

function Get-VerifiedArchive {
    param([string]$Url, [string]$OutFile, [string]$Sha256)
    if ((Test-Path $OutFile) -and $Force) {
        Remove-Item -LiteralPath $OutFile -Force
    }
    if (-not (Test-Path $OutFile)) {
        New-Item -ItemType Directory -Force (Split-Path $OutFile) | Out-Null
        Write-Host "Downloading $Url"
        $partial = "$OutFile.part"
        Invoke-WebRequest -Uri $Url -OutFile $partial -UseBasicParsing
        Move-Item -LiteralPath $partial -Destination $OutFile -Force
    }
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $OutFile).Hash.ToLowerInvariant()
    if ($actual -ne $Sha256.ToLowerInvariant()) {
        throw "SHA-256 mismatch for ${OutFile}`n  expected $Sha256`n  actual   $actual`nDelete the file and run this script again."
    }
}

function Expand-TarGz {
    param([string]$Archive, [string]$Destination)
    $marker = Join-Path $Destination ".hesia-extracted"
    if ((Test-Path $marker) -and -not $Force) {
        return
    }
    if (Test-Path $Destination) {
        Remove-Item -LiteralPath $Destination -Recurse -Force
    }
    New-Item -ItemType Directory -Force $Destination | Out-Null
    & $tar -xzf $Archive -C $Destination --strip-components 1
    if ($LASTEXITCODE -ne 0) {
        throw "tar failed for $Archive"
    }
    New-Item -ItemType File -Force $marker | Out-Null
}

foreach ($dep in $Name) {
    switch ($dep) {
        "openssl" {
            $entry = $manifest.dependencies.openssl
            $archive = Join-Path $downloads "openssl-$($entry.version).tar.gz"
            Get-VerifiedArchive $entry.url $archive $entry.sha256
            Expand-TarGz $archive (Join-Path $sources "openssl-$($entry.version)")
        }
        "liboqs" {
            $entry = $manifest.dependencies.liboqs
            $archive = Join-Path $downloads "liboqs-$($entry.version).tar.gz"
            Get-VerifiedArchive $entry.url $archive $entry.sha256
            Expand-TarGz $archive (Join-Path $sources "liboqs-$($entry.version)")
        }
        "cmake" {
            $entry = $manifest.buildTools.cmake
            $archive = Join-Path $downloads "cmake-$($entry.version)-windows-x86_64.zip"
            $dest = Join-Path $tools "cmake-$($entry.version)-windows-x86_64"
            Get-VerifiedArchive $entry.windowsUrl $archive $entry.sha256
            if ($Force -or -not (Test-Path (Join-Path $dest "bin/cmake.exe"))) {
                if (Test-Path $dest) {
                    Remove-Item -LiteralPath $dest -Recurse -Force
                }
                Expand-Archive -Path $archive -DestinationPath $tools -Force
            }
        }
        "wix" {
            $entry = $manifest.buildTools.wix
            $archive = Join-Path $downloads "wix-$($entry.version).zip"
            $dest = Join-Path $tools "wix-$($entry.version)"
            Get-VerifiedArchive $entry.windowsUrl $archive $entry.sha256
            $complete = (Test-Path (Join-Path $dest "candle.exe")) -and
                (Test-Path (Join-Path $dest "light.exe")) -and
                (Test-Path (Join-Path $dest "heat.exe"))
            if ($Force -or -not $complete) {
                if (Test-Path $dest) {
                    Remove-Item -LiteralPath $dest -Recurse -Force
                }
                New-Item -ItemType Directory -Force $dest | Out-Null
                Expand-Archive -Path $archive -DestinationPath $dest -Force
            }
        }
    }
}

Write-Host "Third-party archives verified: $($Name -join ', ')"
