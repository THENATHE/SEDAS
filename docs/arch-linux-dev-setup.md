# Arch Linux Development Setup

SEDAS can be edited and tested from Arch Linux, but the final SKSE plugin is a Windows DLL. CommonLibSSE-NG documents Visual Studio 2022 as a build dependency and notes that Linux cross-compilation is not a ready path because MSVC ABI/linking is still the real constraint.

## Recommended Route

Use Arch for editing, local scripts, packaging, Steam/Proton testing, logs, and source control. Use a Windows builder for the actual DLL:

- GitHub Actions `windows-2022` runner using `.github/workflows/build.yml`
- a small Windows VM
- a Windows machine running a self-hosted GitHub Actions runner

This avoids Creation Kit, xEdit, and ESP authoring entirely.

## Install On Arch

```bash
sudo pacman -S --needed \
  base-devel git cmake ninja clang llvm lld clang-tools-extra \
  python unzip p7zip zip jq github-cli steam
```

Optional, useful for Proton/mod troubleshooting:

```bash
sudo pacman -S --needed wine winetricks protontricks
```

## Put These In Place

1. Install Skyrim Special Edition from Steam and launch it once normally.
2. Install the current SKSE64 AE build from `https://skse.silverlock.org/`.
3. Install Address Library for SKSE Plugins into Skyrim's `Data` folder.
4. Install SKSE Menu Framework and its runtime requirements into Skyrim's `Data` folder.
5. Download the SKSE Menu Framework developer header:

```bash
bash ./scripts/fetch-skmf-header.sh
```

Or place it manually here:

```text
SEDAS/extern/SKSEMenuFramework/SKSEMenuFramework.h
```

## What I Need From You

Send me:

- the absolute path to your Skyrim Special Edition install
- whether Steam installed Skyrim under the default library path or a custom library
- the exact SKSE archive you installed
- whether you want GitHub Actions builds or a local Windows VM/self-hosted runner
- any crash logs from `Documents/My Games/Skyrim Special Edition/SKSE` after testing

## GitHub Actions Build

If `SEDAS` is pushed as the root of a GitHub repository, the included workflow builds the DLL on a Windows runner and uploads `SEDAS.zip`.

The archive layout is:

```text
Data/SKSE/Plugins/SEDAS.dll
Data/SKSE/Plugins/SEDAS.toml
```

Install that `Data` folder into Skyrim through your mod manager or directly into the game folder while testing.
