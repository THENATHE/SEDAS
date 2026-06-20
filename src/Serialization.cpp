#include "SEDAS/Serialization.h"

#include "SEDAS/State.h"

namespace
{
	constexpr std::uint32_t kUniqueID = 'SDAS';
	constexpr std::uint32_t kStateRecord = 'STAT';
	constexpr std::uint32_t kStateVersion = 2;

	struct SaveBlobV1
	{
		std::uint32_t version = 1;
		RE::FormID lastBedRefID = 0;
		float lastBedPosition[3]{};
		float lastBedAngle[3]{};
		SEDAS::State::AppliedBonuses appliedBonuses;
		bool haveOriginalPlayerEssential = false;
		bool originalPlayerEssential = false;
	};

	struct SaveBlob
	{
		std::uint32_t version = kStateVersion;
		RE::FormID lastBedRefID = 0;
		RE::FormID lastBedCellID = 0;
		float lastBedPosition[3]{};
		float lastBedAngle[3]{};
		SEDAS::State::AppliedBonuses appliedBonuses;
		bool haveOriginalPlayerEssential = false;
		bool originalPlayerEssential = false;
	};

	SaveBlob MakeBlob()
	{
		const auto& state = SEDAS::State::Get();
		SaveBlob blob;
		blob.lastBedRefID = state.lastBedRefID;
		blob.lastBedCellID = state.lastBedCellID;
		blob.lastBedPosition[0] = state.lastBedPosition.x;
		blob.lastBedPosition[1] = state.lastBedPosition.y;
		blob.lastBedPosition[2] = state.lastBedPosition.z;
		blob.lastBedAngle[0] = state.lastBedAngle.x;
		blob.lastBedAngle[1] = state.lastBedAngle.y;
		blob.lastBedAngle[2] = state.lastBedAngle.z;
		blob.appliedBonuses = state.appliedBonuses;
		blob.haveOriginalPlayerEssential = state.haveOriginalPlayerEssential;
		blob.originalPlayerEssential = state.originalPlayerEssential;
		return blob;
	}

	void ApplyBlobData(
		SKSE::SerializationInterface* a_intfc,
		RE::FormID a_lastBedRefID,
		RE::FormID a_lastBedCellID,
		const float (&a_lastBedPosition)[3],
		const float (&a_lastBedAngle)[3],
		const SEDAS::State::AppliedBonuses& a_appliedBonuses,
		bool a_haveOriginalPlayerEssential,
		bool a_originalPlayerEssential)
	{
		auto& state = SEDAS::State::Get();
		state.lastBedRefID = 0;
		state.lastBedCellID = 0;

		if (a_lastBedRefID) {
			RE::FormID resolved = 0;
			if (a_intfc->ResolveFormID(a_lastBedRefID, resolved)) {
				state.lastBedRefID = resolved;
			} else {
				logger::warn("Could not resolve last bed form {:08X}", a_lastBedRefID);
			}
		}

		if (a_lastBedCellID) {
			RE::FormID resolved = 0;
			if (a_intfc->ResolveFormID(a_lastBedCellID, resolved)) {
				state.lastBedCellID = resolved;
			} else {
				logger::warn("Could not resolve last bed cell {:08X}", a_lastBedCellID);
			}
		}

		state.lastBedPosition = { a_lastBedPosition[0], a_lastBedPosition[1], a_lastBedPosition[2] };
		state.lastBedAngle = { a_lastBedAngle[0], a_lastBedAngle[1], a_lastBedAngle[2] };
		state.appliedBonuses = a_appliedBonuses;
		state.haveOriginalPlayerEssential = a_haveOriginalPlayerEssential;
		state.originalPlayerEssential = a_originalPlayerEssential;
		state.candidateBedRefID = 0;
		state.recoveringFromDowned = false;
		state.haveOriginalDownedFlags = false;
		state.originalMovementBlocked = false;
		state.originalInBleedoutAnimation = false;
		state.originalBlockPlayerInput = false;
		state.haveOriginalPlayerRuntimeFlags = false;
		state.originalPlayerRuntimeEssential = false;
		state.originalPlayerRuntimeProtected = false;
	}

	void ApplyBlob(SKSE::SerializationInterface* a_intfc, const SaveBlob& a_blob)
	{
		ApplyBlobData(
			a_intfc,
			a_blob.lastBedRefID,
			a_blob.lastBedCellID,
			a_blob.lastBedPosition,
			a_blob.lastBedAngle,
			a_blob.appliedBonuses,
			a_blob.haveOriginalPlayerEssential,
			a_blob.originalPlayerEssential);
	}

	void ApplyBlob(SKSE::SerializationInterface* a_intfc, const SaveBlobV1& a_blob)
	{
		ApplyBlobData(
			a_intfc,
			a_blob.lastBedRefID,
			0,
			a_blob.lastBedPosition,
			a_blob.lastBedAngle,
			a_blob.appliedBonuses,
			a_blob.haveOriginalPlayerEssential,
			a_blob.originalPlayerEssential);
	}

	void SaveCallback(SKSE::SerializationInterface* a_intfc)
	{
		const auto blob = MakeBlob();
		if (!a_intfc->OpenRecord(kStateRecord, kStateVersion)) {
			logger::error("Failed to open SEDAS serialization record");
			return;
		}

		if (!a_intfc->WriteRecordData(&blob, sizeof(blob))) {
			logger::error("Failed to write SEDAS serialization record");
		}
	}

	void LoadCallback(SKSE::SerializationInterface* a_intfc)
	{
		std::uint32_t type = 0;
		std::uint32_t version = 0;
		std::uint32_t length = 0;

		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			if (type != kStateRecord) {
				logger::warn("Unknown SEDAS serialization record {:08X}", type);
				continue;
			}

			if (version == 1 && length == sizeof(SaveBlobV1)) {
				SaveBlobV1 blob;
				if (!a_intfc->ReadRecordData(&blob, sizeof(blob))) {
					logger::error("Failed to read SEDAS v1 serialization record");
					continue;
				}

				ApplyBlob(a_intfc, blob);
				continue;
			}

			if (version != kStateVersion || length != sizeof(SaveBlob)) {
				logger::error("Unsupported SEDAS serialization record version={}, length={}", version, length);
				continue;
			}

			SaveBlob blob;
			if (!a_intfc->ReadRecordData(&blob, sizeof(blob))) {
				logger::error("Failed to read SEDAS serialization record");
				continue;
			}

			ApplyBlob(a_intfc, blob);
		}
	}

	void RevertCallback(SKSE::SerializationInterface*)
	{
		SEDAS::State::ResetForNewGame();
	}
}

namespace SEDAS::Serialization
{
	void Install()
	{
		auto serialization = SKSE::GetSerializationInterface();
		if (!serialization) {
			logger::critical("SKSE serialization interface unavailable");
			return;
		}

		serialization->SetUniqueID(kUniqueID);
		serialization->SetSaveCallback(SaveCallback);
		serialization->SetLoadCallback(LoadCallback);
		serialization->SetRevertCallback(RevertCallback);
	}
}
