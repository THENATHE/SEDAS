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
			SEDAS::SoulEconomy::Refresh();
			SEDAS::DeathAlternative::RefreshPlayerEssentialFlag();
		}
	}

	void __stdcall RenderSaving()
	{
		auto& config = SEDAS::Settings::Get().saving;
		ImGuiMCP::Checkbox("Save only at beds", &config.saveOnlyAtBeds);
		if (!config.saveOnlyAtBeds) {
			config.allowExitSave = false;
		}

		ImGuiMCP::BeginDisabled(!config.saveOnlyAtBeds);
		ImGuiMCP::Checkbox("Allow exit save", &config.allowExitSave);
		ImGuiMCP::SliderInt("Bed save window seconds", &config.bedSaveWindowSeconds, 0, 120);
		ImGuiMCP::Checkbox("Install experimental save hook", &config.installSaveHook);
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
		ImGuiMCP::SliderInt("Downed duration seconds", &config.downedDurationSeconds, 0, 30);

		ImGuiMCP::Separator();
		ImGuiMCP::Text("Last bed: %08X", state.lastBedRefID);
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
		ImGuiMCP::Text("Dragon souls: %d", SEDAS::SoulEconomy::GetDragonSoulCount());
		ImGuiMCP::Text("Effective iterations: %d", applied.iterations);
		RenderSaveButton();
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
		logger::info("Registered SKSE Menu Framework pages");
#else
		logger::info("SKSE Menu Framework header absent at build time; menu pages disabled");
#endif
	}
}
