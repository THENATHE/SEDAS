#include "SEDAS/DeathAlternative.h"

#include "SEDAS/Settings.h"
#include "SEDAS/SoulEconomy.h"
#include "SEDAS/State.h"

namespace
{
	std::atomic_bool g_recoveryScheduled{ false };
	std::atomic_bool g_consumedSoulForRecovery{ false };
	std::atomic_int g_soulsAtRecoveryStart{ 0 };
	std::atomic<std::uint64_t> g_recoveryGeneration{ 0 };

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

	constexpr std::array<std::string_view, 8> kSoulAbsorbEffectShaderEditorIDs{
		"DragonSoulAbsorbEffectShader",
		"DragonSoulAbsorbFXS",
		"FXDragonSoulAbsorbEffectShader",
		"FXDragonAbsorbEffectShader",
		"DragonAbsorbEffectShader",
		"DragonAbsorbFXS",
		"MQKillDragonSoulAbsorbFXS",
		"MQKillDragonFXS"
	};

	constexpr std::array<std::string_view, 8> kSoulAbsorbArtObjectEditorIDs{
		"DragonSoulAbsorbArt",
		"DragonSoulAbsorbEffect",
		"FXDragonSoulAbsorbArt",
		"FXDragonSoulAbsorbEffect",
		"FXDragonAbsorbArt",
		"FXDragonAbsorbEffect",
		"MQKillDragonSoulAbsorbArt",
		"MQKillDragonArt"
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
			auto& state = SEDAS::State::Get();
			if (a_blocked && !state.haveOriginalDownedFlags) {
				state.originalBlockPlayerInput = controls->blockPlayerInput;
			}

			controls->blockPlayerInput = a_blocked;
			controls->data.moveInputVec = RE::NiPoint2{ 0.0F, 0.0F };
			controls->data.prevMoveVec = RE::NiPoint2{ 0.0F, 0.0F };
			controls->data.autoMove = false;
		}
	}

	void RestorePlayerInputBlocked()
	{
		auto controls = RE::PlayerControls::GetSingleton();
		if (!controls) {
			return;
		}

		auto& state = SEDAS::State::Get();
		controls->blockPlayerInput = state.haveOriginalDownedFlags ? state.originalBlockPlayerInput : false;
		controls->data.moveInputVec = RE::NiPoint2{ 0.0F, 0.0F };
		controls->data.prevMoveVec = RE::NiPoint2{ 0.0F, 0.0F };
		controls->data.autoMove = false;
	}

	void SetActorFlag(RE::PlayerCharacter* a_player, RE::Actor::BOOL_FLAGS a_flag, bool a_enabled)
	{
		if (!a_player) {
			return;
		}

		auto& flags = a_player->GetActorRuntimeData().boolFlags;
		if (a_enabled) {
			flags.set(a_flag);
		} else {
			flags.reset(a_flag);
		}
	}

	void RestoreActorFlag(RE::PlayerCharacter* a_player, RE::Actor::BOOL_FLAGS a_flag, bool a_enabled)
	{
		SetActorFlag(a_player, a_flag, a_enabled);
	}

	void ClearDeathBits(RE::PlayerCharacter* a_player)
	{
		if (!a_player) {
			return;
		}

		auto& bits = a_player->GetActorRuntimeData().boolBits;
		bits.reset(RE::Actor::BOOL_BITS::kDead);
		bits.reset(RE::Actor::BOOL_BITS::kSetOnDeath);
	}

	void ExitBleedoutCamera(RE::PlayerCharacter* a_player)
	{
		auto camera = RE::PlayerCamera::GetSingleton();
		if (!camera) {
			return;
		}

		camera->ForceThirdPerson();
		const auto weaponDrawn = a_player && a_player->AsActorState() && a_player->AsActorState()->IsWeaponDrawn();
		camera->UpdateThirdPerson(weaponDrawn);
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

	template <class T>
	T* LookupFormByEditorID(std::string_view a_editorID)
	{
		if (a_editorID.empty()) {
			return nullptr;
		}

		return RE::TESForm::LookupByEditorID<T>(a_editorID);
	}

	RE::SpellItem* LookupSpellByEditorID(std::string_view a_editorID)
	{
		return LookupFormByEditorID<RE::SpellItem>(a_editorID);
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

	template <class T>
	T* ResolveConfiguredOrVanillaForm(
		std::string_view a_configuredEditorID,
		std::span<const std::string_view> a_candidateEditorIDs,
		std::string_view a_logName)
	{
		if (!a_configuredEditorID.empty()) {
			auto form = LookupFormByEditorID<T>(a_configuredEditorID);
			if (form) {
				logger::info("Resolved configured {} {}", a_logName, a_configuredEditorID);
				return form;
			}
			logger::warn("Configured {} editor ID did not resolve: {}", a_logName, a_configuredEditorID);
		}

		for (const auto candidate : a_candidateEditorIDs) {
			if (auto form = LookupFormByEditorID<T>(candidate)) {
				logger::info("Resolved vanilla {} {}", a_logName, candidate);
				return form;
			}
		}

		logger::warn("Could not resolve a vanilla {}", a_logName);
		return nullptr;
	}

	void PlayDragonSoulAbsorbEffect(RE::PlayerCharacter* a_player)
	{
		const auto& config = SEDAS::Settings::Get().death;
		if (!a_player || !config.playDragonSoulAbsorbEffect) {
			return;
		}

		bool played = false;
		if (auto shader = ResolveConfiguredOrVanillaForm<RE::TESEffectShader>(
				config.soulAbsorbEffectShaderEditorID,
				kSoulAbsorbEffectShaderEditorIDs,
				"dragon soul absorb effect shader")) {
			a_player->ApplyEffectShader(shader, 3.0F, nullptr, false, false, nullptr, false);
			played = true;
		}

		if (auto artObject = ResolveConfiguredOrVanillaForm<RE::BGSArtObject>(
				config.soulAbsorbArtObjectEditorID,
				kSoulAbsorbArtObjectEditorIDs,
				"dragon soul absorb art object")) {
			a_player->ApplyArtObject(artObject, 3.0F, nullptr, false, false, nullptr, false);
			played = true;
		}

		if (played) {
			logger::info("Played dragon soul absorb revive effect");
		}
	}

	void CaptureOriginalDownedFlags(RE::PlayerCharacter* a_player)
	{
		if (!a_player) {
			return;
		}

		auto& state = SEDAS::State::Get();
		if (state.haveOriginalDownedFlags) {
			return;
		}

		const auto& flags = a_player->GetActorRuntimeData().boolFlags;
		state.originalMovementBlocked = flags.all(RE::Actor::BOOL_FLAGS::kMovementBlocked);
		state.originalInBleedoutAnimation = flags.all(RE::Actor::BOOL_FLAGS::kInBleedoutAnimation);
		if (auto controls = RE::PlayerControls::GetSingleton()) {
			state.originalBlockPlayerInput = controls->blockPlayerInput;
		} else {
			state.originalBlockPlayerInput = false;
		}
		state.haveOriginalDownedFlags = true;
	}

	void StabilizeAliveState(RE::PlayerCharacter* a_player)
	{
		if (!a_player) {
			return;
		}

		if (a_player->IsDead(false)) {
			a_player->Resurrect(false, true);
		}

		ClearDeathBits(a_player);
		a_player->SetLifeState(RE::ACTOR_LIFE_STATE::kAlive);
		RestoreHealthToFull(a_player);
	}

	void BeginDownedState(RE::PlayerCharacter* a_player)
	{
		if (!a_player) {
			return;
		}

		const auto& config = SEDAS::Settings::Get().death;
		CaptureOriginalDownedFlags(a_player);
		StabilizeAliveState(a_player);
		a_player->StopMoving(0.0F);
		a_player->StopInteractingQuick(true);

		SetActorFlag(a_player, RE::Actor::BOOL_FLAGS::kMovementBlocked, true);

		if (config.disableControlsWhileDowned) {
			SetPlayerInputBlocked(true);
		}

		if (config.ragdollInsteadOfBleedout) {
			SetActorFlag(a_player, RE::Actor::BOOL_FLAGS::kInBleedoutAnimation, false);
			a_player->SetLifeState(RE::ACTOR_LIFE_STATE::kUnconcious);
			a_player->SetMotionType(RE::TESObjectREFR::MotionType::kDynamic, true);
		} else {
			SetActorFlag(a_player, RE::Actor::BOOL_FLAGS::kInBleedoutAnimation, true);
			a_player->SetLifeState(RE::ACTOR_LIFE_STATE::kEssentialDown);
		}

		ExitBleedoutCamera(a_player);
	}

	void EndDownedState(RE::PlayerCharacter* a_player)
	{
		if (!a_player) {
			return;
		}

		auto& state = SEDAS::State::Get();
		StabilizeAliveState(a_player);

		if (a_player->IsInRagdollState()) {
			a_player->SetMotionType(RE::TESObjectREFR::MotionType::kCharacter, true);
			a_player->PotentiallyFixRagdollState();
		}

		if (state.haveOriginalDownedFlags) {
			RestoreActorFlag(a_player, RE::Actor::BOOL_FLAGS::kMovementBlocked, state.originalMovementBlocked);
			RestoreActorFlag(a_player, RE::Actor::BOOL_FLAGS::kInBleedoutAnimation, state.originalInBleedoutAnimation);
		} else {
			RestoreActorFlag(a_player, RE::Actor::BOOL_FLAGS::kMovementBlocked, false);
			RestoreActorFlag(a_player, RE::Actor::BOOL_FLAGS::kInBleedoutAnimation, false);
		}

		RestorePlayerInputBlocked();
		a_player->StopMoving(0.0F);
		a_player->StopInteractingQuick(true);
		ExitBleedoutCamera(a_player);
		a_player->InitiateGetUpPackage();
		a_player->EvaluatePackage(true, true);
	}

	void PostRecoveryCleanup()
	{
		auto player = Player();
		if (!player) {
			return;
		}

		StabilizeAliveState(player);
		auto& state = SEDAS::State::Get();
		if (state.haveOriginalDownedFlags) {
			RestoreActorFlag(player, RE::Actor::BOOL_FLAGS::kMovementBlocked, state.originalMovementBlocked);
			RestoreActorFlag(player, RE::Actor::BOOL_FLAGS::kInBleedoutAnimation, state.originalInBleedoutAnimation);
		} else {
			SetActorFlag(player, RE::Actor::BOOL_FLAGS::kMovementBlocked, false);
			SetActorFlag(player, RE::Actor::BOOL_FLAGS::kInBleedoutAnimation, false);
		}
		RestorePlayerInputBlocked();

		if (player->IsInRagdollState()) {
			player->SetMotionType(RE::TESObjectREFR::MotionType::kCharacter, true);
			player->PotentiallyFixRagdollState();
		}

		ExitBleedoutCamera(player);
		player->InitiateGetUpPackage();
		player->EvaluatePackage(true, true);
		state.haveOriginalDownedFlags = false;
	}

	void SchedulePostRecoveryCleanup(std::uint64_t a_generation)
	{
		std::thread([a_generation]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(250));
			if (g_recoveryGeneration.load() != a_generation) {
				return;
			}

			if (auto task = SKSE::GetTaskInterface()) {
				task->AddTask([a_generation]() {
					if (g_recoveryGeneration.load() == a_generation) {
						PostRecoveryCleanup();
					}
				});
			}
		}).detach();
	}

	bool RecallPlayerToLastBed(RE::PlayerCharacter* a_player)
	{
		if (!a_player) {
			return false;
		}

		if (auto bed = SEDAS::State::ResolveLastBed()) {
			a_player->MoveTo(bed);
			logger::info("Recalled player to last slept bed {:08X}", bed->GetFormID());
			return true;
		}

		logger::warn("Recall requested, but last slept bed could not be resolved; reviving in place");
		return false;
	}

	void ScheduleFinish(bool a_recallToBed, std::uint64_t a_generation)
	{
		const auto delay = std::max(0, SEDAS::Settings::Get().death.downedDurationSeconds);
		std::thread([delay, a_recallToBed, a_generation]() {
			std::this_thread::sleep_for(std::chrono::seconds(delay));
			if (g_recoveryGeneration.load() != a_generation) {
				return;
			}

			auto task = SKSE::GetTaskInterface();
			if (task) {
				task->AddTask([a_recallToBed, a_generation]() {
					if (g_recoveryGeneration.load() == a_generation) {
						SEDAS::DeathAlternative::FinishRecovery(a_recallToBed);
					}
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

	void ClearRecoveryState()
	{
		g_recoveryGeneration.fetch_add(1);
		g_recoveryScheduled.store(false);
		g_consumedSoulForRecovery.store(false);
		g_soulsAtRecoveryStart.store(0);

		auto& state = State::Get();
		state.recoveringFromDowned = false;
		state.haveOriginalDownedFlags = false;
		state.originalMovementBlocked = false;
		state.originalInBleedoutAnimation = false;
		state.originalBlockPlayerInput = false;

		auto player = Player();
		if (!player) {
			SetPlayerInputBlocked(false);
			return;
		}

		SetActorFlag(player, RE::Actor::BOOL_FLAGS::kMovementBlocked, false);
		SetActorFlag(player, RE::Actor::BOOL_FLAGS::kInBleedoutAnimation, false);
		SetPlayerInputBlocked(false);
	}

	void ForceFixPlayerState()
	{
		ClearRecoveryState();

		auto player = Player();
		if (!player) {
			logger::warn("Force death-state cleanup requested, but player singleton is unavailable");
			return;
		}

		StabilizeAliveState(player);
		if (player->IsInRagdollState()) {
			player->SetMotionType(RE::TESObjectREFR::MotionType::kCharacter, true);
			player->PotentiallyFixRagdollState();
		}

		SetActorFlag(player, RE::Actor::BOOL_FLAGS::kMovementBlocked, false);
		SetActorFlag(player, RE::Actor::BOOL_FLAGS::kInBleedoutAnimation, false);
		SetPlayerInputBlocked(false);
		player->StopMoving(0.0F);
		player->StopInteractingQuick(true);
		ExitBleedoutCamera(player);
		player->InitiateGetUpPackage();
		player->EvaluatePackage(true, true);
		SoulEconomy::Refresh();
		logger::info("Forced player death-state cleanup");
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

		auto& runtimeFlags = player->GetActorRuntimeData().boolFlags;
		if (!state.haveOriginalPlayerRuntimeFlags) {
			state.originalPlayerRuntimeEssential = runtimeFlags.all(RE::Actor::BOOL_FLAGS::kEssential);
			state.originalPlayerRuntimeProtected = runtimeFlags.all(RE::Actor::BOOL_FLAGS::kProtected);
			state.haveOriginalPlayerRuntimeFlags = true;
		}

		if (Settings::Get().death.enabled) {
			runtimeFlags.set(RE::Actor::BOOL_FLAGS::kEssential);
			runtimeFlags.set(RE::Actor::BOOL_FLAGS::kProtected);
		} else {
			if (!state.originalPlayerRuntimeEssential) {
				runtimeFlags.reset(RE::Actor::BOOL_FLAGS::kEssential);
			}
			if (!state.originalPlayerRuntimeProtected) {
				runtimeFlags.reset(RE::Actor::BOOL_FLAGS::kProtected);
			}
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
		const auto generation = g_recoveryGeneration.fetch_add(1) + 1;
		g_soulsAtRecoveryStart.store(SoulEconomy::GetDragonSoulCount());
		g_consumedSoulForRecovery.store(false);
		BeginDownedState(player);

		bool recallToBed = false;
		if (config.consumeDragonSoulToRevive && g_soulsAtRecoveryStart.load() > 0) {
			SoulEconomy::ConsumeDragonSoul();
			g_consumedSoulForRecovery.store(true);
			logger::info("Downed recovery started from {}; consumed one dragon soul", a_reason);
		} else {
			recallToBed = config.recallToLastBedWhenNoSouls;
			logger::info("Downed recovery started from {}; no dragon soul consumed", a_reason);
		}

		ScheduleFinish(recallToBed, generation);
	}

	void FinishRecovery(bool a_recallToBed)
	{
		auto player = Player();
		if (!player) {
			State::Get().recoveringFromDowned = false;
			g_consumedSoulForRecovery.store(false);
			g_recoveryScheduled.store(false);
			return;
		}

		EndDownedState(player);

		if (a_recallToBed) {
			RecallPlayerToLastBed(player);
		}

		RestoreHealthToFull(player);
		if (g_consumedSoulForRecovery.load()) {
			PlayDragonSoulAbsorbEffect(player);
		}
		ApplyDragonAspectIfConfigured(player);

		State::Get().recoveringFromDowned = false;
		g_consumedSoulForRecovery.store(false);
		g_recoveryScheduled.store(false);
		SoulEconomy::Refresh();
		SchedulePostRecoveryCleanup(g_recoveryGeneration.load());
	}
}
