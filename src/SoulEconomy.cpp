#include "SEDAS/SoulEconomy.h"

#include "SEDAS/Settings.h"

namespace
{
	constexpr auto kModifier = RE::ACTOR_VALUE_MODIFIERS::kTemporary;

	RE::ActorValueOwner* PlayerAV()
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		return player ? player->AsActorValueOwner() : nullptr;
	}

	void ModAV(RE::ActorValueOwner* a_av, RE::ActorValue a_actorValue, float a_delta)
	{
		if (!a_av || std::fabs(a_delta) < 0.001F) {
			return;
		}

		a_av->RestoreActorValue(kModifier, a_actorValue, a_delta);
	}

	void ApplyDelta(const SEDAS::State::AppliedBonuses& a_old, const SEDAS::State::AppliedBonuses& a_new)
	{
		auto av = PlayerAV();
		if (!av) {
			return;
		}

		ModAV(av, RE::ActorValue::kCarryWeight, a_new.carryWeight - a_old.carryWeight);
		ModAV(av, RE::ActorValue::kHealth, a_new.health - a_old.health);
		ModAV(av, RE::ActorValue::kStamina, a_new.stamina - a_old.stamina);
		ModAV(av, RE::ActorValue::kMagicka, a_new.magicka - a_old.magicka);

		const auto weaponDelta = a_new.weaponDamage - a_old.weaponDamage;
		ModAV(av, RE::ActorValue::kOneHandedModifier, weaponDelta);
		ModAV(av, RE::ActorValue::kTwoHandedModifier, weaponDelta);
		ModAV(av, RE::ActorValue::kMarksmanModifier, weaponDelta);

		const auto spellDelta = a_new.spellCostReduction - a_old.spellCostReduction;
		ModAV(av, RE::ActorValue::kAlterationModifier, spellDelta);
		ModAV(av, RE::ActorValue::kConjurationModifier, spellDelta);
		ModAV(av, RE::ActorValue::kDestructionModifier, spellDelta);
		ModAV(av, RE::ActorValue::kIllusionModifier, spellDelta);
		ModAV(av, RE::ActorValue::kRestorationModifier, spellDelta);
	}
}

namespace SEDAS::SoulEconomy
{
	int GetDragonSoulCount()
	{
		auto av = PlayerAV();
		if (!av) {
			return 0;
		}

		return std::max(0, static_cast<int>(std::floor(av->GetActorValue(RE::ActorValue::kDragonSouls) + 0.5F)));
	}

	bool ConsumeDragonSoul()
	{
		auto av = PlayerAV();
		if (!av || GetDragonSoulCount() <= 0) {
			return false;
		}

		av->ModActorValue(RE::ActorValue::kDragonSouls, -1.0F);
		Refresh();
		return true;
	}

	State::AppliedBonuses BuildDesiredBonuses()
	{
		const auto& config = Settings::Get().soulEconomy;
		State::AppliedBonuses desired;

		if (!config.enabled) {
			return desired;
		}

		desired.iterations = std::clamp(GetDragonSoulCount(), 0, std::max(0, config.soulCap));

		if (config.enableCarryWeight) {
			desired.carryWeight = config.carryWeightPerSoul * desired.iterations;
		}
		if (config.enableHealth) {
			desired.health = config.healthPerSoul * desired.iterations;
		}
		if (config.enableStamina) {
			desired.stamina = config.staminaPerSoul * desired.iterations;
		}
		if (config.enableMagicka) {
			desired.magicka = config.magickaPerSoul * desired.iterations;
		}
		if (config.enableWeaponDamage) {
			desired.weaponDamage = config.weaponDamagePercentPerSoul * desired.iterations;
		}
		if (config.enableSpellCostReduction) {
			desired.spellCostReduction = config.spellCostReductionPercentPerSoul * desired.iterations;
		}

		return desired;
	}

	void Refresh()
	{
		const auto desired = BuildDesiredBonuses();
		const auto old = State::Get().appliedBonuses;
		ApplyDelta(old, desired);
		State::SetAppliedBonuses(desired);
		logger::debug("Soul bonuses reconciled for {} effective souls", desired.iterations);
	}

	void RemoveAppliedBonuses()
	{
		const auto old = State::Get().appliedBonuses;
		ApplyDelta(old, {});
		State::SetAppliedBonuses({});
	}
}
