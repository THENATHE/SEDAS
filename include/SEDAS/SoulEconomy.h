#pragma once

#include "SEDAS/PCH.h"
#include "SEDAS/State.h"

namespace SEDAS::SoulEconomy
{
	int GetDragonSoulCount();
	bool ConsumeDragonSoul();
	State::AppliedBonuses BuildDesiredBonuses();
	void Refresh();
	void RemoveAppliedBonuses();
}

