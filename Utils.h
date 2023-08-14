
#pragma once

#include <functional>
#include <chrono>
#include "logger.h"
#include <windows.h>

namespace Utilities {

    const auto mod_name = static_cast<std::string>(SKSE::PluginDeclaration::GetSingleton()->GetName());
    constexpr auto path = L"Data/SKSE/Plugins/FuelableFramework.ini";
    constexpr auto po3path = "Data/SKSE/Plugins/po3_Tweaks.dll";

    const auto no_src_msgbox = std::format("{}: You currently do not have any sources set up. Check your ini file or see the mod page for instructions.", mod_name);
    const auto po3_err_msgbox = std::format(
        "{}: You have given an invalid FormID. If you are using Editor IDs, you must have powerofthree's Tweaks installed. See mod page for further instructions.", mod_name);
    const auto general_err_msgbox = std::format("{}: Something went wrong. Please contact the mod author.", mod_name);
    const auto init_err_msgbox = std::format("{}: The mod failed to initialize and will be terminated.", mod_name);

    std::string DecodeTypeCode(std::uint32_t typeCode) {
        char buf[4];
        buf[3] = char(typeCode);
        buf[2] = char(typeCode >> 8);
        buf[1] = char(typeCode >> 16);
        buf[0] = char(typeCode >> 24);
        return std::string(buf, buf + 4);
    }

    std::string dec2hex(int dec) {
        std::stringstream stream;
        stream << std::hex << dec;
        std::string hexString = stream.str();
        return hexString;
    };

    bool isValidHexWithLength7or8(const char* input) {
        std::string inputStr(input);
        std::regex hexRegex("^[0-9A-Fa-f]{7,8}$");  // Allow 7 to 8 characters
        bool isValid = std::regex_match(inputStr, hexRegex);
        return isValid;
    }

    float Round(float number, int decimalPlaces) { 
        auto rounded_number = static_cast<float>(std::round(number * std::pow(10, decimalPlaces))) / std::pow(10, decimalPlaces);
        return rounded_number;
    }

    bool IsPo3Installed() { return std::filesystem::exists(po3path); };
    
    namespace MsgBoxesNotifs {

        namespace Windows {
            
            int Po3ErrMsg() {
            MessageBoxA(nullptr,po3_err_msgbox.c_str(),"Error", MB_OK | MB_ICONERROR);
            return 1;
            };

            int GeneralErr(){
                MessageBoxA(nullptr, general_err_msgbox.c_str(), "Error", MB_OK | MB_ICONERROR);
                return 1;
            };
        };

        namespace InGame {

            void InitErr() { RE::DebugMessageBox(init_err_msgbox.c_str()); };

            void GeneralErr() {
                RE::DebugMessageBox(general_err_msgbox.c_str());
            };

            void NoSourceFound() { RE::DebugMessageBox(no_src_msgbox.c_str()); };

            void FormIDError(RE::FormID id) {
                RE::DebugMessageBox(
                    std::format("{}: The ID ({}) you have provided in the ini file could not have been found.", Utilities::mod_name, Utilities::dec2hex(id)).c_str());
            }

            void EditorIDError(std::string id) {
                RE::DebugMessageBox(
                    std::format("{}: The ID ({}) you have provided in the ini file could not have been found.", Utilities::mod_name, id).c_str());
            }

            void Refuel(std::string_view item, std::string_view fuel) {
                RE::DebugNotification(std::format("Adding {} to my {}...", fuel, item).c_str());
            }
            void NoFuel(std::string_view item, std::string_view fuel) {
                RE::DebugNotification(std::format("I need {} to fuel my {}.", fuel, item).c_str());
            }

            void Remaining(int remaining, std::string_view item) {
                if (remaining>1) RE::DebugNotification(std::format("I have about {} hours left in my {}.", remaining, item).c_str());
                if (remaining > 0.5) RE::DebugNotification(std::format("I have about an hour left in my {}.", item).c_str());
                else RE::DebugNotification(std::format("I don't have much left in my {}...", item).c_str());
            }
        };
    };


    // https:  // github.com/ozooma10/OSLAroused-SKSE/blob/905d70b29875f89edfca9ede5a64c0cc126bd8fb/src/Utilities/Ticker.h
    class Ticker {
    public:
        Ticker(std::function<void(float)> onTick, std::chrono::milliseconds interval) : m_OnTick(onTick), m_Interval(interval), m_Running(false), m_ThreadActive(false) {}

        void Start(float start_time) {
            if (m_Running) {
                return;
            }
            m_Running = true;
            // logger::info("Start Called with thread active state of: {}", m_ThreadActive);
            if (!m_ThreadActive) {
                std::thread tickerThread(&Ticker::RunLoop, this, start_time);
                tickerThread.detach();
            }
        }

        void Stop() { m_Running = false; }

        void UpdateInterval(std::chrono::milliseconds newInterval) {
            m_IntervalMutex.lock();
            m_Interval = newInterval;
            m_IntervalMutex.unlock();
        }

    private:
        void RunLoop(float start_t) {
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

    // https :  // github.com/ozooma10/OSLAroused/blob/29ac62f220fadc63c829f6933e04be429d4f96b0/src/PersistedData.cpp
    template <typename T>
    // BaseData is based off how powerof3's did it in Afterlife
    class BaseData {
    public:
        float GetData(RE::FormID formId, T missing) {
            Locker locker(m_Lock);
            if (auto idx = m_Data.find(formId) != m_Data.end()) {
                return m_Data[formId];
            }
            return missing;
        }

        void SetData(RE::FormID formId, T value) {
            Locker locker(m_Lock);
            m_Data[formId] = value;
        }

        virtual const char* GetType() = 0;

        virtual bool Save(SKSE::SerializationInterface* serializationInterface, std::uint32_t type, std::uint32_t version);
        virtual bool Save(SKSE::SerializationInterface* serializationInterface);
        virtual bool Load(SKSE::SerializationInterface* serializationInterface);

        void Clear();

        virtual void DumpToLog() = 0;

    protected:
        std::map<RE::FormID, T> m_Data;

        using Lock = std::recursive_mutex;
        using Locker = std::lock_guard<Lock>;
        mutable Lock m_Lock;
    };



    class BaseFormFloat : public BaseData<float> {
    public:
        virtual void DumpToLog() override {
            Locker locker(m_Lock);
            for (const auto& [formId, value] : m_Data) {
                logger::info("Dump Row From {} - FormID: {} - value: {}", GetType(), formId, value);
            }
            logger::info("{} Rows Dumped For Type {}", m_Data.size(), GetType());
        }
    };



    // BaseData is based off how powerof3's did it in Afterlife
    template <typename T>
    bool BaseData<T>::Save(SKSE::SerializationInterface* serializationInterface, std::uint32_t type, std::uint32_t version) {
        if (!serializationInterface->OpenRecord(type, version)) {
            logger::error("Failed to open record for Data Serialization!");
            return false;
        }

        return Save(serializationInterface);
    }

    template <typename T>
    bool BaseData<T>::Save(SKSE::SerializationInterface* serializationInterface) {
        assert(serializationInterface);
        Locker locker(m_Lock);

        const auto numRecords = m_Data.size();
        if (!serializationInterface->WriteRecordData(numRecords)) {
            logger::error("Failed to save {} data records", numRecords);
            return false;
        }

        for (const auto& [formId, value] : m_Data) {
            if (!serializationInterface->WriteRecordData(formId)) {
                logger::error("Failed to save data for FormID: ({:X})", formId);
                return false;
            }

            if (!serializationInterface->WriteRecordData(value)) {
                logger::error("Failed to save value data for form: {}", formId);
                return false;
            }
        }
        return true;
    }

    template <typename T>
    bool BaseData<T>::Load(SKSE::SerializationInterface* serializationInterface) {
        assert(serializationInterface);

        std::size_t recordDataSize;
        serializationInterface->ReadRecordData(recordDataSize);

        Locker locker(m_Lock);
        m_Data.clear();

        RE::FormID formId;
        T value;

        for (auto i = 0; i < recordDataSize; i++) {
            serializationInterface->ReadRecordData(formId);
            // Ensure form still exists
            RE::FormID fixedId;
            if (!serializationInterface->ResolveFormID(formId, fixedId)) {
                logger::error("Failed to resolve formID {} {}"sv, formId, fixedId);
                continue;
            }

            serializationInterface->ReadRecordData(value);
            m_Data[formId] = value;
        }
        return true;
    }

    template <typename T>
    void BaseData<T>::Clear() {
        Locker locker(m_Lock);
        m_Data.clear();
    }




};
