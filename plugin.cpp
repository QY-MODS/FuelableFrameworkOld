#include "Ticker.h"
//#include "logger.h"
#include "Settings.h"

std::uint32_t formid;
auto timer = WorldChecks::UpdateTicker::GetSingleton();
auto remaining = 0.0f;
float duration = 0.5f;


void WorldChecks::UpdateLoop(float start_h) {
    if (remaining > 0.0001) {
        remaining = duration - RE::Calendar::GetSingleton()->GetHoursPassed() + start_h;
        logger::info("Remaining hours: {}", remaining);
    } 
    else {
        auto plyr = RE::PlayerCharacter::GetSingleton();
        if (plyr) {
            RE::ActorEquipManager::GetSingleton()->UnequipObject(plyr, RE::TESForm::LookupByID(formid)->As<RE::TESBoundObject>());
        }
        remaining = 0.0f;
    }
};

class OurEventSink : public RE::BSTEventSink<RE::TESEquipEvent> {
    OurEventSink() = default;
    OurEventSink(const OurEventSink&) = delete;
    OurEventSink(OurEventSink&&) = delete;
    OurEventSink& operator=(const OurEventSink&) = delete;
    OurEventSink& operator=(OurEventSink&&) = delete;

public:
    static OurEventSink* GetSingleton() {
        static OurEventSink singleton;
        return &singleton;
    }
    
    RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* event, RE::BSTEventSource<RE::TESEquipEvent>*) {
        if (!event) return RE::BSEventNotifyControl::kContinue;
        if (!formid && (std::string_view)RE::TESForm::LookupByID(event->baseObject)->GetName() == "Iron Lantern") {
			formid = event->baseObject;
			logger::info("Lantern formid: {}", formid);
		}
        
        if (event->baseObject == formid) {
            if (event->equipped) {
				logger::info("Lantern equipped.");
                timer->Start(RE::Calendar::GetSingleton()->GetHoursPassed());
			}
            else {
				logger::info("Lantern unequipped.");
				timer->Stop();
                logger::info("timer stopped.");
			}
        }
        return RE::BSEventNotifyControl::kContinue;
    }

};


void OnMessage(SKSE::MessagingInterface::Message* message) {
    switch (message->type) {
        case SKSE::MessagingInterface::kInputLoaded:
            break;
        case SKSE::MessagingInterface::kPostPostLoad:
            remaining = duration;
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESEquipEvent>(OurEventSink::GetSingleton());
            break;
    }
};


SKSEPluginLoad(const SKSE::LoadInterface *skse) {

    SetupLog();
    spdlog::set_pattern("%v");
    SKSE::Init(skse);
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    auto sources = Settings::LoadSettings();
    logger::info("Loaded {} sources.", sources.size());

    return true;
}