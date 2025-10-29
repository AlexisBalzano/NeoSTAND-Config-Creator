#include "config_manager.h"
#include "utils.h"
#include "map_generator.h"
#include <filesystem>
#include <fstream>
#include <iostream>

bool getConfig(const std::string &icao, nlohmann::ordered_json &configJson, bool& mapGenerated)
{
    std::string baseDir = getBaseDir();
    std::cout << "Config directory path: " << baseDir << std::endl;
    
    bool dirExists = std::filesystem::exists(baseDir);
    if (!dirExists)
    {
        std::cout << "Config directory not found, creating it." << std::endl;
        try
        {
            std::filesystem::create_directories(baseDir);
        }
        catch (const std::exception &e)
        {
            std::cout << "Error creating directory: " << e.what() << std::endl
                      << "Please create the directory manually: " << baseDir << std::endl;
            return false;
        }
    }

    bool fileExists = std::filesystem::exists(baseDir + icao + ".json");
    if (!fileExists)
    {
        std::cout << "Config file not found, creating a new one." << std::endl;
    }
    else
    {
        std::cout << "Config file found: " << icao << ".json" << std::endl;
        std::ifstream inputFile(baseDir + icao + ".json");
        if (inputFile)
        {
            try
            {
                inputFile >> configJson;
            }
            catch (const std::exception &e)
            {
                std::cout << "Error reading JSON: " << e.what() << std::endl;
                return false;
            }
        }
        else
        {
            std::cout << "Error opening file for reading." << std::endl;
            return false;
        }
    }

    if (configJson.empty())
    {
        // Create default config
        std::string coordinates;
        std::cout << "Enter airport coordinates (format: lat:lon:radius): ";
        while (true)
        {
            std::getline(std::cin, coordinates);
            if (!isCoordinatesValid(coordinates))
            {
                std::cout << "Invalid coordinates format. Please use lat:lon:radius (e.g., 43.666359:7.216941:20)." << std::endl;
                std::cout << "Enter airport coordinates (format: lat:lon:radius): ";
                continue;
            }
            else
            {
                break;
            }
        }

        configJson = {
            {"$schema", "https://raw.githubusercontent.com/vaccfr/RampAgent-Config/main/.github/schema/airportConfig.schema.json"},
            {"version", "v1.0.0"},
            {"ICAO", icao},
            {"Coordinates", coordinates},
            {"Stands", {}}};
        std::cout << "Created default config structure." << std::endl;
        // Generate an initial map file for live-reload/debugging
        generateMap(configJson, icao, true);
        mapGenerated = true;
    }

    return true;
}

void saveFile(const std::string &icao, const nlohmann::ordered_json &configJson)
{
    std::string baseDir = getBaseDir();
    std::ofstream outputFile(baseDir + icao + ".json");
    if (outputFile)
    {
        nlohmann::ordered_json sortedStands;
        if (configJson.contains("Stands") && configJson["Stands"].is_object())
        {
            std::vector<std::string> standKeys;
            for (auto &[key, value] : configJson["Stands"].items())
            {
                standKeys.push_back(key);
            }
            // Use natural sort instead of standard sort
            std::sort(standKeys.begin(), standKeys.end(), naturalSort);
            for (const auto &key : standKeys)
            {
                sortedStands[key] = configJson["Stands"][key];
            }
        }
        nlohmann::ordered_json finalJson = configJson;
        finalJson["Stands"] = sortedStands;
        outputFile << finalJson.dump(4);
        std::cout << GREEN << "Config file saved: " << icao << ".json" << std::endl;
        outputFile.close();
    }
    else
    {
        std::cout << RED << "Error opening file for writing." << std::endl;
        return;
    }
}
