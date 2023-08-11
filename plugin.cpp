#include "Ticker.h"
//#include "logger.h"
#include "Settings.h"

LightSourceManager* LSM;

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
        if (!LSM->SetSource(event->baseObject)) return RE::BSEventNotifyControl::kContinue;
        if (event->equipped) {
            logger::info("{} equipped.", LSM->GetName(LSM->current_source->formid));
            LSM->StartBurn();
		}
        else {
            logger::info("{} unequipped.", LSM->GetName(LSM->current_source->formid));
			LSM->StopBurn();
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
    auto sources = Settings::LoadSettings();
    LSM = LightSourceManager::GetSingleton(sources, 500);

    return true;
}