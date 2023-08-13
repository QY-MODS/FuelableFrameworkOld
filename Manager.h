#pragma once

#include "Utils.h"
#include "Settings.h"


class LightSourceManager : public Utilities::Ticker, public Utilities::BaseFormFloat {

	void UpdateLoop(float start_h) {
        logger::info("Updating LightSourceManager.");
        if (!current_source) {
			Stop();
            logger::info("No current source!!No current source!!No current source!!No current source!!");
        }
		if (HasFuel(current_source)) {
            UpdateElapsed(start_h);
			logger::info("Remaining hours: {}", current_source->remaining - current_source->elapsed);
        } else NoFuel();
	};

	void UpdateElapsed(float start) {
        logger::info("Updating elapsed time.");
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
				RE::ActorEquipManager::GetSingleton()->UnequipObject(plyr, GetBoundObject());
                RE::DebugNotification(std::format("My {} needs {} for fuel.", GetName(), GetFuelName()).c_str());
				logger::info("No fuel.");
			}
        }
	}

    void Init(){
        logger::info("Initializing LightSourceManager.");
        m_Data.clear();
        
        for (auto& src : sources) {
            src.remaining = 0.f;
            src.elapsed = 0.f;
            SetData(src.formid, src.remaining);
        }
        logger::info("setting current source to nullptr.");
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
        logger::info("Refueling.");
        current_source->remaining = current_source->duration;
        current_source->elapsed = 0.f;
    };

	void StartBurn() {
        logger::info("Starting to burn fuel.");
		Start(RE::Calendar::GetSingleton()->GetHoursPassed());
        is_burning = true;
        logger::info("Started to burn fuel.");
	};

    void PauseBurn() {
        logger::info("Pausing burning fuel.");
        Stop();
        current_source->remaining -= current_source->elapsed;
        current_source->elapsed = 0.f;
    };

	void StopBurn() {
        logger::info("Stopping burning fuel.");
        PauseBurn();
        is_burning = false;
        logger::info("setting current source to nullptr.");
        current_source = nullptr;
        logger::info("Stopped burning fuel.");
    };

	bool HasFuel(Settings::LightSource* src) { return GetRemaining(src) > 0.0001;};

	float GetRemaining(Settings::LightSource* src) { return src->remaining - src->elapsed; };

    bool IsValidSource(RE::FormID eqp_obj) {
        logger::info("Looking if valid source.");
        for (auto& src : sources) {
            if (eqp_obj == src.formid) return true;
        }
        return false;
    };

	bool SetSource(RE::FormID eqp_obj) {
        logger::info("Setting light source.");
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
        logger::info("Sending data.");
        for (auto& src : sources) {
            SetData(src.formid, src.remaining);
        }
    };

    void ReceiveData() {
        logger::info("Receiving data.");
        for (auto& src : sources) {
            for (auto& pair : m_Data) {
				if (pair.first == src.formid) {
                    src.remaining = std::min(pair.second,src.duration);
					break;
				}
			}
        }
        logger::info("Received data.");
    };

	void LogRemainings() {
        logger::info("Logging remaining hours...");
		for (auto& src : sources) {
			logger::info("Remaining hours for {}: {}", src.GetName(), src.remaining);
		}
	}
    
    void Reset(){
        logger::info("Resetting LightSourceManager.");
        Stop();
		Init();
    };
};
	