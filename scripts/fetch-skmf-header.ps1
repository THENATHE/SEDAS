$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$target = Join-Path $root "extern/SKSEMenuFramework/SKSEMenuFramework.h"
$url = "https://raw.githubusercontent.com/QTR-Modding/SKSE-Menu-Framework-3/master/resources/SKSEMenuFramework.h"

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
Invoke-WebRequest -Uri $url -OutFile $target
Write-Host "Downloaded SKSEMenuFramework.h to $target"
