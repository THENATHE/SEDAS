#include "SEDAS/UI.h"

#include "SEDAS/DeathAlternative.h"
#include "SEDAS/Saving.h"
#include "SEDAS/Settings.h"
#include "SEDAS/SoulEconomy.h"
#include "SEDAS/State.h"

#if SEDAS_WITH_SKSE_MENU_FRAMEWORK
#	include "SKSEMenuFramework.h"
#endif

namespace
{
#if SEDAS_WITH_SKSE_MENU_FRAMEWORK
	void SaveAndRefresh()
	{
		SEDAS::Settings::Save();
		SEDAS::Saving::InstallHooks();
		SEDAS::Saving::RefreshSaveDisabledFlag();
		SEDAS::SoulEconomy::Refresh();
		SEDAS::DeathAlternative::RefreshPlayerEssentialFlag();
	}

	void RenderSaveButton()
	{
		if (ImGuiMCP::Button("Save")) {
			SaveAndRefresh();
		}
		ImGuiMCP::SameLine();
		if (ImGuiMCP::Button("Reload")) {
			SEDAS::Settings::Load();
			SEDAS::Saving::RefreshSaveDisabledFlag();
			SEDAS::SoulEconomy::Refresh();
			SEDAS::DeathAlternative::RefreshPlayerEssentialFlag();
		}
	}

	void __stdcall RenderSaving()
	{
		auto& config = SEDAS::Settings::Get().saving;
		ImGuiMCP::Checkbox("Save only at beds", &config.saveOnlyAtBeds);
		if (config.saveOnlyAtBeds) {
			config.installSaveHook = true;
		} else {
			config.allowExitSave = false;
			config.autoSaveAtBedInsteadOfWindow = false;
			config.installSaveHook = false;
		}

		ImGuiMCP::BeginDisabled(!config.saveOnlyAtBeds);
		ImGuiMCP::Checkbox("Allow exit save", &config.allowExitSave);
		ImGuiMCP::Checkbox("Autosave after sleeping", &config.autoSaveAtBedInsteadOfWindow);
		ImGuiMCP::EndDisabled();

		ImGuiMCP::BeginDisabled(!config.saveOnlyAtBeds || config.autoSaveAtBedInsteadOfWindow);
		ImGuiMCP::SliderInt("Bed save window seconds", &config.bedSaveWindowSeconds, 0, 120);
		ImGuiMCP::EndDisabled();

		ImGuiMCP::Separator();
		ImGuiMCP::Text("Bed save window: %s", SEDAS::Saving::IsInBedSaveWindow() ? "open" : "closed");
		RenderSaveButton();
	}

	void __stdcall RenderDeath()
	{
		auto& config = SEDAS::Settings::Get().death;
		auto& state = SEDAS::State::Get();

		ImGuiMCP::Checkbox("Enable death alternative", &config.enabled);
		ImGuiMCP::Checkbox("Consume dragon soul to revive", &config.consumeDragonSoulToRevive);
		ImGuiMCP::Checkbox("Recall to last bed with no souls", &config.recallToLastBedWhenNoSouls);
		ImGuiMCP::Checkbox("Disable controls while downed", &config.disableControlsWhileDowned);
		ImGuiMCP::Checkbox("Ragdoll instead of bleedout", &config.ragdollInsteadOfBleedout);
		ImGuiMCP::Checkbox("Play dragon soul absorb effect on revive", &config.playDragonSoulAbsorbEffect);
		ImGuiMCP::SliderInt("Downed duration seconds", &config.downedDurationSeconds, 0, 30);

		char effectShader[256]{};
		char artObject[256]{};
		std::strncpy(effectShader, config.soulAbsorbEffectShaderEditorID.c_str(), sizeof(effectShader) - 1);
		std::strncpy(artObject, config.soulAbsorbArtObjectEditorID.c_str(), sizeof(artObject) - 1);

		if (ImGuiMCP::InputText("Soul absorb effect shader editor ID", effectShader, sizeof(effectShader))) {
			config.soulAbsorbEffectShaderEditorID = effectShader;
		}
		if (ImGuiMCP::InputText("Soul absorb art object editor ID", artObject, sizeof(artObject))) {
			config.soulAbsorbArtObjectEditorID = artObject;
		}

		ImGuiMCP::Separator();
		ImGuiMCP::Text("Last bed: %08X", state.lastBedRefID);
		ImGuiMCP::Text("Last bed cell: %08X", state.lastBedCellID);
		ImGuiMCP::Text("Recovering: %s", state.recoveringFromDowned ? "yes" : "no");
		RenderSaveButton();
	}

	void RenderBonusFloat(const char* a_label, bool* a_enabled, float* a_value, float a_min, float a_max)
	{
		ImGuiMCP::Checkbox(a_label, a_enabled);
		ImGuiMCP::SameLine();
		ImGuiMCP::SetNextItemWidth(120.0F);
		ImGuiMCP::SliderFloat((std::string("##") + a_label).c_str(), a_value, a_min, a_max, "%.1f");
	}

	void RenderAppliedBonuses(const SEDAS::State::AppliedBonuses& a_applied)
	{
		ImGuiMCP::Text("Dragon souls: %d", SEDAS::SoulEconomy::GetDragonSoulCount());
		ImGuiMCP::Text("Effective iterations: %d", a_applied.iterations);
		ImGuiMCP::Separator();
		ImGuiMCP::Text("Soul Attunement - Carry Weight: +%.1f", a_applied.carryWeight);
		ImGuiMCP::Text("Soul Attunement - Health: +%.1f", a_applied.health);
		ImGuiMCP::Text("Soul Attunement - Stamina: +%.1f", a_applied.stamina);
		ImGuiMCP::Text("Soul Attunement - Magicka: +%.1f", a_applied.magicka);
		ImGuiMCP::Text("Soul Attunement - Weapon Damage: +%.1f%%", a_applied.weaponDamage);
		ImGuiMCP::Text("Soul Attunement - Spell Cost Reduction: +%.1f%%", a_applied.spellCostReduction);
	}

	void __stdcall RenderSoulEconomy()
	{
		auto& config = SEDAS::Settings::Get().soulEconomy;
		auto applied = SEDAS::State::Get().appliedBonuses;

		ImGuiMCP::Checkbox("Enable soul economy", &config.enabled);
		ImGuiMCP::SliderInt("Soul cap", &config.soulCap, 0, 200);
		ImGuiMCP::Separator();

		RenderBonusFloat("Carry weight per soul", &config.enableCarryWeight, &config.carryWeightPerSoul, 0.0F, 100.0F);
		RenderBonusFloat("Health per soul", &config.enableHealth, &config.healthPerSoul, 0.0F, 100.0F);
		RenderBonusFloat("Stamina per soul", &config.enableStamina, &config.staminaPerSoul, 0.0F, 100.0F);
		RenderBonusFloat("Magicka per soul", &config.enableMagicka, &config.magickaPerSoul, 0.0F, 100.0F);
		RenderBonusFloat("Weapon damage percent per soul", &config.enableWeaponDamage, &config.weaponDamagePercentPerSoul, 0.0F, 25.0F);
		RenderBonusFloat("Spell cost reduction percent per soul", &config.enableSpellCostReduction, &config.spellCostReductionPercentPerSoul, 0.0F, 25.0F);

		ImGuiMCP::Separator();
		RenderAppliedBonuses(applied);
		RenderSaveButton();
	}

	void __stdcall RenderDebugging()
	{
		if (ImGuiMCP::Button("Force fix death state")) {
			SEDAS::DeathAlternative::ForceFixPlayerState();
		}
		ImGuiMCP::SameLine();
		if (ImGuiMCP::Button("Force bed save window")) {
			SEDAS::Saving::OpenBedSaveWindow();
		}

		ImGuiMCP::Separator();
		if (ImGuiMCP::Button("Remove all Soul Attunement buffs")) {
			SEDAS::SoulEconomy::RemoveAppliedBonuses();
		}
		ImGuiMCP::SameLine();
		if (ImGuiMCP::Button("Reapply Soul Attunement buffs")) {
			SEDAS::SoulEconomy::Refresh();
		}

		ImGuiMCP::Separator();
		auto& state = SEDAS::State::Get();
		ImGuiMCP::Text("Last bed: %08X", state.lastBedRefID);
		ImGuiMCP::Text("Last bed cell: %08X", state.lastBedCellID);
		ImGuiMCP::Text("Bed save window: %s", SEDAS::Saving::IsInBedSaveWindow() ? "open" : "closed");
		RenderAppliedBonuses(SEDAS::State::Get().appliedBonuses);
	}

	void __stdcall RenderDragonAspect()
	{
		auto& config = SEDAS::Settings::Get().dragonAspect;

		ImGuiMCP::Checkbox("Enable on revive", &config.enabled);
		ImGuiMCP::Checkbox("Use vanilla Dragonborn spell lookup", &config.useVanillaDragonAspect);
		ImGuiMCP::SliderInt("Rank 1 souls", &config.rank1SoulThreshold, 0, 200);
		ImGuiMCP::SliderInt("Rank 2 souls", &config.rank2SoulThreshold, 0, 200);
		ImGuiMCP::SliderInt("Rank 3 souls", &config.rank3SoulThreshold, 0, 200);

		char rank1[256]{};
		char rank2[256]{};
		char rank3[256]{};
		std::strncpy(rank1, config.rank1SpellEditorID.c_str(), sizeof(rank1) - 1);
		std::strncpy(rank2, config.rank2SpellEditorID.c_str(), sizeof(rank2) - 1);
		std::strncpy(rank3, config.rank3SpellEditorID.c_str(), sizeof(rank3) - 1);

		if (ImGuiMCP::InputText("Rank 1 spell editor ID", rank1, sizeof(rank1))) {
			config.rank1SpellEditorID = rank1;
		}
		if (ImGuiMCP::InputText("Rank 2 spell editor ID", rank2, sizeof(rank2))) {
			config.rank2SpellEditorID = rank2;
		}
		if (ImGuiMCP::InputText("Rank 3 spell editor ID", rank3, sizeof(rank3))) {
			config.rank3SpellEditorID = rank3;
		}

		RenderSaveButton();
	}
#endif
}

namespace SEDAS::UI
{
	void Register()
	{
#if SEDAS_WITH_SKSE_MENU_FRAMEWORK
		if (!SKSEMenuFramework::IsInstalled()) {
			logger::warn("SKSE Menu Framework not installed; SEDAS menu pages disabled");
			return;
		}

		SKSEMenuFramework::SetSection("SEDAS");
		SKSEMenuFramework::AddSectionItem("Saving", RenderSaving);
		SKSEMenuFramework::AddSectionItem("Death Alternative", RenderDeath);
		SKSEMenuFramework::AddSectionItem("Soul Economy", RenderSoulEconomy);
		SKSEMenuFramework::AddSectionItem("Dragon Aspect", RenderDragonAspect);
		SKSEMenuFramework::AddSectionItem("Debugging", RenderDebugging);
		logger::info("Registered SKSE Menu Framework pages");
#else
		logger::info("SKSE Menu Framework header absent at build time; menu pages disabled");
#endif
	}
}
