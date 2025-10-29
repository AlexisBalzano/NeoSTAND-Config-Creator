#pragma once
#include "nlohmann/json.hpp"
#include <string>

void printMenu();
void printStandInfo(const nlohmann::ordered_json &standJson);
void listAllStands(const nlohmann::ordered_json &configJson);
void addStand(nlohmann::ordered_json &configJson, const std::string &standName);
void removeStand(nlohmann::ordered_json &configJson, const std::string &standName);
void editStand(nlohmann::ordered_json &configJson, const std::string &standName);
void editStandRadius(nlohmann::ordered_json &configJson, const std::string &standName);
void copyStand(nlohmann::ordered_json &configJson, const std::string &standName);
void batchcopy(nlohmann::ordered_json &configJson, const std::string &standName);
void softStandCopy(nlohmann::ordered_json &configJson, const std::string &standName);
