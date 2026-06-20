#include "SEDAS/Events.h"

#include "SEDAS/DeathAlternative.h"
#include "SEDAS/Saving.h"
#include "SEDAS/SoulEconomy.h"
#include "SEDAS/State.h"

namespace
{
	bool IsPlayer(RE::TESObjectREFR* a_ref)
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		return player && a_ref == player;
	}

	bool IsSleepFurniture(RE::TESObjectREFR* a_ref)
	{
		if (!a_ref) {
			return false;
		}

		auto base = a_ref->GetBaseObject();
		auto furniture = base ? base->As<RE::TESFurniture>() : nullptr;
		return furniture && furniture->furnFlags.all(RE::TESFurniture::ActiveMarker::kCanSleep);
	}

	RE::NiPointer<RE::TESObjectREFR> GetOccupiedFurniture(RE::Actor* a_actor)
	{
		if (!a_actor) {
			return nullptr;
		}

		return a_actor->GetOccupiedFurniture().get();
	}

	RE::TESObjectREFR* FindPlayerSleepFurniture()
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		auto occupied = GetOccupiedFurniture(player);
		if (IsSleepFurniture(occupied.get())) {
			return occupied.get();
		}

		return nullptr;
	}

	class EventSink :
		public RE::BSTEventSink<RE::TESFurnitureEvent>,
		public RE::BSTEventSink<RE::TESSleepStopEvent>,
		public RE::BSTEventSink<RE::TESEnterBleedoutEvent>,
		public RE::BSTEventSink<RE::TESDeathEvent>,
		public RE::BSTEventSink<RE::TESHitEvent>,
		public RE::BSTEventSink<RE::TESTrackedStatsEvent>
	{
	public:
		static EventSink* GetSingleton()
		{
			static EventSink sink;
			return std::addressof(sink);
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESFurnitureEvent* a_event, RE::BSTEventSource<RE::TESFurnitureEvent>*) override
		{
			if (!a_event || !IsPlayer(a_event->actor.get())) {
				return RE::BSEventNotifyControl::kContinue;
			}

			if (a_event->type.all(RE::TESFurnitureEvent::FurnitureEventType::kEnter)) {
				auto target = a_event->targetFurniture.get();
				if (IsSleepFurniture(target)) {
					SEDAS::State::RecordCandidateBed(target);
				} else if (auto occupied = FindPlayerSleepFurniture()) {
					SEDAS::State::RecordCandidateBed(occupied);
				} else {
					logger::debug("Player entered non-sleep furniture or unresolved furniture");
				}
			}

			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESSleepStopEvent* a_event, RE::BSTEventSource<RE::TESSleepStopEvent>*) override
		{
			if (!a_event) {
				return RE::BSEventNotifyControl::kContinue;
			}

			logger::info("Sleep stop event received; interrupted={}", a_event->interrupted);

			bool committedBed = false;
			if (auto occupied = FindPlayerSleepFurniture()) {
				committedBed = SEDAS::State::CommitBed(occupied);
			}

			if (!committedBed) {
				committedBed = SEDAS::State::CommitCandidateBed();
			}

			if (committedBed) {
				SEDAS::Saving::HandleBedSaveOpportunity();
			} else {
				logger::warn("Sleep stop event did not have a recorded or occupied bed");
			}

			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESEnterBleedoutEvent* a_event, RE::BSTEventSource<RE::TESEnterBleedoutEvent>*) override
		{
			if (a_event && IsPlayer(a_event->actor.get())) {
				SEDAS::DeathAlternative::TryStartRecovery("bleedout");
			}

			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESDeathEvent* a_event, RE::BSTEventSource<RE::TESDeathEvent>*) override
		{
			if (a_event && a_event->dead && IsPlayer(a_event->actorDying.get())) {
				SEDAS::DeathAlternative::TryStartRecovery("death");
			}

			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>*) override
		{
			if (!a_event || !IsPlayer(a_event->target.get())) {
				return RE::BSEventNotifyControl::kContinue;
			}

			auto player = RE::PlayerCharacter::GetSingleton();
			if (player && player->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth) <= 1.0F) {
				SEDAS::DeathAlternative::TryStartRecovery("lethal hit");
			}

			SEDAS::SoulEconomy::Refresh();
			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESTrackedStatsEvent*, RE::BSTEventSource<RE::TESTrackedStatsEvent>*) override
		{
			SEDAS::SoulEconomy::Refresh();
			return RE::BSEventNotifyControl::kContinue;
		}
	};
}

namespace SEDAS::Events
{
	void Install()
	{
		auto holder = RE::ScriptEventSourceHolder::GetSingleton();
		if (!holder) {
			logger::critical("ScriptEventSourceHolder unavailable");
			return;
		}

		auto sink = EventSink::GetSingleton();
		holder->AddEventSink<RE::TESFurnitureEvent>(sink);
		holder->AddEventSink<RE::TESSleepStopEvent>(sink);
		holder->AddEventSink<RE::TESEnterBleedoutEvent>(sink);
		holder->AddEventSink<RE::TESDeathEvent>(sink);
		holder->AddEventSink<RE::TESHitEvent>(sink);
		holder->AddEventSink<RE::TESTrackedStatsEvent>(sink);
		logger::info("SEDAS event sinks registered");
	}
}
