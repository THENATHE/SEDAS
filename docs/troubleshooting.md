# Troubleshooting

## SKSE reports plugin 126

Example:

```text
plugin SEDAS.dll (...) couldn't load plugin 126
```

Windows error 126 means the loader found `SEDAS.dll`, but a dependent DLL was missing. Current builds use the `x64-windows-static-md` vcpkg triplet so vcpkg dependencies are linked into `SEDAS.dll`.

If error 126 still happens after installing a current build, install the Microsoft Visual C++ 2015-2022 runtime into the Skyrim Proton prefix:

```bash
protontricks 489830 vcrun2022
```

Then launch through SKSE again and check:

```text
Documents/My Games/Skyrim Special Edition/SKSE/skse64.log
Documents/My Games/Skyrim Special Edition/SKSE/SEDAS.log
```

## SEDAS loaded, but no SEDAS.log exists

SEDAS tries the normal SKSE log directory first:

```text
Documents/My Games/Skyrim Special Edition/SKSE/SEDAS.log
```

If that path is not available, current builds fall back to writing beside the plugin:

```text
Data/SKSE/Plugins/SEDAS.log
```
