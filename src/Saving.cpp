#include "SEDAS/Saving.h"

#include "SEDAS/Settings.h"

namespace
{
	constexpr auto kBedAutoSaveName = "Autosave SEDAS";

	std::atomic<std::int64_t> g_bedSaveWindowEndMs{ 0 };
	std::atomic_bool g_bedAutoSaveQueued{ false };
	std::atomic_bool g_hookInstalled{ false };
	std::atomic_bool g_haveOriginalSavingDisabled{ false };
	std::atomic_bool g_internalSaveAllowed{ false };
	std::atomic_bool g_originalSavingDisabled{ false };
	std::mutex g_saveHookLock;

	using SaveImpl_t = bool (*)(RE::BGSSaveLoadManager*, std::int32_t, std::uint32_t, const char*);
	std::uintptr_t g_saveTarget = 0;
	std::array<std::uint8_t, 5> g_originalSaveBytes{};

	std::int64_t NowMs()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch())
			.count();
	}

	std::string LowerFileName(const char* a_fileName)
	{
		if (!a_fileName) {
			return {};
		}

		std::string fileName{ a_fileName };
		std::transform(fileName.begin(), fileName.end(), fileName.begin(), [](unsigned char a_ch) {
			return static_cast<char>(std::tolower(a_ch));
		});
		return fileName;
	}

	SEDAS::Saving::SaveKind DeduceSaveKind(const char* a_fileName)
	{
		const auto fileName = LowerFileName(a_fileName);
		if (fileName.empty()) {
			return SEDAS::Saving::SaveKind::kUnknown;
		}

		if (fileName.find("exitsave") != std::string::npos || fileName.find("exit save") != std::string::npos) {
			return SEDAS::Saving::SaveKind::kExit;
		}
		if (fileName.find("quicksave") != std::string::npos || fileName.find("quick save") != std::string::npos) {
			return SEDAS::Saving::SaveKind::kQuick;
		}
		if (fileName.find("autosave") != std::string::npos || fileName.find("auto save") != std::string::npos) {
			return SEDAS::Saving::SaveKind::kAuto;
		}

		return SEDAS::Saving::SaveKind::kManual;
	}

	bool SaveHook(RE::BGSSaveLoadManager* a_manager, std::int32_t a_deviceID, std::uint32_t a_outputStats, const char* a_fileName);

	void PatchSaveHook()
	{
		if (!g_saveTarget) {
			return;
		}

		SKSE::GetTrampoline().write_branch<5>(g_saveTarget, SaveHook);
	}

	void RestoreOriginalSave()
	{
		if (!g_saveTarget) {
			return;
		}

		REL::safe_write(g_saveTarget, g_originalSaveBytes.data(), g_originalSaveBytes.size());
	}

	class SaveHookPatchGuard
	{
	public:
		SaveHookPatchGuard()
		{
			RestoreOriginalSave();
		}

		~SaveHookPatchGuard()
		{
			PatchSaveHook();
		}

		SaveHookPatchGuard(const SaveHookPatchGuard&) = delete;
		SaveHookPatchGuard(SaveHookPatchGuard&&) = delete;
		SaveHookPatchGuard& operator=(const SaveHookPatchGuard&) = delete;
		SaveHookPatchGuard& operator=(SaveHookPatchGuard&&) = delete;
	};

	class InternalSaveGuard
	{
	public:
		InternalSaveGuard() :
			_wasAllowed(g_internalSaveAllowed.exchange(true))
		{}

		~InternalSaveGuard()
		{
			g_internalSaveAllowed.store(_wasAllowed);
			SEDAS::Saving::RefreshSaveDisabledFlag();
		}

		InternalSaveGuard(const InternalSaveGuard&) = delete;
		InternalSaveGuard(InternalSaveGuard&&) = delete;
		InternalSaveGuard& operator=(const InternalSaveGuard&) = delete;
		InternalSaveGuard& operator=(InternalSaveGuard&&) = delete;

	private:
		bool _wasAllowed;
	};

	bool SaveHook(RE::BGSSaveLoadManager* a_manager, std::int32_t a_deviceID, std::uint32_t a_outputStats, const char* a_fileName)
	{
		const auto& config = SEDAS::Settings::Get().saving;
		const auto kind = DeduceSaveKind(a_fileName);
		const auto saveName = a_fileName ? a_fileName : "<null>";

		if (config.installSaveHook && !g_internalSaveAllowed.load() && !SEDAS::Saving::ShouldAllowSave(kind)) {
			logger::warn("Blocked {} save request: {}", SEDAS::Saving::SaveKindName(kind), saveName);
			return false;
		}

		if (!g_saveTarget) {
			logger::error("Save hook reached without an original save address; blocking {}", saveName);
			return false;
		}

		std::scoped_lock lock(g_saveHookLock);
		SaveHookPatchGuard patchGuard;
		return reinterpret_cast<SaveImpl_t>(g_saveTarget)(a_manager, a_deviceID, a_outputStats, a_fileName);
	}

	bool ShouldDisableSavingNow()
	{
		const auto& config = SEDAS::Settings::Get().saving;
		return config.saveOnlyAtBeds && !g_internalSaveAllowed.load() && !SEDAS::Saving::IsInBedSaveWindow();
	}

	void SetPlayerSavingDisabled(bool a_disabled)
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		auto& flags = player->GetGameStatsData().byCharGenFlag;
		if (!g_haveOriginalSavingDisabled.exchange(true)) {
			g_originalSavingDisabled.store(flags.all(RE::PlayerCharacter::ByCharGenFlag::kDisableSaving));
		}

		if (a_disabled) {
			flags.set(RE::PlayerCharacter::ByCharGenFlag::kDisableSaving);
		} else if (!g_originalSavingDisabled.load()) {
			flags.reset(RE::PlayerCharacter::ByCharGenFlag::kDisableSaving);
		}
	}

	void ScheduleSaveFlagRefresh(std::chrono::milliseconds a_delay)
	{
		std::thread([a_delay]() {
			std::this_thread::sleep_for(a_delay);
			if (auto task = SKSE::GetTaskInterface()) {
				task->AddTask([]() {
					SEDAS::Saving::RefreshSaveDisabledFlag();
				});
			}
		}).detach();
	}

	void PerformBedAutoSave()
	{
		auto manager = RE::BGSSaveLoadManager::GetSingleton();
		if (!manager) {
			logger::error("Could not request SEDAS bed autosave; BGSSaveLoadManager is unavailable");
			return;
		}

		InternalSaveGuard internalSave;
		SetPlayerSavingDisabled(false);
		manager->Save(kBedAutoSaveName);
		SEDAS::Saving::RefreshSaveDisabledFlag();
		logger::info("Requested SEDAS bed autosave");
	}
}

namespace SEDAS::Saving
{
	void InstallHooks()
	{
		const auto& config = Settings::Get().saving;
		if (!config.installSaveHook) {
			logger::info("Save hook disabled by settings");
			return;
		}

		if (g_hookInstalled.exchange(true)) {
			return;
		}

		REL::Relocation<std::uintptr_t> target{ RE::Offset::BGSSaveLoadManager::Save };
		g_saveTarget = target.address();
		std::memcpy(g_originalSaveBytes.data(), reinterpret_cast<const void*>(g_saveTarget), g_originalSaveBytes.size());

		SKSE::AllocTrampoline(1 << 6);
		PatchSaveHook();
		logger::info("Installed BGSSaveLoadManager save hook at {:X}", g_saveTarget);
		RefreshSaveDisabledFlag();
	}

	void HandleBedSaveOpportunity()
	{
		if (Settings::Get().saving.autoSaveAtBedInsteadOfWindow) {
			QueueBedAutoSave();
		} else {
			OpenBedSaveWindow();
		}
	}

	void OpenBedSaveWindow()
	{
		const auto seconds = std::max(0, Settings::Get().saving.bedSaveWindowSeconds);
		g_bedSaveWindowEndMs.store(NowMs() + static_cast<std::int64_t>(seconds) * 1000);
		RefreshSaveDisabledFlag();
		ScheduleSaveFlagRefresh(std::chrono::milliseconds(static_cast<std::int64_t>(seconds) * 1000 + 250));
		logger::info("Opened bed save window for {} seconds", seconds);
	}

	void QueueBedAutoSave()
	{
		if (g_bedAutoSaveQueued.exchange(true)) {
			logger::debug("SEDAS bed autosave is already queued");
			return;
		}

		auto saveTask = []() {
			g_bedAutoSaveQueued.store(false);
			PerformBedAutoSave();
		};

		if (auto taskInterface = SKSE::GetTaskInterface()) {
			taskInterface->AddTask(saveTask);
		} else {
			saveTask();
		}
	}

	bool IsInBedSaveWindow()
	{
		return NowMs() <= g_bedSaveWindowEndMs.load();
	}

	void RefreshSaveDisabledFlag()
	{
		SetPlayerSavingDisabled(ShouldDisableSavingNow());
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
