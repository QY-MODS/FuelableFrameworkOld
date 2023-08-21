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
            logger::info("Equip event!");
            if (LSM->IsCurrentSource(event->baseObject)) {
                logger::info("Already equipped!!! Dunno how that can happen?! Ignoring...");
                return RE::BSEventNotifyControl::kContinue;
            }
            if (!LSM->SetSource(event->baseObject)) logger::info("Failed to set source. Something is terribly wrong!!!");
            logger::info("{} equipped.", LSM->GetName());
            LSM->StartBurn();
		}
        else {
            logger::info("Unequip event!");
            if (!LSM->IsCurrentSource(event->baseObject)) {
                logger::info("How is this possible?! Ignoring..."); // Either already unequipped or another source is getting unequipped without getting equipped in the first place.
                return RE::BSEventNotifyControl::kContinue;
            }
            logger::info("{} unequipped.", LSM->GetName());
            LSM->StopBurn();
            logger::info("timer stopped.");
		}
        return RE::BSEventNotifyControl::kContinue;
    }
};

void OnMessage(SKSE::MessagingInterface::Message* message) {
    switch (message->type) {
        case SKSE::MessagingInterface::kPostPostLoad:
            logger::info("Postpostload.");
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESEquipEvent>(OurEventSink::GetSingleton());
            break;
        case SKSE::MessagingInterface::kNewGame:
            logger::info("Newgame.");
            if (LSM->sources.empty()) {
                logger::info("No sources found.");
                if (Settings::enabled_err_msgbox) Utilities::MsgBoxesNotifs::InGame::NoSourceFound();
                return;
            }
            LSM->Reset();
            logger::info("Newgame LSM reset succesful.");
            break;
        case SKSE::MessagingInterface::kPreLoadGame:
            logger::info("Preload.");
            LSM->Reset();
            logger::info("Preload LSM reset succesful.");
			break;
        case SKSE::MessagingInterface::kPostLoadGame:
            logger::info("Postload.");
            if (LSM->sources.empty()) {
                logger::info("No sources found.");
                if (Settings::enabled_err_msgbox) Utilities::MsgBoxesNotifs::InGame::NoSourceFound();
                return;
            } else LSM->LogRemainings();
            if (LSM->current_source) LSM->StartBurn();
            logger::info("Postload LSM succesful.");
			break;
        case SKSE::MessagingInterface::kDataLoaded:
			logger::info("Dataloaded.");
            auto sources = Settings::LoadINISettings();
            LSM = LightSourceManager::GetSingleton(sources, 500);
            logger::info("enabled_editor_id: {}", Settings::force_editor_id);
            logger::info("enabled_plyrmsg: {}", Settings::enabled_plyrmsg);
            logger::info("enabled_err_msgbox: {}", Settings::enabled_err_msgbox);
			break;
    }
};

void SaveCallback(SKSE::SerializationInterface* serializationInterface) {
    logger::info("Save Start");
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
    logger::info("Load Start");
    std::uint32_t type;
    std::uint32_t version;
    std::uint32_t length;
    logger::info("Load Start");
    uint32_t equipped_obj_id;
    while (serializationInterface->GetNextRecordInfo(type, version, length)) {
        auto temp = Utilities::DecodeTypeCode(type);
        logger::info("Trying Load for {}", temp);

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
                logger::info("read equipped_obj_id: {}", equipped_obj_id);
                if (equipped_obj_id) LSM->SetSource(equipped_obj_id);
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
    SKSE::log::trace("Initializing cosave serialization...");
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