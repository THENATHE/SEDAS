#pragma once

#include "SEDAS/PCH.h"

namespace SEDAS::Saving
{
	enum class SaveKind
	{
		kManual,
		kQuick,
		kAuto,
		kExit,
		kUnknown
	};

	void InstallHooks();
	void HandleBedSaveOpportunity();
	void OpenBedSaveWindow();
	void QueueBedAutoSave();
	bool IsInBedSaveWindow();
	bool ShouldAllowSave(SaveKind a_kind);
	std::string_view SaveKindName(SaveKind a_kind);
}
