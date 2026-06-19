#pragma once

#include "SEDAS/PCH.h"

namespace SEDAS::DeathAlternative
{
	void Install();
	void RefreshPlayerEssentialFlag();
	void ClearRecoveryState();
	void ForceFixPlayerState();
	void TryStartRecovery(std::string_view a_reason);
	void FinishRecovery(bool a_recallToBed);
}
