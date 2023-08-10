#include "Ticker.h"
#include "logger.h"

float duration = 8.0f;


void WorldChecks::UpdateLoop(float start_h) {
    // dostuff
    logger::info("Time is ticking...");

    float curHours = RE::Calendar::GetSingleton()->GetHoursPassed();
    logger::info("Current hours: {}", curHours);
    logger::info("Pi: {}", start_h);
};

class OurEventSink : public RE::BSTEventSink<RE::TESEquipEvent>, public RE::BSTEventSink<RE::TESMagicEffectApplyEvent> {
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
        auto obj = RE::TESForm::LookupByID(event->baseObject);
        if (obj && (std::string_view)obj->GetName() == "Iron Lantern") {
            if (event->equipped) logger::info("Iron Lantern equipped");
            else logger::info("Iron Lantern unequipped");
        }
        if (gameIsPaused) logger::info("asdasd");


        return RE::BSEventNotifyControl::kContinue;
    }
    
    RE::BSEventNotifyControl ProcessEvent(const RE::TESMagicEffectApplyEvent* event, RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*) {
        if (!event) return RE::BSEventNotifyControl::kContinue;
        auto mgeff = RE::TESForm::LookupByID<RE::EffectSetting>(event->magicEffect);
        if ((std::string_view)mgeff->GetName() == "Lantern Light") {
            auto timer = WorldChecks::UpdateTicker::GetSingleton();
            timer->Start(RE::Calendar::GetSingleton()->GetHoursPassed());
        }
        
        return RE::BSEventNotifyControl::kContinue;
    }

};


void OnMessage(SKSE::MessagingInterface::Message* message) {
    switch (message->type) {
        case SKSE::MessagingInterface::kInputLoaded:
            break;
        case SKSE::MessagingInterface::kPostPostLoad:
            auto* eventSink = OurEventSink::GetSingleton();
            //ui->AddEventSink<RE::MenuOpenCloseEvent>(eventSink);
            auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
            eventSourceHolder->AddEventSink<RE::TESEquipEvent>(eventSink);
            eventSourceHolder->AddEventSink<RE::TESMagicEffectApplyEvent>(eventSink);
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