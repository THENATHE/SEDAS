#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
target="$root/extern/SKSEMenuFramework/SKSEMenuFramework.h"
url="https://raw.githubusercontent.com/QTR-Modding/SKSE-Menu-Framework-3/master/resources/SKSEMenuFramework.h"

mkdir -p "$(dirname "$target")"
curl -L "$url" -o "$target"
echo "Downloaded SKSEMenuFramework.h to $target"
