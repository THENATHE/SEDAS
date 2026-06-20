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
		g_state.haveOriginalDownedFlags = false;
		g_state.originalMovementBlocked = false;
		g_state.originalInBleedoutAnimation = false;
		g_state.originalBlockPlayerInput = false;
	}

	bool RecordCandidateBed(RE::TESObjectREFR* a_bed)
	{
		if (!a_bed) {
			return false;
		}

		std::scoped_lock lock(g_stateLock);
		g_state.candidateBedRefID = a_bed->GetFormID();
		logger::info("Recorded candidate bed {:08X}", g_state.candidateBedRefID);
		return true;
	}

	bool CommitBed(RE::TESObjectREFR* a_bed)
	{
		if (!a_bed) {
			return false;
		}

		std::scoped_lock lock(g_stateLock);
		g_state.lastBedRefID = a_bed->GetFormID();
		if (auto cell = a_bed->GetParentCell()) {
			g_state.lastBedCellID = cell->GetFormID();
		} else {
			g_state.lastBedCellID = 0;
		}
		g_state.lastBedPosition = a_bed->GetPosition();
		g_state.lastBedAngle = a_bed->GetAngle();
		g_state.candidateBedRefID = 0;
		logger::info("Recorded last slept bed {:08X} in cell {:08X}", g_state.lastBedRefID, g_state.lastBedCellID);
		return true;
	}

	bool CommitCandidateBed()
	{
		RE::FormID candidate = 0;
		{
			std::scoped_lock lock(g_stateLock);
			candidate = g_state.candidateBedRefID;
		}

		if (!candidate) {
			return false;
		}

		auto bed = RE::TESForm::LookupByID<RE::TESObjectREFR>(candidate);
		if (!bed) {
			std::scoped_lock lock(g_stateLock);
			g_state.candidateBedRefID = 0;
			logger::warn("Candidate bed {:08X} no longer resolves", candidate);
			return false;
		}

		return CommitBed(bed);
	}

	bool HasCandidateBed()
	{
		std::scoped_lock lock(g_stateLock);
		return g_state.candidateBedRefID != 0;
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
