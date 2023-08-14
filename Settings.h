#pragma once
#include "SimpleIni.h"


namespace Settings {
    
    bool po3installed = false;

    constexpr std::array<const char*, 4> InISections = {"Light Sources", "Fuel Sources", "Durations", "Other Stuff"};
    constexpr std::array<const char*, 4> InIDefaultKeys = {"src1", "src1", "src1"};

    const std::array<std::string, 4> section_comments = {
        ";Make sure to use unique keys, e.g. src1=... NOTsrc1=...", 
        std::format(";Make sure to use matching keys with the ones provided in section {}.", static_cast<std::string>(InISections[0])),
        std::format(";Make sure to use matching keys with the ones provided in sections {} and {}.", static_cast<std::string>(InISections[0]),static_cast<std::string>(InISections[1])),
        ";Set boolean values in this section, i.e. true or false."};

    
    bool force_editor_id = false;
    bool enabled_plyrmsg = true;
    bool enabled_remainingmsg = true;
    bool enabled_err_msgbox = true;
    
    // DO NOT CHANGE THE ORDER OF THESE
    constexpr std::array<const char*, 4> OtherStuffDefKeys = {"ForceEditorID", "DepleteReplenishMessages","RemainingMessages", "ErrMsgBox"};
    const std::map<const char*, std::map<bool, const char*>> other_stuff_defaults = {
        {OtherStuffDefKeys[0], {{force_editor_id, ";Set to true if you ONLY use EditorIDs and NO FormIDs AND you have powerofthree's Tweaks installed, otherwise false."}}},
        {OtherStuffDefKeys[1], {{enabled_plyrmsg, ";These are the messages that will be displayed when the player's fuel depletes."}}},
        {OtherStuffDefKeys[2], {{enabled_remainingmsg, ";These are the messages that will be displayed when the player puts away the source."}}},
        {OtherStuffDefKeys[3], {{enabled_err_msgbox, ";Pop-up error messages. Do not set to false, unless you have to..."}}}
    };

    const char* default_duration = "8";
    
    constexpr std::uint32_t kSerializationVersion = 1;
    constexpr std::uint32_t kDataKey = 'FUEL';


    struct LightSource {

		const float duration;
        std::uint32_t formid;
        const std::string editorid;
        std::uint32_t fuel;
        const std::string fuel_editorid;


        LightSource(std::uint32_t id, const std::string id_str, float duration, std::uint32_t fuelid, const std::string fuelid_str)
            : formid(id), editorid(id_str), duration(duration), fuel(fuelid), fuel_editorid(fuelid_str) {
            if (!formid) {
                auto form = RE::TESForm::LookupByEditorID<RE::TESForm>(editorid);
                if (form) {
                    logger::info("Found formid for editorid {}", editorid);
                    formid = form->GetFormID();
                } else logger::info("Could not find formid for editorid {}", editorid);
            }
            if (!fuel) {
                auto form = RE::TESForm::LookupByEditorID<RE::TESForm>(fuel_editorid);
                if (form) {
                    logger::info("Found formid2 for editorid2 {}", editorid);
                    fuel = form->GetFormID();
                } else logger::info("Could not find formid for editorid {}", editorid);
            }
        }

        float remaining = 0.f;
        float elapsed = 0.f;


		/*RE::TESForm* GetID(){
            auto form = RE::TESForm::LookupByEditorID<RE::TESForm>(editorid);
        };*/

       RE::TESForm* GetForm(){
       	 auto form = RE::TESForm::LookupByID(formid);
			if (form) return form;
            else {
                if (editorid.empty()) Utilities::MsgBoxesNotifs::InGame::FormIDError(formid);
                else Utilities::MsgBoxesNotifs::InGame::EditorIDError(editorid);
            }
            return nullptr;
       };

       RE::TESForm* GetFormFuel() {
            auto form = RE::TESForm::LookupByID(fuel);
            if (form)
                return form;
            else {
                if (editorid.empty())
                    Utilities::MsgBoxesNotifs::InGame::FormIDError(fuel);
                else
                    Utilities::MsgBoxesNotifs::InGame::EditorIDError(fuel_editorid);
            }
            return nullptr;
       };
       
        std::string_view GetName() { 
            auto form = GetForm();
            if (form) return form->GetName(); else return "";
        };

        std::string_view GetFuelName() {
            auto form = GetFormFuel();
            if (form) return form->GetName(); else return "";
        };

        RE::TESBoundObject* GetBoundObject() { return RE::TESForm::LookupByID<RE::TESBoundObject>(formid);};
        RE::TESBoundObject* GetBoundFuelObject() { return RE::TESForm::LookupByID<RE::TESBoundObject>(fuel); };

	};
    

    std::vector<LightSource> LoadINISettings() {
        
        logger::info("Loading ini settings");
        if (Utilities::IsPo3Installed()) {
			logger::info("powerofthree's Tweaks is installed. Enabling EditorID support.");
            po3installed = true;
		} else {
			logger::info("powerofthree's Tweaks is not installed. Disabling EditorID support.");
            po3installed = false;
		}

        CSimpleIniA ini;
        CSimpleIniA::TNamesDepend source_names;
        std::vector<LightSource> lightSources;

        ini.SetUnicode();
        ini.LoadFile(Utilities::path);

        // Create Sections with defaults if they don't exist
        for (int i = 0; i < InISections.size(); ++i) {
            logger::info("Checking section {}", InISections[i]);
            if (!ini.SectionExists(InISections[i])) {
                logger::info("Section {} does not exist. Creating it.", InISections[i]);
                ini.SetValue(InISections[i], nullptr, nullptr);
                logger::info("Setting default keys for section {}", InISections[i]);
                if (i < 3) {
                    ini.SetValue(InISections[i], InIDefaultKeys[i], nullptr, section_comments[i].c_str());
                    logger::info("Default values set for section {}", InISections[i]);
                }
				else {
                    logger::info("Creating Other Stuff section");
                    for (auto it = other_stuff_defaults.begin(); it != other_stuff_defaults.end(); ++it) {
                        auto it2 = it->second.begin();
                        ini.SetBoolValue(InISections[i], it->first, it2->first, it2->second);
                    }
                }
			}
		}

        // Sections: Other stuff
        // get from user
        CSimpleIniA::TNamesDepend other_stuff_userkeys;
        ini.GetAllKeys(InISections[3], other_stuff_userkeys);
        for (CSimpleIniA::TNamesDepend::const_iterator it = other_stuff_userkeys.begin(); it != other_stuff_userkeys.end(); ++it) {
            for (auto it2 = other_stuff_defaults.begin(); it2 != other_stuff_defaults.end(); ++it2) {
                if (static_cast<std::string_view>(it->pItem) == static_cast<std::string_view>(it2->first)) {
                    logger::info("Found key: {}", it->pItem);
                    auto it3 = it2->second.begin();
                    auto val = ini.GetBoolValue(InISections[3], it->pItem, it3->first);
                    ini.SetBoolValue(InISections[3], it->pItem, val, it3->second);
                    break;
                }
            }
        }

        // set stuff which is not found
        for (auto it = other_stuff_defaults.begin(); it != other_stuff_defaults.end(); ++it) {
            if (ini.KeyExists(InISections[3], it->first)) continue;
            auto it2 = it->second.begin();
            ini.SetBoolValue(InISections[3], it->first, it2->first, it2->second);
        }

        force_editor_id = ini.GetBoolValue(InISections[3], OtherStuffDefKeys[0]);   // logger::info("force_editor_id: {}", force_editor_id);
        enabled_plyrmsg = ini.GetBoolValue(InISections[3], OtherStuffDefKeys[1]);     // logger::info("enabled_plyrmsg: {}", enabled_plyrmsg);
        enabled_remainingmsg = ini.GetBoolValue(InISections[3], OtherStuffDefKeys[2]);  // logger::info("enabled_remainingmsg: {}", enabled_remainingmsg);
        enabled_err_msgbox = ini.GetBoolValue(InISections[3], OtherStuffDefKeys[3]);  // logger::info("enabled_err_msgbox: {}", enabled_err_msgbox);

        // Sections: Light Sources, Fuel Sources, Durations
        ini.GetAllKeys(InISections[0], source_names);
        auto numSources = source_names.size();
        logger::info("source_names size {}", numSources);

        lightSources.reserve(numSources);

        for (CSimpleIniA::TNamesDepend::const_iterator it = source_names.begin(); it != source_names.end(); ++it) {
            logger::info("source name {}", it->pItem);
            const char* val1 = ini.GetValue(InISections[0], it->pItem);
            const char* val2 = ini.GetValue(InISections[1], it->pItem);
            const char* val3 = ini.GetValue(InISections[2], it->pItem);
            if (!val1 || !val2) {
                logger::warn("Source {} is missing a value. Skipping.", it->pItem);
                continue;
            } else logger::info("Source {} has a value of {}", it->pItem, val1);
            if (std::strlen(val1) && std::strlen(val2)) {
                logger::info("We have valid entries for source: {} and fuel: {}", val1, val2);
                // Duration of the source
                float duration;
                if (!val3 || !std::strlen(val3)) {
                    logger::warn("Source {} is missing a duration value. Using default value of {}.", it->pItem, default_duration);
                    duration = std::stof(default_duration);
                    ini.SetValue(InISections[2], it->pItem, default_duration);
                } 
                else {
                    logger::info("Source {} has a duration value of {}.", it->pItem, val3);
                    duration = std::stof(val3);
                    ini.SetValue(InISections[2], it->pItem, val3);
                }
                // back to id and fuelid
                auto id = static_cast<uint32_t>(std::strtoul(val1, nullptr, 16));
                auto id_str = static_cast<std::string>(val1);
                auto fuelid = static_cast<uint32_t>(std::strtoul(val2, nullptr, 16));
                auto fuelid_str = static_cast<std::string>(val2);
                // Our job is easy if the user wants to force editor ids
                if (force_editor_id && po3installed) lightSources.emplace_back(0, id_str, duration, 0, fuelid_str);
                // if both formids are valid hex, use them
                else if (Utilities::isValidHexWithLength7or8(val1) && Utilities::isValidHexWithLength7or8(val2)) lightSources.emplace_back(id, "", duration, fuelid, "");
                // one of them is not formid so if powerofthree's Tweaks is not installed we have problem
                else if (!po3installed) {
                    Utilities::MsgBoxesNotifs::Windows::Po3ErrMsg();
                    return lightSources;
                }
                // below editor id is allowed
                // if the source is hex, fuel isnt vice versa
                else if (Utilities::isValidHexWithLength7or8(val1)) lightSources.emplace_back(id, "", duration, 0, fuelid_str);
                else if (Utilities::isValidHexWithLength7or8(val2)) lightSources.emplace_back(0, id_str, duration, fuelid, "");
                // both are not hex,
                else lightSources.emplace_back(0, id_str, duration, 0, fuelid_str);

                ini.SetValue(InISections[0], it->pItem, val1);
                ini.SetValue(InISections[1], it->pItem, val2);
                logger::info("Loaded source: {} with duration: {} and fuel: {}", val1, duration, val2);
            }
        }

        ini.SaveFile(Utilities::path);

        return lightSources;
    }


};