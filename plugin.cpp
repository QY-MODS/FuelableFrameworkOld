#include "Manager.h"


RE::FormID lightid;

LightSourceManager* LSM;
using InputEvents = RE::InputEvent*;
class OurEventSink : public RE::BSTEventSink<RE::TESEquipEvent>,
                     public RE::BSTEventSink<RE::TESActiveEffectApplyRemoveEvent>,
                     public RE::BSTEventSink<RE::TESSpellCastEvent>,
                     public RE::BSTEventSink<RE::TESMagicEffectApplyEvent>,
                     public RE::BSTEventSink<RE::InputEvent*> {
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
        if (!LSM->allow_equip_event_sink) return RE::BSEventNotifyControl::kContinue;
        if (!LSM->IsValidSource(event->baseObject)) return RE::BSEventNotifyControl::kContinue;
        if (event->equipped) {
            if (!LSM->SetSource(event->baseObject)) logger::info("Failed to set source. Something is terribly wrong!!!");
            //logger::info("{} equipped.", LSM->GetName());
            LSM->StartBurn();
		}
        else {
            //logger::info("{} unequipped.", LSM->GetName());
            LSM->StopBurn();
            //logger::info("timer stopped.");
		}
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::TESActiveEffectApplyRemoveEvent* event, RE::BSTEventSource<RE::TESActiveEffectApplyRemoveEvent>*) {
        if (!event) return RE::BSEventNotifyControl::kContinue;
        logger::info("TESActiveEffectApplyRemoveEvent.");
        if (event->isApplied) {
            logger::info("{} is applied", event->activeEffectUniqueID);
        } else {
			logger::info("{} is removed", event->activeEffectUniqueID);
		}
        
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* event, RE::BSTEventSource<RE::TESSpellCastEvent>*) {
        if (!event) return RE::BSEventNotifyControl::kContinue;
        logger::info("TESSpellCastEvent.");
        logger::info("spellname:{}", RE::TESForm::LookupByID(event->spell)->GetName());
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::TESMagicEffectApplyEvent* event, RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*) {
        if (!event) return RE::BSEventNotifyControl::kContinue;
        logger::info("Magic effect event.");
        auto mgeff = RE::TESForm::LookupByID<RE::EffectSetting>(event->magicEffect);
        auto mgeff_form = RE::TESForm::LookupByID(event->magicEffect);
        logger::info("mgeff:{}", mgeff_form->GetName());

        if (mgeff && (std::string_view) mgeff_form->GetName() == "Lantern Light") {
            logger::info("Lantern Light.");
            auto mg_caster = event->caster.get()->As<RE::MagicCaster>();
            if (!mg_caster) logger::info("Lantern Light.");
            auto mg_target = event->target.get()->As<RE::MagicTarget>();
            if (!mg_target) logger::info("Lantern Light.");
            //logger::info("current spell name:{}", mg_caster->currentSpell->GetName());
        }
        /*if (mgeff && (std::string_view)mgeff->GetName() == "Lantern Light") {
        }*/

        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* evns, RE::BSTEventSource<RE::InputEvent*>*) {
        if (!*evns) return RE::BSEventNotifyControl::kContinue;

        for (RE::InputEvent* e = *evns; e; e = e->next) {
            if (e->eventType.get() == RE::INPUT_EVENT_TYPE::kButton) {
                RE::ButtonEvent* a_event = e->AsButtonEvent();
                uint32_t keyMask = a_event->idCode;
                
                float duration = a_event->heldDownSecs;
                bool isReleased = a_event->value == 0 && duration != 0;

                if (isReleased && keyMask==47) {
                    logger::info("key released.");
                    auto light = RE::TESForm::LookupByID<RE::TESObjectLIGH>(lightid);
                    if (!light) {
						logger::info("light not found.");
						return RE::BSEventNotifyControl::kContinue;
					}
                    auto player = RE::PlayerCharacter::GetSingleton();
                    light->data.radius = 0;
                    player->GetInfoRuntimeData().thirdPersonLight;
                    //logger::info("lenth {}", player->GetInfoRuntimeData().thirdPersonLight.get()->light.get()->GetLightRuntimeData().radius.Length());

                }
            }
        }

        return RE::BSEventNotifyControl::kContinue;
    }
};
//mgeff->data.light->data.radius = 0.0f;

void OnMessage(SKSE::MessagingInterface::Message* message) {
    switch (message->type) {
        case SKSE::MessagingInterface::kInputLoaded:
            logger::info("Inputloaded.");
            RE::BSInputDeviceManager::GetSingleton()->AddEventSink(OurEventSink::GetSingleton());
            break;
        case SKSE::MessagingInterface::kPostPostLoad:
            logger::info("Postpostloadddd.");
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESEquipEvent>(OurEventSink::GetSingleton());
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESActiveEffectApplyRemoveEvent>(OurEventSink::GetSingleton());
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESSpellCastEvent>(OurEventSink::GetSingleton());
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESMagicEffectApplyEvent>(OurEventSink::GetSingleton());
            break;
        case SKSE::MessagingInterface::kNewGame:
            logger::info("Newgame.");
            if (LSM->sources.empty()) {
                logger::info("No sources found.");
                if (Settings::enabled_err_msgbox) Utilities::MsgBoxesNotifs::InGame::NoSourceFound();
                return;
            }
            LSM->Reset();
            //logger::info("Newgame LSM reset succesful.");
            break;
        case SKSE::MessagingInterface::kPreLoadGame:
            logger::info("Preload.");
            LSM->Reset();
            //logger::info("Preload LSM reset succesful.");
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
            //logger::info("enabled_editor_id: {}", Settings::force_editor_id);
            //logger::info("enabled_plyrmsg: {}", Settings::enabled_plyrmsg);
            //logger::info("enabled_err_msgbox: {}", Settings::enabled_err_msgbox);
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
        LSM->StartBurn();
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