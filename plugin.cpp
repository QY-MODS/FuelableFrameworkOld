#include "Ticker.h"
#include "logger.h"

float duration = 0.025f;
auto timer = WorldChecks::UpdateTicker::GetSingleton();


void WorldChecks::UpdateLoop(float start_h, RE::EffectSetting* _mgeff) {
    logger::info("Time is ticking...");
    auto remaining = duration - RE::Calendar::GetSingleton()->GetHoursPassed() + start_h;
    logger::info("Remaining hours: {}", remaining);
    auto plyr = RE::PlayerCharacter::GetSingleton()->AsMagicTarget();
    if (!plyr) {
        logger::info("Player is not a magic target.");
    }
    else {
        if (remaining <= 0.001 && plyr->HasMagicEffect(_mgeff)) {
            timer->Stop();
            logger::info("timer stopped.");
        }
    }
};

class OurEventSink :public RE::BSTEventSink<RE::TESMagicEffectApplyEvent> {
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
    
    RE::BSEventNotifyControl ProcessEvent(const RE::TESMagicEffectApplyEvent* event, RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*) {
        if (!event) return RE::BSEventNotifyControl::kContinue;
        logger::info("Magic effect event.");
        auto mgeff = RE::TESForm::LookupByID<RE::EffectSetting>(event->magicEffect);
        if (mgeff && (std::string_view) mgeff->GetName() == "Lantern Light") {
            timer->Start(RE::Calendar::GetSingleton()->GetHoursPassed(),mgeff);
        }
        
        return RE::BSEventNotifyControl::kContinue;
    }

};


void OnMessage(SKSE::MessagingInterface::Message* message) {
    switch (message->type) {
        case SKSE::MessagingInterface::kInputLoaded:
            break;
        case SKSE::MessagingInterface::kPostPostLoad:
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESMagicEffectApplyEvent>(OurEventSink::GetSingleton());
            break;
    }
};


SKSEPluginLoad(const SKSE::LoadInterface *skse) {

    SetupLog();
    spdlog::set_pattern("%v");
    SKSE::Init(skse);
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);

    return true;
}