// https:  // github.com/ozooma10/OSLAroused-SKSE/blob/905d70b29875f89edfca9ede5a64c0cc126bd8fb/src/Utilities/Ticker.h
#pragma once

#include <functional>
#include <chrono>
#include "Settings.h"


namespace Utilities
{
	class Ticker
	{
	public:
        Ticker(std::function<void(float)> onTick, std::chrono::milliseconds interval)
            :
			m_OnTick(onTick),
			m_Interval(interval),
			m_Running(false),
			m_ThreadActive(false)
		{}

		void Start(float start_time)
		{
			if (m_Running) {
				return;
			}
			m_Running = true;
			//logger::info("Start Called with thread active state of: {}", m_ThreadActive);
			if (!m_ThreadActive) {
				std::thread tickerThread(&Ticker::RunLoop, this, start_time);
				tickerThread.detach();
			}
		}

		void Stop()
		{
			m_Running = false;
		}

		void UpdateInterval(std::chrono::milliseconds newInterval)
		{
			m_IntervalMutex.lock();
			m_Interval = newInterval;
			m_IntervalMutex.unlock();
		}

	private:
        void RunLoop(float start_t)
		{
			m_ThreadActive = true;
			while (m_Running) {
                std::thread runnerThread(m_OnTick, start_t);
				runnerThread.detach();

				m_IntervalMutex.lock();
				std::chrono::milliseconds interval = m_Interval;
				m_IntervalMutex.unlock();
				std::this_thread::sleep_for(interval);
			}
			m_ThreadActive = false;
		}

		std::function<void(float)> m_OnTick;
		std::chrono::milliseconds m_Interval;

		std::atomic<bool> m_ThreadActive;
		std::atomic<bool> m_Running;
		std::mutex m_IntervalMutex;
	};

}


class LightSourceManager : public Utilities::Ticker {
	void UpdateLoop(float start_h) {
		if (HasFuel(current_source)) {
            UpdateElapsed(start_h);
			logger::info("Remaining hours: {}", current_source->remaining - current_source->elapsed);
        } else NoFuel();
	};

	void UpdateElapsed(float start) { 
		current_source->elapsed = RE::Calendar::GetSingleton()->GetHoursPassed() - start;
	};

	void NoFuel(){
		auto plyr = RE::PlayerCharacter::GetSingleton();
        if (plyr) {
			auto fuel_item = GetBoundObject(current_source->fuel);
            if (plyr->GetItemCount(fuel_item) > 0) {
                plyr->RemoveItem(fuel_item, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				ReFuel();
				logger::info("Refueled.");
			} else {
				RE::ActorEquipManager::GetSingleton()->UnequipObject(plyr, GetBoundObject(current_source->formid));
                RE::DebugNotification(std::format("My {} needs {} for fuel.", GetName(current_source->formid), GetName(current_source->fuel)).c_str());
				logger::info("No fuel.");
			}
        }
	}

public:
    LightSourceManager(std::vector<Settings::LightSource>& data, std::chrono::milliseconds interval)
        : sources(data), Utilities::Ticker([this](float start_h) { UpdateLoop(start_h); }, interval){
			/*for (auto& src : sources) {
				logger::info("Name: {}, Fuel Name: {} duration {}", GetName(src.formid), GetName(src.fuel), src.duration);
			}*/
	};

    static LightSourceManager* GetSingleton(std::vector<Settings::LightSource>& data, int u_intervall) {
        static LightSourceManager singleton(data,std::chrono::milliseconds(u_intervall));
        return &singleton;
    }

	std::vector<Settings::LightSource> sources;
    Settings::LightSource* current_source = nullptr;

	void ReFuel() {
        current_source->remaining = current_source->duration;
        current_source->elapsed = 0.f;
    };

	void StartBurn() { 
		Start(RE::Calendar::GetSingleton()->GetHoursPassed());
        //logger::info("Started to burn fuel.");
	};

	void StopBurn() {
        Stop();
		current_source->remaining -= current_source->elapsed;
		current_source->elapsed = 0.f;
        //logger::info("Stopped burning fuel.");
    };

	bool HasFuel(Settings::LightSource* src) { return GetRemaining(src) > 0.0001;};

	float GetRemaining(Settings::LightSource* src) { return src->remaining - src->elapsed; };

	bool SetSource(RE::FormID eqp_obj) {
        for (auto& src : sources) {
            current_source = &src;
            if (eqp_obj == current_source->formid) return true;
        }
        current_source = nullptr;
		return false;
	};

	RE::TESBoundObject* GetBoundObject(RE::FormID fid) { return RE::TESForm::LookupByID(fid)->As<RE::TESBoundObject>(); }

    std::string_view GetName(RE::FormID fid) { return RE::TESForm::LookupByID(fid)->GetName(); }

	//void SaveState();

};