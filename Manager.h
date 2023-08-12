#pragma once

#include "Utils.h"
#include "Settings.h"


class LightSourceManager : public Utilities::Ticker, public Utilities::BaseFormFloat {

	void UpdateLoop(float start_h) {
        logger::info("Updating LightSourceManager.");
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
        current_source = nullptr;
        is_burning = false;
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
        current_source = nullptr;
        logger::info("Stopped burning fuel.");
    };

	bool HasFuel(Settings::LightSource* src) { return GetRemaining(src) > 0.0001;};

	float GetRemaining(Settings::LightSource* src) { return src->remaining - src->elapsed; };

	bool SetSource(RE::FormID eqp_obj) {
        logger::info("Setting light source.");
        for (auto& src : sources) {
            current_source = &src;
            if (eqp_obj == current_source->formid) return true;
        }
        current_source = nullptr;
		return false;
	};

    std::string_view GetName() { return current_source->GetName(); };
    std::string_view GetFuelName() { return current_source->GetFuelName(); };
    RE::TESBoundObject* GetBoundObject() { return current_source->GetBoundObject(); };
    RE::TESBoundObject* GetBoundFuelObject() { return current_source->GetBoundFuelObject(); }

    void DetectSetSource() {
        logger::info("Detecting and setting light source.");
        if (current_source) StopBurn();
        else Stop();
        auto plyr = RE::PlayerCharacter::GetSingleton();
        for (auto& source : sources) {
            auto obj = source.GetBoundObject();
            if (RE::ActorEquipManager::GetSingleton()->UnequipObject(plyr, obj)) {
                RE::ActorEquipManager::GetSingleton()->EquipObject(plyr, obj);
                current_source = &source;
                StartBurn();
                break;
			}
		}
    };

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
					src.remaining = pair.second;
					break;
				}
			}
        }
    };

	void LogRemainings() {
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
	