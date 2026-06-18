# Save Hook Notes

`Saving::ShouldAllowSave()` contains the policy SEDAS wants:

- if `SaveOnlyAtBeds` is false, allow all save types
- if `SaveOnlyAtBeds` is true, allow saves during the post-sleep bed window
- if `AllowExitSave` is true, allow exit saves while `SaveOnlyAtBeds` is true

The remaining work is the actual save interception for the latest runtime.

The safe target is the internal `BGSSaveLoadManager` save path, not menu text or input events. `BGSSaveLoadManager::Save()` is public in CommonLibSSE-NG, but the enforceable gate is lower than that and should be confirmed against the current Address Library IDs before a trampoline is installed.

Until this hook is confirmed, `InstallSaveHook = false` is the default.
