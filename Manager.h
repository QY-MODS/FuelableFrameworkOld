#pragma once

#include "Utils.h"
#include "Settings.h"


class LightSourceManager : public Utilities::Ticker, public Utilities::BaseFormFloat {

    RE::Calendar* cal;
    RE::UI* ui;
    bool __restart;
    float __start_h;

	void UpdateLoop() {
        if (__restart) {
            is_burning = true;
            __restart = false;
            __start_h = cal->GetHoursPassed();
        }
        logger::info("Updating LightSourceManager.");
        logger::info("start_h: {}", __start_h);
        logger::info("hours passed: {}", cal->GetHoursPassed());
        if (ui->GameIsPaused()) return;
		if (HasFuel(current_source)) {
            current_source->elapsed = cal->GetHoursPassed() - __start_h;
            logger::info("elapsed: {}", current_source->elapsed);
            logger::info("Remaining hours: {} (remaining: {}) (elapsed: {})", current_source->remaining - current_source->elapsed, current_source->remaining,
                         current_source->elapsed);
        } else NoFuel();
	};


	void NoFuel(){
		logger::info("No fuel.");
		auto plyr = RE::PlayerCharacter::GetSingleton();
        if (plyr) {
			auto fuel_item = GetBoundFuelObject();
            if (plyr->GetItemCount(fuel_item) > 0) {
                logger::info("Refueling.");
                if (Settings::enabled_plyrmsg) Utilities::MsgBoxesNotifs::InGame::Refuel(GetName(), GetFuelName());
                PauseBurn();
                plyr->RemoveItem(fuel_item, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr); // Refuel
                current_source->remaining += current_source->duration; // Refuel
				logger::info("Refueled.");
                Start();
			} else {
                Stop(); // better safe than sorry?
                if (Settings::enabled_plyrmsg) Utilities::MsgBoxesNotifs::InGame::NoFuel(GetName(), GetFuelName());
                // This has to be the last thing that happens in this function, which involves current_source because current_source is set to nullptr in the main thread
				RE::ActorEquipManager::GetSingleton()->UnequipObject(plyr, GetBoundObject(),nullptr,1,nullptr,true,false,false);
				logger::info("No fuel.");
			}
        }
	}

    float GetRemaining(Settings::LightSource* src) { return src->remaining - src->elapsed; };
    bool HasFuel(Settings::LightSource* src) { return GetRemaining(src) > 0.0001; };



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
        logger::info("setting current source to nullptr (Init).");
        current_source = nullptr;
        is_burning = false;
        //allow_equip_event_sink = true;
        
        __restart = true;
        cal = RE::Calendar::GetSingleton();
        ui = RE::UI::GetSingleton();

        logger::info("LightSourceManager initialized.");
    };


public:
    LightSourceManager(std::vector<Settings::LightSource>& data, std::chrono::milliseconds interval)
        : sources(data), Utilities::Ticker([this]() { UpdateLoop(); }, interval) {Init();};

    static LightSourceManager* GetSingleton(std::vector<Settings::LightSource>& data, int u_intervall) {
        static LightSourceManager singleton(data,std::chrono::milliseconds(u_intervall));
        return &singleton;
    }
    
	std::vector<Settings::LightSource> sources;
    Settings::LightSource* current_source;
    bool is_burning;
    //bool allow_equip_event_sink;


	void StartBurn() {
        if (is_burning) {
            logger::info("Already burning.");
            return;
        }
        logger::info("Starting to burn fuel.");
        if (!current_source) {
			logger::error("No current source!!No current source!!No current source!!No current source!!");
            Utilities::MsgBoxesNotifs::Windows::GeneralErr();
			return;
		}
        logger::info("Started to burn fuel.");
        ShowRemaining();
        __restart = true;
		Start();
	};

    void PauseBurn() {
        logger::info("Pausing burning fuel.");
        Stop();
        current_source->remaining -= current_source->elapsed;
        current_source->elapsed = 0.f;
        __restart = true;
        logger::info("Paused burning fuel.");
    };

    void UnPauseBurn() {
		logger::info("Unpausing burning fuel.");
		Start();
		logger::info("Unpaused burning fuel.");
	};

	void StopBurn() {
        logger::info("Stopping burning fuel.");
        PauseBurn();
        is_burning = false; 
        logger::info("setting current source to nullptr (StopBurn).");
        current_source = nullptr;
        logger::info("Stopped burning fuel.");
    };


    bool IsValidSource(RE::FormID eqp_obj) {
        logger::info("Looking if valid source.");
        for (auto& src : sources) {
            logger::info("in for loop.");
            logger::info("src.formid: {}", src.formid);
            if (eqp_obj == src.formid) {
                logger::info("Found a match.");
                return true;
            }
        }
        logger::info("Did not find a match.");
        return false;
    };

    bool IsCurrentSource(RE::FormID eqp_obj) {
		logger::info("Looking if eqp_obj is current source.");
        if (!current_source) {
            logger::info("No current source.");
            return false;
        }
		return eqp_obj == current_source->formid;
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

    void ShowRemaining() {
        if (!Settings::enabled_remainingmsg) return;
        int _remaining = Utilities::Round2Int(current_source->remaining);
        Utilities::MsgBoxesNotifs::InGame::Remaining(_remaining, GetName());
    }
    
    void Reset(){
        logger::info("Resetting LightSourceManager.");
        Stop();
		Init();
    };
};
	