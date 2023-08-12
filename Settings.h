#pragma once
#include "SimpleIni.h"
#include "logger.h"

// keyboard-gp & mouse-gp
using KeyValuePair = std::pair<const char*, int>;

namespace Settings {
    
    constexpr auto path = L"Data/SKSE/Plugins/FuelableFramework.ini";
    constexpr auto path2 = L"Data/SKSE/SavedStates.txt";
    const char* comment = ";Make sure to use unique keys, e.g. Source1=000f11c0 Source2=05002301 ...";
    const char* default_duration = "8";
    bool verbose = false;
    
    struct LightSource {
		float duration;
        std::uint32_t fuel;
        std::uint32_t formid;
        LightSource(std::uint32_t formid, float duration, std::uint32_t fuel) : formid(formid), duration(duration), fuel(fuel){};

        float remaining = 0.f;
        float elapsed = 0.f;
        
        std::string_view GetName() { return RE::TESForm::LookupByID(formid)->GetName(); };
        std::string_view GetFuelName() { return RE::TESForm::LookupByID(fuel)->GetName(); };
        RE::TESBoundObject* GetBoundObject() { return RE::TESForm::LookupByID<RE::TESBoundObject>(formid);};
        RE::TESBoundObject* GetBoundFuelObject() { return RE::TESForm::LookupByID<RE::TESBoundObject>(fuel); };

	};


    std::vector<LightSource> LoadSettings() {
        CSimpleIniA ini;
        ini.SetUnicode();

        ini.LoadFile(path);

        CSimpleIniA::TNamesDepend ls_keys;
        CSimpleIniA::TNamesDepend light_sources;
        if (!ini.SectionExists("Light Sources")) {
            ini.SetValue("Light Sources", nullptr, nullptr);
            ini.SetValue("Light Sources", "Source1", nullptr,comment);
            ini.GetAllKeys("Light Sources", ls_keys);
        }
        else {
            ini.GetAllKeys("Light Sources", ls_keys);
            for (CSimpleIniA::TNamesDepend::const_iterator it = ls_keys.begin(); it != ls_keys.end(); ++it) {
				auto val = ini.GetValue("Light Sources", it->pItem);
                if (std::strlen(val))
                    light_sources.push_back(val);
                else continue;
                ini.SetValue("Light Sources", it->pItem, val);
            }
		}
       
        CSimpleIniA::TNamesDepend fuel_keys;
        CSimpleIniA::TNamesDepend fuel_sources;
        if (!ini.SectionExists("Fuel Sources")) {
			ini.SetValue("Fuel Sources", nullptr, nullptr);
            for (CSimpleIniA::TNamesDepend::const_iterator i = ls_keys.begin(); i != ls_keys.end(); ++i) {
				ini.SetValue("Fuel Sources", i->pItem, nullptr);
			}
		}
        else {
			ini.GetAllKeys("Fuel Sources", fuel_keys);
            for (CSimpleIniA::TNamesDepend::const_iterator it = fuel_keys.begin(); it != fuel_keys.end(); ++it) {
				auto val = ini.GetValue("Fuel Sources", it->pItem);
				if (std::strlen(val))
					fuel_sources.push_back(val);
				else continue;
				ini.SetValue("Fuel Sources", it->pItem, val);
			}
		}

        CSimpleIniA::TNamesDepend dur_keys;
        CSimpleIniA::TNamesDepend durations;
        if (!ini.SectionExists("Durations")) {
            ini.SetValue("Durations", nullptr, nullptr);
            for (CSimpleIniA::TNamesDepend::const_iterator i = ls_keys.begin(); i != ls_keys.end(); ++i) {
				ini.SetValue("Durations", i->pItem, default_duration);
                durations.push_back(default_duration);
            }
        }
        else {
			ini.GetAllKeys("Durations", dur_keys);
            for (CSimpleIniA::TNamesDepend::const_iterator it = dur_keys.begin(); it != dur_keys.end(); ++it) {
				auto val = ini.GetValue("Durations", it->pItem);
				if (std::strlen(val))
                    durations.push_back(val);
				else continue;
				ini.SetValue("Durations", it->pItem, val);
			}
		}

        if (light_sources.size() > durations.size()) {
            auto diff = light_sources.size() - durations.size();
            for (auto i = 0; i < diff; ++i) {
                ini.SetValue("Durations", std::format("Duration{}", durations.size() + i+1).c_str(), default_duration);
                durations.push_back(default_duration);
            }
		}

        if (!ini.SectionExists("Other Stuff")) {
            ini.SetValue("Other Stuff", nullptr, nullptr);
            ini.SetValue("Other Stuff", "In-game messages", verbose ? "true" : "false");
        } 
        else {
            verbose = ini.GetBoolValue("Other Stuff", "In-game messages", verbose);
        }


        ini.SaveFile(path);

        std::size_t numSources = std::min({light_sources.size(), durations.size(), fuel_sources.size()});
        std::vector<LightSource> lightSources;
        lightSources.reserve(numSources);
        int index = 0;
        for (auto it1 = light_sources.begin(), it2 = durations.begin(), it3 = fuel_sources.begin(); index<numSources; ++it1, ++it2, ++it3, ++index) {
            lightSources.emplace_back(static_cast<uint32_t>(std::strtoul(it1->pItem, nullptr, 16)), std::stof(it2->pItem),
                                      static_cast<uint32_t>(std::strtoul(it3->pItem, nullptr, 16)));
        }

        return lightSources;
    };

    void gameSavedHandler(SKSE::SerializationInterface* intfc) {
		logger::info("Saving cosave data...");
	};

    void gameLoadedHandler(SKSE::SerializationInterface* intfc) {
        logger::info("Loading cosave data...");
	};

    void initializeCosaves() {
        logger::info("Initializing cosave serialization...");
        auto* s_intfc = SKSE::GetSerializationInterface();
        s_intfc->SetUniqueID(_byteswap_ulong('FUEL'));
        s_intfc->SetSaveCallback(gameSavedHandler);
        // cosave->SetRevertCallback(cosave::revertHandler);
        s_intfc->SetLoadCallback(gameLoadedHandler);
    }





    
};