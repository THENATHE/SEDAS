#pragma once

#include "SEDAS/PCH.h"

namespace SEDAS::Settings
{
	struct SavingConfig
	{
		bool saveOnlyAtBeds = true;
		bool allowExitSave = true;
		bool autoSaveAtBedInsteadOfWindow = false;
		int bedSaveWindowSeconds = 20;
		bool installSaveHook = true;
	};

	struct DeathAlternativeConfig
	{
		bool enabled = true;
		bool consumeDragonSoulToRevive = true;
		bool recallToLastBedWhenNoSouls = true;
		int downedDurationSeconds = 4;
		bool disableControlsWhileDowned = true;
		bool ragdollInsteadOfBleedout = false;
		bool playDragonSoulAbsorbEffect = true;
		std::string soulAbsorbEffectShaderEditorID;
		std::string soulAbsorbArtObjectEditorID;
	};

	struct SoulEconomyConfig
	{
		bool enabled = true;
		int soulCap = 20;

		bool enableCarryWeight = true;
		float carryWeightPerSoul = 5.0F;

		bool enableHealth = true;
		float healthPerSoul = 5.0F;

		bool enableStamina = true;
		float staminaPerSoul = 5.0F;

		bool enableMagicka = true;
		float magickaPerSoul = 5.0F;

		bool enableWeaponDamage = true;
		float weaponDamagePercentPerSoul = 1.0F;

		bool enableSpellCostReduction = true;
		float spellCostReductionPercentPerSoul = 1.0F;
	};

	struct DragonAspectConfig
	{
		bool enabled = true;
		bool useVanillaDragonAspect = true;
		int rank1SoulThreshold = 5;
		int rank2SoulThreshold = 10;
		int rank3SoulThreshold = 20;
		std::string rank1SpellEditorID;
		std::string rank2SpellEditorID;
		std::string rank3SpellEditorID;
	};

	struct Config
	{
		SavingConfig saving;
		DeathAlternativeConfig death;
		SoulEconomyConfig soulEconomy;
		DragonAspectConfig dragonAspect;
	};

	Config& Get();
	const std::filesystem::path& Path();
	void Load();
	bool Save();
}
