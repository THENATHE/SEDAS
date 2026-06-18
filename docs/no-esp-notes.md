# No-ESP Notes

SEDAS is intended to run as a DLL-only SKSE plugin.

What works cleanly without an ESP:

- SKSE Menu Framework configuration pages
- TOML configuration
- bed tracking and bed recall
- save policy decisions
- dragon soul stat bonuses
- downed/revive death alternative
- SKSE serialization

Dragon Aspect is handled without an ESP by trying to resolve vanilla Dragonborn spell editor IDs at runtime. If Bethesda's editor IDs differ from the built-in candidates, set these override values in `Data/SKSE/Plugins/SEDAS.toml`:

```toml
[DragonAspect]
Rank1SpellEditorID = ""
Rank2SpellEditorID = ""
Rank3SpellEditorID = ""
```

No Creation Kit or xEdit workflow is required for those overrides; they are plain text.
