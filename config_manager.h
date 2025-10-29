#pragma once
#include <string>
#include "nlohmann/json.hpp"

bool getConfig(const std::string &icao, nlohmann::ordered_json &configJson, bool& mapGenerated);
void saveFile(const std::string &icao, const nlohmann::ordered_json &configJson);
