#pragma once

#include "SEDAS/PCH.h"

namespace SEDAS::State
{
	struct AppliedBonuses
	{
		float carryWeight = 0.0F;
		float health = 0.0F;
		float stamina = 0.0F;
		float magicka = 0.0F;
		float weaponDamage = 0.0F;
		float spellCostReduction = 0.0F;
		int iterations = 0;
	};

	struct RuntimeState
	{
		RE::FormID candidateBedRefID = 0;
		RE::FormID lastBedRefID = 0;
		RE::NiPoint3 lastBedPosition{ 0.0F, 0.0F, 0.0F };
		RE::NiPoint3 lastBedAngle{ 0.0F, 0.0F, 0.0F };

		AppliedBonuses appliedBonuses;

		bool haveOriginalPlayerEssential = false;
		bool originalPlayerEssential = false;
		bool haveOriginalPlayerRuntimeFlags = false;
		bool originalPlayerRuntimeEssential = false;
		bool originalPlayerRuntimeProtected = false;
		bool haveOriginalDownedFlags = false;
		bool originalMovementBlocked = false;
		bool originalInBleedoutAnimation = false;
		bool originalBlockPlayerInput = false;
		bool recoveringFromDowned = false;
	};

	RuntimeState& Get();
	void ResetForNewGame();
	void ClearTransient();

	void RecordCandidateBed(RE::TESObjectREFR* a_bed);
	bool CommitCandidateBed();
	RE::TESObjectREFR* ResolveLastBed();

	void SetAppliedBonuses(const AppliedBonuses& a_bonuses);
}
