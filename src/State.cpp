#include "SEDAS/State.h"

namespace
{
	std::mutex g_stateLock;
	SEDAS::State::RuntimeState g_state;
}

namespace SEDAS::State
{
	RuntimeState& Get()
	{
		return g_state;
	}

	void ResetForNewGame()
	{
		std::scoped_lock lock(g_stateLock);
		g_state = RuntimeState{};
	}

	void ClearTransient()
	{
		std::scoped_lock lock(g_stateLock);
		g_state.candidateBedRefID = 0;
		g_state.recoveringFromDowned = false;
	}

	void RecordCandidateBed(RE::TESObjectREFR* a_bed)
	{
		if (!a_bed) {
			return;
		}

		std::scoped_lock lock(g_stateLock);
		g_state.candidateBedRefID = a_bed->GetFormID();
	}

	bool CommitCandidateBed()
	{
		std::scoped_lock lock(g_stateLock);
		if (!g_state.candidateBedRefID) {
			return false;
		}

		auto bed = RE::TESForm::LookupByID<RE::TESObjectREFR>(g_state.candidateBedRefID);
		if (!bed) {
			g_state.candidateBedRefID = 0;
			return false;
		}

		g_state.lastBedRefID = bed->GetFormID();
		g_state.lastBedPosition = bed->GetPosition();
		g_state.lastBedAngle = bed->GetAngle();
		g_state.candidateBedRefID = 0;
		logger::info("Recorded last slept bed {:08X}", g_state.lastBedRefID);
		return true;
	}

	RE::TESObjectREFR* ResolveLastBed()
	{
		std::scoped_lock lock(g_stateLock);
		if (!g_state.lastBedRefID) {
			return nullptr;
		}

		return RE::TESForm::LookupByID<RE::TESObjectREFR>(g_state.lastBedRefID);
	}

	void SetAppliedBonuses(const AppliedBonuses& a_bonuses)
	{
		std::scoped_lock lock(g_stateLock);
		g_state.appliedBonuses = a_bonuses;
	}
}

