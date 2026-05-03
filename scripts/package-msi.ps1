param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$Stage = (Join-Path (Resolve-Path (Join-Path $PSScriptRoot "..")).Path "dist/windows/hesia-core-0.1.0"),
    [string]$Output = (Join-Path (Resolve-Path (Join-Path $PSScriptRoot "..")).Path "dist/HesiaCore-0.1.0-x64.msi"),
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

function Download-File {
    param([string]$Url, [string]$OutFile)
    if (Test-Path $OutFile) {
        return
    }
    New-Item -ItemType Directory -Force (Split-Path $OutFile) | Out-Null
    Write-Host "Downloading $Url"
    Invoke-WebRequest -Uri $Url -OutFile $OutFile -UseBasicParsing
}

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

if (-not $SkipBuild) {
    & (Join-Path $PSScriptRoot "build-windows.ps1") -Root $Root
}
& (Join-Path $PSScriptRoot "stage-windows.ps1") -Root $Root -Stage $Stage

$manifest = Get-Content (Join-Path $Root "third_party/dependencies.lock.json") -Raw | ConvertFrom-Json
$wixDir = Join-Path $Root "third_party/tools/wix-$($manifest.buildTools.wix.version)"
$candle = Join-Path $wixDir "candle.exe"
$light = Join-Path $wixDir "light.exe"
$heat = Join-Path $wixDir "heat.exe"
if (-not (Test-Path $candle) -or -not (Test-Path $light) -or -not (Test-Path $heat)) {
    $zip = Join-Path $Root "third_party/downloads/wix-$($manifest.buildTools.wix.version).zip"
    Download-File $manifest.buildTools.wix.windowsUrl $zip
    if (Test-Path $wixDir) {
        Remove-Item -LiteralPath $wixDir -Recurse -Force
    }
    New-Item -ItemType Directory -Force $wixDir | Out-Null
    Expand-Archive -Path $zip -DestinationPath $wixDir -Force
}

$wixWork = Join-Path $Root "build/msi"
if (Test-Path $wixWork) {
    Remove-Item -LiteralPath $wixWork -Recurse -Force
}
New-Item -ItemType Directory -Force $wixWork | Out-Null

$wxs = @"
<?xml version="1.0" encoding="UTF-8"?>
<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi">
  <Product Id="*" Name="HESIA CoreLib" Language="1033" Version="0.1.0" Manufacturer="HESIA" UpgradeCode="19D7303C-003B-4CC6-8E16-5E4F01D1F20D">
    <Package InstallerVersion="500" Compressed="yes" InstallScope="perMachine" Platform="x64" />
    <MajorUpgrade DowngradeErrorMessage="A newer HESIA CoreLib version is already installed." />
    <MediaTemplate EmbedCab="yes" />
    <Feature Id="DefaultFeature" Title="HESIA CoreLib" Level="1">
      <ComponentGroupRef Id="HesiaCoreFiles" />
    </Feature>
    <Directory Id="TARGETDIR" Name="SourceDir">
      <Directory Id="ProgramFiles64Folder">
        <Directory Id="INSTALLFOLDER" Name="HESIA CoreLib" />
      </Directory>
    </Directory>
  </Product>
</Wix>
"@

$wxsPath = Join-Path $wixWork "HesiaCore.wxs"
$harvestPath = Join-Path $wixWork "HesiaCoreFiles.wxs"
$wixObj = Join-Path $wixWork "HesiaCore.wixobj"
$harvestObj = Join-Path $wixWork "HesiaCoreFiles.wixobj"
$wxs | Set-Content -Encoding UTF8 $wxsPath
New-Item -ItemType Directory -Force (Split-Path $Output) | Out-Null
Invoke-Checked $heat @(
    "dir", $Stage,
    "-cg", "HesiaCoreFiles",
    "-dr", "INSTALLFOLDER",
    "-srd",
    "-sreg",
    "-sfrag",
    "-gg",
    "-var", "var.SourceDir",
    "-out", $harvestPath
) $Root
Invoke-Checked $candle @("-arch", "x64", "-dSourceDir=$Stage", "-out", $wixObj, $wxsPath) $Root
Invoke-Checked $candle @("-arch", "x64", "-dSourceDir=$Stage", "-out", $harvestObj, $harvestPath) $Root
Invoke-Checked $light @("-out", $Output, $wixObj, $harvestObj) $Root
Write-Host "MSI ready: $Output"
