#include "SEDAS/Settings.h"

namespace
{
	const std::filesystem::path kSettingsPath{ "Data/SKSE/Plugins/SEDAS.toml" };

	template <class T>
	T ReadValue(const toml::table& a_table, std::string_view a_section, std::string_view a_key, T a_fallback)
	{
		auto section = a_table[a_section].as_table();
		if (!section) {
			return a_fallback;
		}

		if (auto value = (*section)[a_key].value<T>()) {
			return *value;
		}

		return a_fallback;
	}

	void WriteBool(std::ostream& a_out, std::string_view a_key, bool a_value)
	{
		a_out << a_key << " = " << (a_value ? "true" : "false") << '\n';
	}
}

namespace SEDAS::Settings
{
	Config& Get()
	{
		static Config config;
		return config;
	}

	const std::filesystem::path& Path()
	{
		return kSettingsPath;
	}

	void Load()
	{
		auto& config = Get();

		try {
			if (!std::filesystem::exists(kSettingsPath)) {
				logger::warn("Settings file not found: {}", kSettingsPath.string());
				return;
			}

			const auto table = toml::parse_file(kSettingsPath.string());

			config.saving.saveOnlyAtBeds = ReadValue(table, "Saving", "SaveOnlyAtBeds", config.saving.saveOnlyAtBeds);
			config.saving.allowExitSave = ReadValue(table, "Saving", "AllowExitSave", config.saving.allowExitSave);
			config.saving.bedSaveWindowSeconds = std::max(0, ReadValue(table, "Saving", "BedSaveWindowSeconds", config.saving.bedSaveWindowSeconds));
			config.saving.installSaveHook = ReadValue(table, "Saving", "InstallSaveHook", config.saving.installSaveHook);

			config.death.enabled = ReadValue(table, "DeathAlternative", "Enabled", config.death.enabled);
			config.death.consumeDragonSoulToRevive = ReadValue(table, "DeathAlternative", "ConsumeDragonSoulToRevive", config.death.consumeDragonSoulToRevive);
			config.death.recallToLastBedWhenNoSouls = ReadValue(table, "DeathAlternative", "RecallToLastBedWhenNoSouls", config.death.recallToLastBedWhenNoSouls);
			config.death.downedDurationSeconds = std::max(0, ReadValue(table, "DeathAlternative", "DownedDurationSeconds", config.death.downedDurationSeconds));
			config.death.disableControlsWhileDowned = ReadValue(table, "DeathAlternative", "DisableControlsWhileDowned", config.death.disableControlsWhileDowned);

			config.soulEconomy.enabled = ReadValue(table, "SoulEconomy", "Enabled", config.soulEconomy.enabled);
			config.soulEconomy.soulCap = std::max(0, ReadValue(table, "SoulEconomy", "SoulCap", config.soulEconomy.soulCap));
			config.soulEconomy.enableCarryWeight = ReadValue(table, "SoulEconomy", "EnableCarryWeight", config.soulEconomy.enableCarryWeight);
			config.soulEconomy.carryWeightPerSoul = ReadValue(table, "SoulEconomy", "CarryWeightPerSoul", config.soulEconomy.carryWeightPerSoul);
			config.soulEconomy.enableHealth = ReadValue(table, "SoulEconomy", "EnableHealth", config.soulEconomy.enableHealth);
			config.soulEconomy.healthPerSoul = ReadValue(table, "SoulEconomy", "HealthPerSoul", config.soulEconomy.healthPerSoul);
			config.soulEconomy.enableStamina = ReadValue(table, "SoulEconomy", "EnableStamina", config.soulEconomy.enableStamina);
			config.soulEconomy.staminaPerSoul = ReadValue(table, "SoulEconomy", "StaminaPerSoul", config.soulEconomy.staminaPerSoul);
			config.soulEconomy.enableMagicka = ReadValue(table, "SoulEconomy", "EnableMagicka", config.soulEconomy.enableMagicka);
			config.soulEconomy.magickaPerSoul = ReadValue(table, "SoulEconomy", "MagickaPerSoul", config.soulEconomy.magickaPerSoul);
			config.soulEconomy.enableWeaponDamage = ReadValue(table, "SoulEconomy", "EnableWeaponDamage", config.soulEconomy.enableWeaponDamage);
			config.soulEconomy.weaponDamagePercentPerSoul = ReadValue(table, "SoulEconomy", "WeaponDamagePercentPerSoul", config.soulEconomy.weaponDamagePercentPerSoul);
			config.soulEconomy.enableSpellCostReduction = ReadValue(table, "SoulEconomy", "EnableSpellCostReduction", config.soulEconomy.enableSpellCostReduction);
			config.soulEconomy.spellCostReductionPercentPerSoul = ReadValue(table, "SoulEconomy", "SpellCostReductionPercentPerSoul", config.soulEconomy.spellCostReductionPercentPerSoul);

			config.dragonAspect.enabled = ReadValue(table, "DragonAspect", "Enabled", config.dragonAspect.enabled);
			config.dragonAspect.useVanillaDragonAspect = ReadValue(table, "DragonAspect", "UseVanillaDragonAspect", config.dragonAspect.useVanillaDragonAspect);
			config.dragonAspect.rank1SoulThreshold = std::max(0, ReadValue(table, "DragonAspect", "Rank1SoulThreshold", config.dragonAspect.rank1SoulThreshold));
			config.dragonAspect.rank2SoulThreshold = std::max(0, ReadValue(table, "DragonAspect", "Rank2SoulThreshold", config.dragonAspect.rank2SoulThreshold));
			config.dragonAspect.rank3SoulThreshold = std::max(0, ReadValue(table, "DragonAspect", "Rank3SoulThreshold", config.dragonAspect.rank3SoulThreshold));
			config.dragonAspect.rank1SpellEditorID = ReadValue(table, "DragonAspect", "Rank1SpellEditorID", config.dragonAspect.rank1SpellEditorID);
			config.dragonAspect.rank2SpellEditorID = ReadValue(table, "DragonAspect", "Rank2SpellEditorID", config.dragonAspect.rank2SpellEditorID);
			config.dragonAspect.rank3SpellEditorID = ReadValue(table, "DragonAspect", "Rank3SpellEditorID", config.dragonAspect.rank3SpellEditorID);
		} catch (const std::exception& e) {
			logger::error("Failed to load settings: {}", e.what());
		}
	}

	bool Save()
	{
		try {
			std::filesystem::create_directories(kSettingsPath.parent_path());
			std::ofstream out(kSettingsPath, std::ios::trunc);
			if (!out) {
				logger::error("Could not open settings for writing: {}", kSettingsPath.string());
				return false;
			}

			const auto& config = Get();

			out << "[Saving]\n";
			WriteBool(out, "SaveOnlyAtBeds", config.saving.saveOnlyAtBeds);
			WriteBool(out, "AllowExitSave", config.saving.allowExitSave);
			out << "BedSaveWindowSeconds = " << config.saving.bedSaveWindowSeconds << "\n";
			WriteBool(out, "InstallSaveHook", config.saving.installSaveHook);

			out << "\n[DeathAlternative]\n";
			WriteBool(out, "Enabled", config.death.enabled);
			WriteBool(out, "ConsumeDragonSoulToRevive", config.death.consumeDragonSoulToRevive);
			WriteBool(out, "RecallToLastBedWhenNoSouls", config.death.recallToLastBedWhenNoSouls);
			out << "DownedDurationSeconds = " << config.death.downedDurationSeconds << "\n";
			WriteBool(out, "DisableControlsWhileDowned", config.death.disableControlsWhileDowned);

			out << "\n[SoulEconomy]\n";
			WriteBool(out, "Enabled", config.soulEconomy.enabled);
			out << "SoulCap = " << config.soulEconomy.soulCap << "\n\n";
			WriteBool(out, "EnableCarryWeight", config.soulEconomy.enableCarryWeight);
			out << "CarryWeightPerSoul = " << config.soulEconomy.carryWeightPerSoul << "\n\n";
			WriteBool(out, "EnableHealth", config.soulEconomy.enableHealth);
			out << "HealthPerSoul = " << config.soulEconomy.healthPerSoul << "\n\n";
			WriteBool(out, "EnableStamina", config.soulEconomy.enableStamina);
			out << "StaminaPerSoul = " << config.soulEconomy.staminaPerSoul << "\n\n";
			WriteBool(out, "EnableMagicka", config.soulEconomy.enableMagicka);
			out << "MagickaPerSoul = " << config.soulEconomy.magickaPerSoul << "\n\n";
			WriteBool(out, "EnableWeaponDamage", config.soulEconomy.enableWeaponDamage);
			out << "WeaponDamagePercentPerSoul = " << config.soulEconomy.weaponDamagePercentPerSoul << "\n\n";
			WriteBool(out, "EnableSpellCostReduction", config.soulEconomy.enableSpellCostReduction);
			out << "SpellCostReductionPercentPerSoul = " << config.soulEconomy.spellCostReductionPercentPerSoul << "\n";

			out << "\n[DragonAspect]\n";
			WriteBool(out, "Enabled", config.dragonAspect.enabled);
			WriteBool(out, "UseVanillaDragonAspect", config.dragonAspect.useVanillaDragonAspect);
			out << "Rank1SoulThreshold = " << config.dragonAspect.rank1SoulThreshold << "\n";
			out << "Rank2SoulThreshold = " << config.dragonAspect.rank2SoulThreshold << "\n";
			out << "Rank3SoulThreshold = " << config.dragonAspect.rank3SoulThreshold << "\n";
			out << "Rank1SpellEditorID = \"" << config.dragonAspect.rank1SpellEditorID << "\"\n";
			out << "Rank2SpellEditorID = \"" << config.dragonAspect.rank2SpellEditorID << "\"\n";
			out << "Rank3SpellEditorID = \"" << config.dragonAspect.rank3SpellEditorID << "\"\n";

			return true;
		} catch (const std::exception& e) {
			logger::error("Failed to save settings: {}", e.what());
			return false;
		}
	}
}
