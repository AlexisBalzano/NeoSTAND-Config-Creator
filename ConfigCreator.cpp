#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <algorithm>
#include "nlohmann/json.hpp"

constexpr auto version = "v1.0.0";
constexpr auto baseDir = "./configs/";

bool getConfig(const std::string& icao, nlohmann::ordered_json& configJson) {
     bool fileExists = std::filesystem::exists(baseDir + icao + ".json");
    if (!fileExists) {
        std::cout << "Config file not found, creating a new one." << std::endl;
    } else {
        std::cout << "Config file found: " << icao << ".json" << std::endl;
        std::ifstream inputFile(baseDir + icao + ".json");
        if (inputFile) {
            try {
                inputFile >> configJson;
            } catch (const std::exception& e) {
                std::cerr << "Error reading JSON: " << e.what() << std::endl;
                return false;
            }
        } else {
            std::cerr << "Error opening file for reading." << std::endl;
            return false;
        }
    }

    if (configJson.empty()) {
        // Create default config
        configJson = {
            {"VERSION", "1.0.0"},
            {"ICAO", icao},
            {"STAND", {}}
        };
        std::cout << "Created default config structure." << std::endl;
    }

    return true;
}

void saveFile(const std::string& icao, const nlohmann::ordered_json& configJson) {
    std::ofstream outputFile(baseDir + icao + ".json");
    if (outputFile) {
        outputFile << configJson.dump(4);
        std::cout << "Config file saved: " << icao << ".json" << std::endl;
        outputFile.close();
    } else {
        std::cerr << "Error opening file for writing." << std::endl;
        return;
    }
}

void printMenu() {
    std::cout << "Available commands:" << std::endl;
    std::cout << " add <standName> : add new stand" << std::endl;
    std::cout << " remove <standName> : remove existing stand" << std::endl;
    std::cout << " copy <standName> : copy existing stand" << std::endl;
    std::cout << " edit <standName> : edit existing stand" << std::endl;
    std::cout << " list : list all stands" << std::endl;
    std::cout << " save : save changes and exit" << std::endl;
    std::cout << " exit : exit without saving" << std::endl;
}

//TODO: display stands details
void listAllStands(const nlohmann::ordered_json& configJson) {
    if (configJson.contains("STAND") && configJson["STAND"].is_object()) {
        std::cout << "Current stands:" << std::endl;
        for (auto& [key, value] : configJson["STAND"].items()) {
            std::cout << " - " << key << std::endl;
        }
    } else {
        std::cout << "No stands available." << std::endl;
    }
}

//TODO: Add all the questions for restrictions
void addStand(nlohmann::ordered_json& configJson, const std::string& standName) {
    std::string standNameUpper = standName;
    std::transform(standNameUpper.begin(), standNameUpper.end(), standNameUpper.begin(), ::toupper);
    if (!configJson.contains("STAND") || !configJson["STAND"].is_object()) {
        configJson["STAND"] = nlohmann::ordered_json::object();
    }
    if (configJson["STAND"].contains(standNameUpper)) {
        std::cout << "Stand " << standNameUpper << " already exists." << std::endl;
    } else {
        configJson["STAND"][standNameUpper] = nlohmann::ordered_json::object();
        std::cout << "Stand " << standNameUpper << " added." << std::endl;
    }
}

void removeStand(nlohmann::ordered_json& configJson, const std::string& standName) {
    std::string standNameUpper = standName;
    std::transform(standNameUpper.begin(), standNameUpper.end(), standNameUpper.begin(), ::toupper);
    if (configJson.contains("STAND") && configJson["STAND"].is_object()) {
        if (configJson["STAND"].contains(standNameUpper)) {
            configJson["STAND"].erase(standNameUpper);
            std::cout << "Stand " << standNameUpper << " removed." << std::endl;
        } else {
            std::cout << "Stand " << standNameUpper << " does not exist." << std::endl;
        }
    } else {
        std::cout << "No stands available to remove." << std::endl;
    }
}

int main() {
    std::cout << "Config Creator " + std::string(version) << std::endl;

    std::string icao;
    std::cout << "Select config file (ICAO, if not found, new one is created): ";
    std::getline(std::cin, icao);
    std::transform(icao.begin(), icao.end(), icao.begin(), ::toupper);

    nlohmann::ordered_json configJson;

    if (!getConfig(icao, configJson)) {
        return 1;
    }

    // Ready to edit the JSON
    std::cout << "JSON edition ready." << std::endl;
    printMenu();

    std::string command;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, command);
        std::transform(command.begin(), command.end(), command.begin(), ::tolower);

        if (command == "exit") {
            break;
        } else if (command == "save") {
            saveFile(icao, configJson);
            break;
        } else if (command == "list") {
            listAllStands(configJson);
        } else if (command.rfind("add ", 0) == 0) {
            std::string standName = command.substr(4);
            addStand(configJson, standName);
        } else if (command.rfind("remove ", 0) == 0) {
            std::string standName = command.substr(7);
            removeStand(configJson, standName);
        } else if (command.rfind("copy ", 0) == 0) {
            std::string standName = command.substr(5);
            // copyStand(configJson, standName);
        } else if (command.rfind("edit ", 0) == 0) {
            std::string standName = command.substr(5);
            // editStand(configJson, standName);
        } else if (command == "help") {
            printMenu();
        } else {
            std::cout << "Unknown command: " << command << std::endl;
        }
    }

    return 0;
}