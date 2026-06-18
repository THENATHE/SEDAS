#include "SEDAS/Serialization.h"

#include "SEDAS/State.h"

namespace
{
	constexpr std::uint32_t kUniqueID = 'SDAS';
	constexpr std::uint32_t kStateRecord = 'STAT';
	constexpr std::uint32_t kStateVersion = 1;

	struct SaveBlob
	{
		std::uint32_t version = kStateVersion;
		RE::FormID lastBedRefID = 0;
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

	void ApplyBlob(SKSE::SerializationInterface* a_intfc, const SaveBlob& a_blob)
	{
		auto& state = SEDAS::State::Get();
		state.lastBedRefID = 0;

		if (a_blob.lastBedRefID) {
			RE::FormID resolved = 0;
			if (a_intfc->ResolveFormID(a_blob.lastBedRefID, resolved)) {
				state.lastBedRefID = resolved;
			} else {
				logger::warn("Could not resolve last bed form {:08X}", a_blob.lastBedRefID);
			}
		}

		state.lastBedPosition = { a_blob.lastBedPosition[0], a_blob.lastBedPosition[1], a_blob.lastBedPosition[2] };
		state.lastBedAngle = { a_blob.lastBedAngle[0], a_blob.lastBedAngle[1], a_blob.lastBedAngle[2] };
		state.appliedBonuses = a_blob.appliedBonuses;
		state.haveOriginalPlayerEssential = a_blob.haveOriginalPlayerEssential;
		state.originalPlayerEssential = a_blob.originalPlayerEssential;
		state.candidateBedRefID = 0;
		state.recoveringFromDowned = false;
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

