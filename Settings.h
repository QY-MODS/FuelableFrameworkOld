#pragma once
#include "SimpleIni.h"


namespace Settings {
    
    const auto mod_name = static_cast<std::string>(SKSE::PluginDeclaration::GetSingleton()->GetName());
    constexpr auto path = L"Data/SKSE/Plugins/FuelableFramework.ini";
    constexpr auto path2 = L"Data/SKSE/SavedStates.txt";
    const char* comment = ";Make sure to use unique keys, e.g. Source1=000f11c0 Source2=05002301 ...";
    const char* default_duration = "8";
    bool verbose = false;
    auto no_src_msgbox = std::format("{}: You currently do not have any sources set up in the ini file. See mod page for instructions.", mod_name);
    constexpr std::uint32_t kSerializationVersion = 1;
    constexpr std::uint32_t kDataKey = 'FUEL';
    constexpr std::array<const char*, 4> InISections = {"Light Sources", "Fuel Sources", "Durations", "Other Stuff"};
    constexpr std::array<const char*, 4> InIDefaultKeys = {"src1", "src1", "src1", "PlayerSelfMessages"};

    std::string dec2hex(int dec) {
        std::stringstream stream;
        stream << std::hex << dec;
        std::string hexString = stream.str();
        return hexString;
    };

    void PromptIDErrorMsgBox(RE::FormID id) {
        logger::info("{}: The ID ({}) you have provided in the ini file could not have been found.", mod_name, dec2hex(id));
        RE::DebugMessageBox(std::format("{}: The ID ({}) you have provided in the ini file could not have been found.", mod_name, dec2hex(id)).c_str());
    }

    struct LightSource {

		float duration;
        std::uint32_t fuel;
        std::uint32_t formid;
        LightSource(std::uint32_t formid, float duration, std::uint32_t fuel) : formid(formid), duration(duration), fuel(fuel){};

        float remaining = 0.f;
        float elapsed = 0.f;


       RE::TESForm* GetForm(RE::FormID id){
       	 auto form = RE::TESForm::LookupByID(id);
			if (form) return form;
            else PromptIDErrorMsgBox(id);return nullptr;
       };
       
        std::string_view GetName() { 
            auto form = GetForm(formid);
            if (form) return form->GetName(); else return "";
        };

        std::string_view GetFuelName() {
            auto form = GetForm(fuel);
            if (form) return form->GetName(); else return "";
        };

        RE::TESBoundObject* GetBoundObject() { return RE::TESForm::LookupByID<RE::TESBoundObject>(formid);};
        RE::TESBoundObject* GetBoundFuelObject() { return RE::TESForm::LookupByID<RE::TESBoundObject>(fuel); };



	};
    

    std::vector<LightSource> LoadINISettings() {
        
        logger::info("Loading ini settings");

        const char* firstSection = InISections[0];
        CSimpleIniA ini;
        CSimpleIniA::TNamesDepend source_names;

        ini.SetUnicode();
        ini.LoadFile(path);

        if (!ini.SectionExists(firstSection)) {
            ini.SetValue(firstSection, nullptr, nullptr);
            ini.SetValue(firstSection, InIDefaultKeys[0], nullptr, comment);
            ini.GetAllKeys(firstSection, source_names);
        } else ini.GetAllKeys(firstSection, source_names);

        auto numSources = source_names.size();
        logger::info("source_names size {}", numSources);

        std::vector<LightSource> lightSources;
        lightSources.reserve(numSources);

        for (CSimpleIniA::TNamesDepend::const_iterator it = source_names.begin(); it != source_names.end(); ++it) {
            logger::info("source name {}", it->pItem);
            const char* val1 = ini.GetValue(InISections[0], it->pItem);
            const char* val2 = ini.GetValue(InISections[1], it->pItem);
            const char* val3 = ini.GetValue(InISections[2], it->pItem);
            if (!val1 || !val2) {
                logger::warn("Source {} is missing a value. Skipping.", it->pItem);
                continue;
            }
            if (std::strlen(val1) && std::strlen(val2)) {
                logger::info("We have valid entries for source: {} and fuel: {}", val1, val2);
                auto formid = static_cast<uint32_t>(std::strtoul(val1, nullptr, 16));
                auto fuel = static_cast<uint32_t>(std::strtoul(val2, nullptr, 16));
                ini.SetValue(InISections[0], it->pItem, val1);
                ini.SetValue(InISections[1], it->pItem, val2);
                float duration;
                if (!val3 || !std::strlen(val3)) {
                    logger::warn("Source {} is missing a duration value. Using default value of {}.", it->pItem, default_duration);
                    duration = std::stof(default_duration);
                    ini.SetValue(InISections[2], it->pItem, default_duration);
                } else {
                    logger::info("Source {} has a duration value of {}.", it->pItem, val3);
                    duration = std::stof(val3);
                    ini.SetValue(InISections[2], it->pItem, val3);
                }
                logger::info("Loaded source: {} with duration: {} and fuel: {}", formid, duration, fuel);
                lightSources.emplace_back(formid, duration, fuel);
            }
        }
        const char* section = InISections[3];
        if (!ini.SectionExists(section)) {
            ini.SetValue(section, nullptr, nullptr);
            ini.SetValue(section, InIDefaultKeys[3], verbose ? "true" : "false");
        } else {
            verbose = ini.GetBoolValue(section, InIDefaultKeys[3], verbose);
            ini.SetBoolValue(section, InIDefaultKeys[3], verbose);
        }

        ini.SaveFile(path);

        return lightSources;
    }


};