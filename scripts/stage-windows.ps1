param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$Stage = (Join-Path (Resolve-Path (Join-Path $PSScriptRoot "..")).Path "dist/windows/hesia-core-0.1.0")
)

$ErrorActionPreference = "Stop"

if (Test-Path $Stage) {
    Remove-Item -LiteralPath $Stage -Recurse -Force
}

$bin = Join-Path $Stage "bin"
$lib = Join-Path $Stage "lib"
$include = Join-Path $Stage "include"
$bindings = Join-Path $Stage "bindings"
$docs = Join-Path $Stage "docs"
$thirdParty = Join-Path $Stage "third_party"
New-Item -ItemType Directory -Force $bin,$lib,$include,$bindings,$docs,$thirdParty | Out-Null

Copy-Item (Join-Path $Root "build/windows-x86_64/bin/hesia_core.dll") $bin
Copy-Item (Join-Path $Root "build/windows-x86_64/lib/libhesia_core.a") $lib
Copy-Item (Join-Path $Root "include/hesia") $include -Recurse
Copy-Item (Join-Path $Root "bindings/python") $bindings -Recurse
Copy-Item (Join-Path $Root "bindings/rust") $bindings -Recurse
Copy-Item (Join-Path $Root "bindings/swift") $bindings -Recurse
Copy-Item (Join-Path $Root "bindings/ada") $bindings -Recurse
Copy-Item (Join-Path $Root "README.md") $Stage
Copy-Item (Join-Path $Root "LICENSE") $Stage
Copy-Item (Join-Path $Root "NOTICE") $Stage
Copy-Item (Join-Path $Root "SECURITY.md") $Stage
Copy-Item (Join-Path $Root "third_party/dependencies.lock.json") $thirdParty
$stageDepsInstall = Join-Path $thirdParty "install"
New-Item -ItemType Directory -Force $stageDepsInstall | Out-Null
Copy-Item (Join-Path $Root "third_party/install/windows-x86_64") $stageDepsInstall -Recurse

$repoDocs = Resolve-Path (Join-Path $Root "..\..\docs")
Copy-Item (Join-Path $repoDocs "HESIA_CORELIB_EN.md") $docs
Copy-Item (Join-Path $repoDocs "HESIA_CORELIB_FR.md") $docs
Copy-Item (Join-Path $repoDocs "HESIA_CORELIB_EN.pdf") $docs
Copy-Item (Join-Path $repoDocs "HESIA_CORELIB_FR.pdf") $docs

Write-Host "Windows stage ready: $Stage"
