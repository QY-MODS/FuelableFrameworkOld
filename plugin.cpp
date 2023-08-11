#include "Ticker.h"
//#include "logger.h"
#include "Settings.h"

auto timer = WorldChecks::UpdateTicker::GetSingleton();
std::vector<Settings::LightSource> sources;
Settings::LightSource* current_source;
int initial_fuel_count = 1;


void WorldChecks::UpdateLoop(float start_h) {
    if (current_source->remaining - current_source->elapsed > 0.0001) {
        current_source->elapsed = RE::Calendar::GetSingleton()->GetHoursPassed() - start_h;
        logger::info("Remaining hours: {}", current_source->remaining - current_source->elapsed);
    } 
    else {
        auto plyr = RE::PlayerCharacter::GetSingleton();
        if (plyr) {
            RE::ActorEquipManager::GetSingleton()->UnequipObject(plyr, RE::TESForm::LookupByID(current_source->formid)->As<RE::TESBoundObject>());
        }
        current_source->remaining = 0.0f;
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
        auto item_name = (std::string_view)RE::TESForm::LookupByID(event->baseObject)->GetName();
        auto is_source = false;
        for (auto& src : sources) {
            current_source = &src;
            if (item_name == current_source->name) {
                is_source = true;
                if (initial_fuel_count) {
                    logger::info("Initial fuel count: {}", initial_fuel_count);
                    current_source->ReFuel();
                    initial_fuel_count--;
                }
                break;
            }
        }
        if (!is_source) return RE::BSEventNotifyControl::kContinue;
        if (!current_source->formid) current_source->formid = event->baseObject;
        if (event->equipped) {
			logger::info("{} equipped.",current_source->name);
            timer->Start(RE::Calendar::GetSingleton()->GetHoursPassed());
            logger::info("timer started.");
		}
        else {
            logger::info("{} unequipped.", current_source->name);
			timer->Stop();
            current_source->remaining -= current_source->elapsed;
            current_source->elapsed = 0.0f;
            logger::info("timer stopped.");
		}
        return RE::BSEventNotifyControl::kContinue;
    }

};


void OnMessage(SKSE::MessagingInterface::Message* message) {
    switch (message->type) {
        case SKSE::MessagingInterface::kInputLoaded:
            break;
        case SKSE::MessagingInterface::kPostPostLoad:
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESEquipEvent>(OurEventSink::GetSingleton());
            break;
    }
};


SKSEPluginLoad(const SKSE::LoadInterface *skse) {

    SetupLog();
    spdlog::set_pattern("%v");
    SKSE::Init(skse);
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    sources = Settings::LoadSettings();

    return true;
}