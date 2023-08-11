// https:  // github.com/ozooma10/OSLAroused-SKSE/blob/905d70b29875f89edfca9ede5a64c0cc126bd8fb/src/Utilities/Ticker.h
#pragma once

#include <functional>
#include <chrono>


namespace Utilities
{
	class Ticker
	{
	public:
        Ticker(std::function<void(float, RE::EffectSetting*)> onTick, std::chrono::milliseconds interval)
            :
			m_OnTick(onTick),
			m_Interval(interval),
			m_Running(false),
			m_ThreadActive(false)
		{}

		void Start(float start_time, RE::EffectSetting* me)
		{
			if (m_Running) {
				return;
			}
			m_Running = true;
			//logger::info("Start Called with thread active state of: {}", m_ThreadActive);
			if (!m_ThreadActive) {
				std::thread tickerThread(&Ticker::RunLoop, this, start_time, me);
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
        void RunLoop(float start_t, RE::EffectSetting* magic_e)
		{
			m_ThreadActive = true;
			while (m_Running) {
                std::thread runnerThread(m_OnTick, start_t, magic_e);
				runnerThread.detach();

				m_IntervalMutex.lock();
				std::chrono::milliseconds interval = m_Interval;
				m_IntervalMutex.unlock();
				std::this_thread::sleep_for(interval);
			}
			m_ThreadActive = false;
		}

		std::function<void(float, RE::EffectSetting*)> m_OnTick;
		std::chrono::milliseconds m_Interval;

		std::atomic<bool> m_ThreadActive;
		std::atomic<bool> m_Running;
		std::mutex m_IntervalMutex;
	};

}

namespace WorldChecks {

    void UpdateLoop(float start_h, RE::EffectSetting* me);

    class UpdateTicker : public Utilities::Ticker {
    public:
        UpdateTicker(std::chrono::milliseconds interval) : Utilities::Ticker(std::function<void(float, RE::EffectSetting*)>(UpdateLoop), interval) {}

        static UpdateTicker* GetSingleton() {
            static UpdateTicker singleton(std::chrono::milliseconds(5000));
            return &singleton;
        }

        float LastUpdatePollGameTime = RE::Calendar::GetSingleton()->GetHoursPassed();
    };
}