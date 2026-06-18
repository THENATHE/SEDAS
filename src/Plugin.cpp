#include "SEDAS/Plugin.h"

#include "SEDAS/DeathAlternative.h"
#include "SEDAS/Events.h"
#include "SEDAS/Saving.h"
#include "SEDAS/Serialization.h"
#include "SEDAS/Settings.h"
#include "SEDAS/SoulEconomy.h"
#include "SEDAS/UI.h"

namespace SEDAS::Plugin
{
	void SetupLog()
	{
		auto path = logger::log_directory();
		if (!path) {
			return;
		}

		*path /= "SEDAS.log";

		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
		auto log = std::make_shared<spdlog::logger>("SEDAS", std::move(sink));

		spdlog::set_default_logger(std::move(log));
		spdlog::set_level(spdlog::level::trace);
		spdlog::flush_on(spdlog::level::trace);
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

