#pragma once

#include "SEDAS/PCH.h"

namespace SEDAS::Plugin
{
	inline constexpr auto Name = "SEDAS";
	inline constexpr auto PrettyName = "Survival Saving, Soul Economy, and Death Alternative System";
	inline constexpr std::uint32_t VersionMajor = 0;
	inline constexpr std::uint32_t VersionMinor = 1;
	inline constexpr std::uint32_t VersionPatch = 0;

	void SetupLog();
	void OnDataLoaded();
	void OnNewGameOrLoad();
}

