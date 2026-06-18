#include "SEDAS/DeathAlternative.h"

#include "SEDAS/Settings.h"
#include "SEDAS/SoulEconomy.h"
#include "SEDAS/State.h"

namespace
{
	std::atomic_bool g_recoveryScheduled{ false };
	std::atomic_int g_soulsAtRecoveryStart{ 0 };

	constexpr std::array<std::string_view, 8> kRank1DragonAspectSpellEditorIDs{
		"DLC2DragonAspectPower01",
		"DLC2DragonAspectPower1",
		"DLC2DragonAspectSpell01",
		"DLC2DragonAspectSpell1",
		"DLC2DragonAspect01",
		"DLC2DragonAspect1",
		"DLC2DragonAspectPower",
		"DLC2DragonAspect"
	};

	constexpr std::array<std::string_view, 8> kRank2DragonAspectSpellEditorIDs{
		"DLC2DragonAspectPower02",
		"DLC2DragonAspectPower2",
		"DLC2DragonAspectSpell02",
		"DLC2DragonAspectSpell2",
		"DLC2DragonAspect02",
		"DLC2DragonAspect2",
		"DLC2DragonAspectPower",
		"DLC2DragonAspect"
	};

	constexpr std::array<std::string_view, 8> kRank3DragonAspectSpellEditorIDs{
		"DLC2DragonAspectPower03",
		"DLC2DragonAspectPower3",
		"DLC2DragonAspectSpell03",
		"DLC2DragonAspectSpell3",
		"DLC2DragonAspect03",
		"DLC2DragonAspect3",
		"DLC2DragonAspectPower",
		"DLC2DragonAspect"
	};

	RE::PlayerCharacter* Player()
	{
		return RE::PlayerCharacter::GetSingleton();
	}

	void RestoreHealthToFull(RE::PlayerCharacter* a_player)
	{
		if (!a_player) {
			return;
		}

		auto av = a_player->AsActorValueOwner();
		if (!av) {
			return;
		}

		av->RestoreActorValue(RE::ACTOR_VALUE_MODIFIERS::kDamage, RE::ActorValue::kHealth, 1000000.0F);
	}

	void SetPlayerInputBlocked(bool a_blocked)
	{
		if (auto controls = RE::PlayerControls::GetSingleton()) {
			controls->blockPlayerInput = a_blocked;
		}
	}

	int DragonAspectRankForSouls(int a_souls)
	{
		const auto& config = SEDAS::Settings::Get().dragonAspect;
		if (!config.enabled) {
			return 0;
		}

		if (a_souls >= config.rank3SoulThreshold) {
			return 3;
		}
		if (a_souls >= config.rank2SoulThreshold) {
			return 2;
		}
		if (a_souls >= config.rank1SoulThreshold) {
			return 1;
		}
		return 0;
	}

	std::string_view SpellOverrideForRank(int a_rank)
	{
		const auto& config = SEDAS::Settings::Get().dragonAspect;
		switch (a_rank) {
		case 1:
			return config.rank1SpellEditorID;
		case 2:
			return config.rank2SpellEditorID;
		case 3:
			return config.rank3SpellEditorID;
		default:
			return {};
		}
	}

	std::span<const std::string_view> CandidateEditorIDsForRank(int a_rank)
	{
		switch (a_rank) {
		case 1:
			return kRank1DragonAspectSpellEditorIDs;
		case 2:
			return kRank2DragonAspectSpellEditorIDs;
		case 3:
			return kRank3DragonAspectSpellEditorIDs;
		default:
			return {};
		}
	}

	RE::SpellItem* LookupSpellByEditorID(std::string_view a_editorID)
	{
		if (a_editorID.empty()) {
			return nullptr;
		}

		return RE::TESForm::LookupByEditorID<RE::SpellItem>(a_editorID);
	}

	RE::SpellItem* ResolveDragonAspectSpell(int a_rank)
	{
		if (auto overrideID = SpellOverrideForRank(a_rank); !overrideID.empty()) {
			auto spell = LookupSpellByEditorID(overrideID);
			if (spell) {
				logger::info("Resolved Dragon Aspect rank {} override spell {}", a_rank, overrideID);
				return spell;
			}
			logger::warn("Configured Dragon Aspect rank {} spell editor ID did not resolve: {}", a_rank, overrideID);
		}

		if (!SEDAS::Settings::Get().dragonAspect.useVanillaDragonAspect) {
			return nullptr;
		}

		for (const auto candidate : CandidateEditorIDsForRank(a_rank)) {
			if (auto spell = LookupSpellByEditorID(candidate)) {
				logger::info("Resolved vanilla Dragon Aspect rank {} spell {}", a_rank, candidate);
				return spell;
			}
		}

		return nullptr;
	}

	void ApplyDragonAspectIfConfigured(RE::PlayerCharacter* a_player)
	{
		const auto rank = DragonAspectRankForSouls(g_soulsAtRecoveryStart.load());
		if (rank == 0) {
			return;
		}

		auto spell = ResolveDragonAspectSpell(rank);
		if (!spell) {
			logger::warn("Could not resolve a DLL-only Dragon Aspect spell for rank {}", rank);
			return;
		}

		auto caster = a_player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
		if (!caster) {
			logger::warn("Could not get instant caster for Dragon Aspect");
			return;
		}

		caster->CastSpellImmediate(spell, false, a_player, 1.0F, false, 0.0F, a_player);
		logger::info("Applied Dragon Aspect rank {}", rank);
	}

	void ScheduleFinish(bool a_recallToBed)
	{
		const auto delay = std::max(0, SEDAS::Settings::Get().death.downedDurationSeconds);
		std::thread([delay, a_recallToBed]() {
			std::this_thread::sleep_for(std::chrono::seconds(delay));
			auto task = SKSE::GetTaskInterface();
			if (task) {
				task->AddTask([a_recallToBed]() {
					SEDAS::DeathAlternative::FinishRecovery(a_recallToBed);
				});
			}
		}).detach();
	}
}

namespace SEDAS::DeathAlternative
{
	void Install()
	{
		RefreshPlayerEssentialFlag();
	}

	void RefreshPlayerEssentialFlag()
	{
		auto player = Player();
		if (!player) {
			return;
		}

		auto base = player->GetActorBase();
		if (!base) {
			return;
		}

		auto& state = State::Get();
		if (!state.haveOriginalPlayerEssential) {
			state.originalPlayerEssential = base->actorData.actorBaseFlags.all(RE::ACTOR_BASE_DATA::Flag::kEssential);
			state.haveOriginalPlayerEssential = true;
		}

		auto& flags = base->actorData.actorBaseFlags;
		if (Settings::Get().death.enabled) {
			flags.set(RE::ACTOR_BASE_DATA::Flag::kEssential);
		} else if (!state.originalPlayerEssential) {
			flags.reset(RE::ACTOR_BASE_DATA::Flag::kEssential);
		}
	}

	void TryStartRecovery(std::string_view a_reason)
	{
		const auto& config = Settings::Get().death;
		if (!config.enabled) {
			return;
		}

		if (g_recoveryScheduled.exchange(true)) {
			return;
		}

		auto player = Player();
		if (!player) {
			g_recoveryScheduled.store(false);
			return;
		}

		auto& state = State::Get();
		state.recoveringFromDowned = true;
		g_soulsAtRecoveryStart.store(SoulEconomy::GetDragonSoulCount());

		if (config.disableControlsWhileDowned) {
			SetPlayerInputBlocked(true);
		}

		bool recallToBed = false;
		if (config.consumeDragonSoulToRevive && g_soulsAtRecoveryStart.load() > 0) {
			SoulEconomy::ConsumeDragonSoul();
			logger::info("Downed recovery started from {}; consumed one dragon soul", a_reason);
		} else {
			recallToBed = config.recallToLastBedWhenNoSouls;
			logger::info("Downed recovery started from {}; no dragon soul consumed", a_reason);
		}

		ScheduleFinish(recallToBed);
	}

	void FinishRecovery(bool a_recallToBed)
	{
		auto player = Player();
		if (!player) {
			g_recoveryScheduled.store(false);
			return;
		}

		if (player->IsDead(false)) {
			player->Resurrect(false, true);
		}

		player->SetLifeState(RE::ACTOR_LIFE_STATE::kAlive);

		if (a_recallToBed) {
			if (auto bed = State::ResolveLastBed()) {
				player->MoveTo(bed);
				logger::info("Recalled player to last slept bed");
			} else {
				logger::warn("Recall requested, but last slept bed could not be resolved; reviving in place");
			}
		}

		RestoreHealthToFull(player);
		ApplyDragonAspectIfConfigured(player);

		if (Settings::Get().death.disableControlsWhileDowned) {
			SetPlayerInputBlocked(false);
		}

		State::Get().recoveringFromDowned = false;
		g_recoveryScheduled.store(false);
		SoulEconomy::Refresh();
	}
}
