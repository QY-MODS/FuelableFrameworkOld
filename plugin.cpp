#include "Manager.h"

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
        //if (!LSM->allow_equip_event_sink) return RE::BSEventNotifyControl::kContinue;
        if (!LSM->IsValidSource(event->baseObject)) return RE::BSEventNotifyControl::kContinue;
        if (event->equipped) {
            if (LSM->IsCurrentSource(event->baseObject)) return RE::BSEventNotifyControl::kContinue;
            if (!LSM->SetSource(event->baseObject)) logger::info("Failed to set source. Something is terribly wrong!!!");
            LSM->StartBurn();
		}
        else {
            if (!LSM->IsCurrentSource(event->baseObject)) return RE::BSEventNotifyControl::kContinue;
            LSM->StopBurn();
		}
        return RE::BSEventNotifyControl::kContinue;
    }
};

void OnMessage(SKSE::MessagingInterface::Message* message) {
    switch (message->type) {
        case SKSE::MessagingInterface::kPostPostLoad:
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESEquipEvent>(OurEventSink::GetSingleton());
            break;
        case SKSE::MessagingInterface::kNewGame:
            if (LSM->sources.empty()) {
                logger::info("No sources found.");
                if (Settings::enabled_err_msgbox) Utilities::MsgBoxesNotifs::InGame::NoSourceFound();
                return;
            }
            LSM->Reset();
            logger::info("Newgame LSM reset succesful.");
            break;
        case SKSE::MessagingInterface::kPreLoadGame:
            LSM->Reset();
            logger::info("Preload LSM reset succesful.");
			break;
        case SKSE::MessagingInterface::kPostLoadGame:
            if (LSM->sources.empty()) {
                logger::info("No sources found (PostLoad).");
                if (Settings::enabled_err_msgbox) Utilities::MsgBoxesNotifs::InGame::NoSourceFound();
                return;
            } else LSM->LogRemainings();
            if (LSM->current_source) LSM->StartBurn();
            logger::info("Postload LSM succesful.");
			break;
        case SKSE::MessagingInterface::kDataLoaded:
            auto sources = Settings::LoadINISettings();
            LSM = LightSourceManager::GetSingleton(sources, 500);
			break;
    }
};

void SaveCallback(SKSE::SerializationInterface* serializationInterface) {
    if (LSM->is_burning) LSM->PauseBurn();
    LSM->SendData();
    LSM->LogRemainings();
    if (!LSM->Save(serializationInterface, Settings::kDataKey, Settings::kSerializationVersion)) {
        logger::critical("Failed to save Data");
    }
    uint32_t equipped_obj_id = LSM->current_source ? LSM->current_source->formid : 0;
    serializationInterface->WriteRecordData(equipped_obj_id);
    if (LSM->is_burning) {
        LSM->UnPauseBurn();
        logger::info("Data Saved");
    }
}

void LoadCallback(SKSE::SerializationInterface* serializationInterface) {
    std::uint32_t type;
    std::uint32_t version;
    std::uint32_t length;
    uint32_t equipped_obj_id;
    while (serializationInterface->GetNextRecordInfo(type, version, length)) {
        auto temp = Utilities::DecodeTypeCode(type);

        if (version != Settings::kSerializationVersion) {
            logger::critical("Loaded data has incorrect version. Recieved ({}) - Expected ({}) for Data Key ({})",
                version, Settings::kSerializationVersion,temp);
            continue;
        }
        switch (type) {
            case Settings::kDataKey: {
                if (!LSM->Load(serializationInterface)) {
                    logger::critical("Failed to Load Data");
                }
                serializationInterface->ReadRecordData(equipped_obj_id);
                if (equipped_obj_id) {
                    if (!LSM->SetSource(equipped_obj_id)) {
                        Utilities::MsgBoxesNotifs::InGame::LoadOrderError();
                    }
                }
            } break;
            default:
                logger::critical("Unrecognized Record Type: {}", temp);
                break;
        }
    }
    LSM->ReceiveData();
    logger::info("Data loaded from skse co-save.");
}

void InitializeSerialization() {
    auto* serialization = SKSE::GetSerializationInterface();
    serialization->SetUniqueID(Settings::kDataKey);
    serialization->SetSaveCallback(SaveCallback);
    // serialization->SetRevertCallback(LightSourceManager::RevertCallback);
    serialization->SetLoadCallback(LoadCallback);
    SKSE::log::trace("Cosave serialization initialized.");
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {

    SetupLog();
    spdlog::set_pattern("%v");
    SKSE::Init(skse);

    InitializeSerialization();
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);

    return true;
}