# SEDAS

**Survival Saving, Soul Economy, and Death Alternative System** is a Skyrim Special Edition SKSE plugin scaffold targeting the latest AE runtime with CommonLibSSE-NG and Address Library.

Implemented in this first pass:

- CommonLibSSE-NG CMake/vcpkg project layout
- TOML configuration in `Data/SKSE/Plugins/SEDAS.toml`
- SKSE serialization for SEDAS runtime state
- SKSE Menu Framework v3 pages when `SKSEMenuFramework.h` is provided
- bed tracking via sleep-capable furniture events
- dragon soul based actor-value bonus reconciliation
- death alternative event flow using essential/bleedout recovery
- DLL-only Dragon Aspect lookup through vanilla Dragonborn editor IDs
- isolated save policy and hook notes

Important current limitation:

The save-only-at-beds policy exists, but the actual low-level save hook is intentionally not installed by default. See `docs/save-hook-notes.md`.

## Dependencies

- SKSE64 for the current/latest Skyrim SE runtime
- Address Library for SKSE Plugins
- CommonLibSSE-NG
- SKSE Menu Framework v3.9 or newer for in-game menus
- SSE Engine Fixes if you use SKSE Menu Framework

Nexus currently lists SKSE Menu Framework v3.9, and its docs show client mods registering pages with `SetSection()` and `AddSectionItem()`.

## Build

The supported build path is MSVC/vcpkg. The easiest route from Arch Linux is to push `SEDAS` as a GitHub repo and run the included Windows workflow.

For a local Windows builder:

```powershell
$env:VCPKG_ROOT = "C:\src\vcpkg"
git clone https://github.com/microsoft/vcpkg $env:VCPKG_ROOT
& "$env:VCPKG_ROOT\bootstrap-vcpkg.bat"
.\scripts\fetch-skmf-header.ps1
cmake --preset vs2022
cmake --build --preset release
```

## No ESP

SEDAS is currently designed to avoid ESP/ESL authoring. See `docs/no-esp-notes.md`.

## Arch Linux

Arch is a good development host for editing and testing under Steam/Proton, but the actual SKSE DLL should be built with an MSVC-compatible Windows toolchain. See `docs/arch-linux-dev-setup.md`.
