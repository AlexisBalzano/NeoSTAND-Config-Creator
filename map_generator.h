#pragma once
#include <string>
#include "nlohmann/json.hpp"

void generateMap(const nlohmann::ordered_json &configJson, const std::string &icao, bool openBrowser = true);
