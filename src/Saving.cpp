#include "SEDAS/Saving.h"

#include "SEDAS/Settings.h"

namespace
{
	std::atomic<std::int64_t> g_bedSaveWindowEndMs{ 0 };

	std::int64_t NowMs()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch())
			.count();
	}
}

namespace SEDAS::Saving
{
	void InstallHooks()
	{
		const auto& config = Settings::Get().saving;
		if (!config.saveOnlyAtBeds || !config.installSaveHook) {
			logger::info("Save hook not installed. SaveOnlyAtBeds={}, InstallSaveHook={}",
				config.saveOnlyAtBeds,
				config.installSaveHook);
			return;
		}

		logger::warn("Save hook requested but no verified latest-runtime trampoline is installed yet");
	}

	void OpenBedSaveWindow()
	{
		const auto seconds = std::max(0, Settings::Get().saving.bedSaveWindowSeconds);
		g_bedSaveWindowEndMs.store(NowMs() + static_cast<std::int64_t>(seconds) * 1000);
		logger::info("Opened bed save window for {} seconds", seconds);
	}

	bool IsInBedSaveWindow()
	{
		return NowMs() <= g_bedSaveWindowEndMs.load();
	}

	bool ShouldAllowSave(SaveKind a_kind)
	{
		const auto& config = Settings::Get().saving;
		if (!config.saveOnlyAtBeds) {
			return true;
		}

		if (a_kind == SaveKind::kExit && config.allowExitSave) {
			return true;
		}

		return IsInBedSaveWindow();
	}

	std::string_view SaveKindName(SaveKind a_kind)
	{
		switch (a_kind) {
		case SaveKind::kManual:
			return "manual";
		case SaveKind::kQuick:
			return "quick";
		case SaveKind::kAuto:
			return "auto";
		case SaveKind::kExit:
			return "exit";
		default:
			return "unknown";
		}
	}
}

