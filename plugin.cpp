#include "Ticker.h"

bool PostPostLoaded = false;
RE::UI* ui = nullptr;
bool gameIsPaused = false;
float duration = 8.0f;
float remaining = 8.0f;

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
            logger::info("Iron Lantern equipped");
        }
        if (gameIsPaused) logger::info("asdasd");


        return RE::BSEventNotifyControl::kContinue;
    }
    
    RE::BSEventNotifyControl ProcessEvent(const RE::TESMagicEffectApplyEvent* event, RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*) {
        if (!event) return RE::BSEventNotifyControl::kContinue;
        logger::info("Magic effect applied");
        auto mgeff = RE::TESForm::LookupByID<RE::EffectSetting>(event->magicEffect);
        if ((std::string_view)mgeff->GetName() == "Lantern Light") {
            auto timer = WorldChecks::UpdateTicker::GetSingleton();
        }
        
        return RE::BSEventNotifyControl::kContinue;
    }

};


void OnMessage(SKSE::MessagingInterface::Message* message) {
    switch (message->type) {
        case SKSE::MessagingInterface::kInputLoaded:
            break;
        case SKSE::MessagingInterface::kPostPostLoad:
            if (!PostPostLoaded) {
                ui = RE::UI::GetSingleton();
                auto* eventSink = OurEventSink::GetSingleton();
                //ui->AddEventSink<RE::MenuOpenCloseEvent>(eventSink);
                auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
                eventSourceHolder->AddEventSink<RE::TESEquipEvent>(eventSink);
                eventSourceHolder->AddEventSink<RE::TESMagicEffectApplyEvent>(eventSink);
                PostPostLoaded = true;
            }
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