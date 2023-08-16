#pragma once

#include "Utils.h"
#include "Settings.h"


class LightSourceManager : public Utilities::Ticker, public Utilities::BaseFormFloat {

	void UpdateLoop(float start_h) {
		if (HasFuel(current_source)) {
            UpdateElapsed(start_h);
        } else NoFuel();
	};

	void UpdateElapsed(float start) {
		current_source->elapsed = RE::Calendar::GetSingleton()->GetHoursPassed() - start;
	};

	void NoFuel(){
		logger::info("No fuel.");
		auto plyr = RE::PlayerCharacter::GetSingleton();
        if (plyr) {
			auto fuel_item = GetBoundFuelObject();
            if (plyr->GetItemCount(fuel_item) > 0) {
                plyr->RemoveItem(fuel_item, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				ReFuel();
				logger::info("Refueled.");
			} else {
                if (Settings::enabled_plyrmsg) Utilities::MsgBoxesNotifs::InGame::NoFuel(GetName(), GetFuelName());
                // This has to be the last thing that happens in this function, which involves current_source because current_source is set to nullptr in the main thread
				RE::ActorEquipManager::GetSingleton()->UnequipObject(plyr, GetBoundObject());
				logger::info("No fuel.");
			}
        }
	}

    void Init(){
        logger::info("Initializing LightSourceManager.");
        m_Data.clear();
        bool init_failed = false;
        for (auto& src : sources) {
            if (!src.GetFormByID(src.formid, src.editorid) || !src.GetFormByID(src.fuel, src.fuel_editorid) || !src.GetBoundObject() || !src.GetBoundFuelObject() ) {
                init_failed = true;
                // continue so that the user can see all the errors
                continue;
            } 
            src.remaining = 0.f;
            src.elapsed = 0.f;
            SetData(src.formid, src.remaining);
            // check if forms are valid
        }
        if (init_failed) {
			logger::error("Failed to initialize LightSourceManager.");
            if (Settings::enabled_err_msgbox) Utilities::MsgBoxesNotifs::InGame::InitErr();
            sources.clear();
			return;
		}
        current_source = nullptr;
        is_burning = false;
        allow_equip_event_sink = true;
        logger::info("LightSourceManager initialized.");
    };

public:
    LightSourceManager(std::vector<Settings::LightSource>& data, std::chrono::milliseconds interval)
        : sources(data), Utilities::Ticker([this](float start_h) { UpdateLoop(start_h); }, interval) {Init();};

    static LightSourceManager* GetSingleton(std::vector<Settings::LightSource>& data, int u_intervall) {
        static LightSourceManager singleton(data,std::chrono::milliseconds(u_intervall));
        return &singleton;
    }
    
	std::vector<Settings::LightSource> sources;
    Settings::LightSource* current_source;
    bool is_burning;
    bool allow_equip_event_sink;

	void ReFuel() {
        if (Settings::enabled_plyrmsg) Utilities::MsgBoxesNotifs::InGame::Refuel(GetName(), GetFuelName());
        current_source->remaining = current_source->duration;
        current_source->elapsed = 0.f;
    };

	void StartBurn() {
        if (!current_source) {
			logger::error("No current source!!No current source!!No current source!!No current source!!");
            Utilities::MsgBoxesNotifs::Windows::GeneralErr();
			return;
		}
		Start(RE::Calendar::GetSingleton()->GetHoursPassed());
        is_burning = true;
        logger::info("Started to burn fuel.");
        if (Settings::enabled_remainingmsg) {
            int _remaining = Utilities::Round(current_source->remaining, 0);
            Utilities::MsgBoxesNotifs::InGame::Remaining(_remaining, GetName());
        }
	};

    void PauseBurn() {
        Stop();
        current_source->remaining -= current_source->elapsed;
        current_source->elapsed = 0.f;
    };

	void StopBurn() {
        PauseBurn();
        is_burning = false;
        current_source = nullptr;
    };

	bool HasFuel(Settings::LightSource* src) { return GetRemaining(src) > 0.0001;};

	float GetRemaining(Settings::LightSource* src) { return src->remaining - src->elapsed; };

    bool IsValidSource(RE::FormID eqp_obj) {
        for (auto& src : sources) {
            if (eqp_obj == src.formid) return true;
        }
        return false;
    };

	bool SetSource(RE::FormID eqp_obj) {
        for (auto& src : sources) {
            if (eqp_obj == src.formid) {
                current_source = &src;
                return true;
            }
        }
        logger::error("Did not find a match!!!");
        return false;
	};

    std::string_view GetName() { return current_source->GetName(); };
    std::string_view GetFuelName() { return current_source->GetFuelName(); };
    RE::TESBoundObject* GetBoundObject() { return current_source->GetBoundObject(); };
    RE::TESBoundObject* GetBoundFuelObject() { return current_source->GetBoundFuelObject(); }

    const char* GetType() override { return "Manager"; }

    void SendData() {
        for (auto& src : sources) {
            SetData(src.formid, src.remaining);
        }
    };

    void ReceiveData() {
        for (auto& src : sources) {
            for (auto& pair : m_Data) {
				if (pair.first == src.formid) {
                    src.remaining = std::min(pair.second,src.duration);
					break;
				}
			}
        }
    };
    
    void Reset(){
        logger::info("Resetting LightSourceManager.");
        Stop();
		Init();
    };
};
	