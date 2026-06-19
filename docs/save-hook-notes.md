# Save Hook Notes

`Saving::ShouldAllowSave()` contains the policy SEDAS wants:

- if `SaveOnlyAtBeds` is false, allow all save types
- if `SaveOnlyAtBeds` is true, allow saves during the post-sleep bed window
- if `AllowExitSave` is true, allow exit saves while `SaveOnlyAtBeds` is true

SEDAS now installs a trampoline on the CommonLibSSE-NG `BGSSaveLoadManager::Save` relocation when `InstallSaveHook` is true. The hook is policy-only: disabling `SaveOnlyAtBeds` allows saves through. If an older config has `SaveOnlyAtBeds = true` and `InstallSaveHook = false`, SEDAS enables the hook for that session so the saving policy still works.

When `AutoSaveAtBedInsteadOfWindow` is true, SEDAS queues a post-sleep autosave on the next SKSE task instead of opening a timed manual-save window. That autosave is marked as an internal SEDAS save so the save hook allows it even when ordinary autosaves are blocked.

The hook is intentionally narrow. It blocks the actual save request instead of trying to hide menu entries or infer input events.
