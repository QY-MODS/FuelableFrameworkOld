#include "logger.h"

bool PostPostLoaded = false;
RE::UI* ui = nullptr;
bool gameIsPaused = false;
float duration = 8.0f;
float remaining = 8.0f;



class OurEventSink : public RE::BSTEventSink<RE::MenuOpenCloseEvent>,
                     public RE::BSTEventSink<RE::TESEquipEvent>{
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

    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* event,
                                          RE::BSTEventSource<RE::MenuOpenCloseEvent>*) {
        if (!event) return RE::BSEventNotifyControl::kContinue;
        
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* event, RE::BSTEventSource<RE::TESEquipEvent>*) {
        if (!event) return RE::BSEventNotifyControl::kContinue;
        auto obj = RE::TESForm::LookupByID(event->baseObject);
        if (obj && (std::string_view)obj->GetName() == "Iron Lantern") {
            auto asd = RE::SkyrimVM::GetSingleton();
            logger::info("{}", asd->queuedOnUpdateEvents.size());
            auto lmao = RE::BSScript::Internal::VirtualMachine::GetSingleton();
            //logger::info("{}", asd->queuedOnUpdateEvents.size());
            /*for (auto& i : asd->queuedOnUpdateEvents) {
				logger::info("updateType: {}", (int)i.get()->updateType);
                logger::info("timeToSendEvent: {}", i.get()->timeToSendEvent);
                logger::info("updateTime: {}", i.get()->updateTime);
                logger::info("handle: {}", (int)i.get()->handle);
			}*/
            //gameIsPaused = true;
            
        }
        if (gameIsPaused) logger::info("asdasd");


        return RE::BSEventNotifyControl::kContinue;
    }
    RE::BSEventNotifyControl ProcessEvent(const RE::TESTriggerEvent* event, RE::BSTEventSource<RE::TESTriggerEvent>*) {
        if (!event) return RE::BSEventNotifyControl::kContinue;

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
                ui->AddEventSink<RE::MenuOpenCloseEvent>(OurEventSink::GetSingleton());
                RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESEquipEvent>(OurEventSink::GetSingleton());
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