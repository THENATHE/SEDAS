#include "SEDAS/Plugin.h"

#include "SEDAS/DeathAlternative.h"
#include "SEDAS/Events.h"
#include "SEDAS/Saving.h"
#include "SEDAS/Serialization.h"
#include "SEDAS/Settings.h"
#include "SEDAS/SoulEconomy.h"
#include "SEDAS/UI.h"

#include <REX/W32/KERNEL32.h>

namespace
{
	std::optional<std::filesystem::path> CurrentModuleDirectory()
	{
		auto module = REX::W32::GetCurrentModule();
		if (!module) {
			return std::nullopt;
		}

		std::wstring buffer(REX::W32::MAX_PATH, L'\0');
		for (;;) {
			const auto copied = REX::W32::GetModuleFileNameW(module, buffer.data(), static_cast<std::uint32_t>(buffer.size()));
			if (copied == 0) {
				return std::nullopt;
			}

			if (copied < buffer.size() - 1) {
				buffer.resize(copied);
				return std::filesystem::path(buffer).parent_path();
			}

			buffer.resize(buffer.size() * 2);
		}
	}

	std::vector<std::filesystem::path> LogPathCandidates()
	{
		std::vector<std::filesystem::path> candidates;

		if (auto skseLogDir = logger::log_directory()) {
			candidates.push_back(*skseLogDir / "SEDAS.log");
		}

		if (auto moduleDir = CurrentModuleDirectory()) {
			candidates.push_back(*moduleDir / "SEDAS.log");
		}

		return candidates;
	}
}

namespace SEDAS::Plugin
{
	void SetupLog()
	{
		for (const auto& path : LogPathCandidates()) {
			try {
				std::filesystem::create_directories(path.parent_path());

				auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.string(), true);
				auto log = std::make_shared<spdlog::logger>("SEDAS", std::move(sink));

				spdlog::set_default_logger(std::move(log));
				spdlog::set_level(spdlog::level::trace);
				spdlog::flush_on(spdlog::level::trace);
				logger::info("Writing SEDAS log to {}", path.string());
				return;
			} catch (...) {
			}
		}
	}

	void OnDataLoaded()
	{
		Settings::Load();
		Events::Install();
		DeathAlternative::Install();
		Saving::InstallHooks();
		UI::Register();
		SoulEconomy::Refresh();
		DeathAlternative::RefreshPlayerEssentialFlag();
		logger::info("{} initialized", PrettyName);
	}

	void OnNewGameOrLoad()
	{
		DeathAlternative::ClearRecoveryState();
		Settings::Load();
		SoulEconomy::Refresh();
		DeathAlternative::RefreshPlayerEssentialFlag();
	}
}

namespace
{
	void OnMessage(SKSE::MessagingInterface::Message* a_message)
	{
		switch (a_message->type) {
		case SKSE::MessagingInterface::kPreLoadGame:
			SEDAS::DeathAlternative::ClearRecoveryState();
			break;
		case SKSE::MessagingInterface::kDataLoaded:
			SEDAS::Plugin::OnDataLoaded();
			break;
		case SKSE::MessagingInterface::kNewGame:
		case SKSE::MessagingInterface::kPostLoadGame:
			SEDAS::Plugin::OnNewGameOrLoad();
			break;
		default:
			break;
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	SEDAS::Plugin::SetupLog();

	logger::info("Loading {} {}.{}.{}",
		SEDAS::Plugin::PrettyName,
		SEDAS::Plugin::VersionMajor,
		SEDAS::Plugin::VersionMinor,
		SEDAS::Plugin::VersionPatch);

	SEDAS::Serialization::Install();

	auto messaging = SKSE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(OnMessage)) {
		logger::critical("Failed to register SKSE messaging listener");
		return false;
	}

	return true;
}
