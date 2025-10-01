#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <algorithm>
#include "nlohmann/json.hpp"

constexpr auto version = "v1.0.1";
constexpr auto baseDir = "./configs/";

// Standard colors
#define RESET "\033[0m"
#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define GREY "\033[90m"

// Bright colors
#define BRIGHT_BLACK "\033[90m"
#define BRIGHT_RED "\033[91m"
#define BRIGHT_GREEN "\033[92m"
#define BRIGHT_YELLOW "\033[93m"
#define BRIGHT_BLUE "\033[94m"
#define BRIGHT_MAGENTA "\033[95m"
#define BRIGHT_CYAN "\033[96m"
#define BRIGHT_WHITE "\033[97m"

// Formatting
#define BOLD "\033[1m"
#define UNDERLINE "\033[4m"
#define REVERSED "\033[7m"

std::vector<std::string> splitString(const std::string &str)
{
    std::vector<std::string> result;
    std::istringstream stream(str);
    std::string token;
    while (std::getline(stream, token, ','))
    {
        token.erase(std::remove_if(token.begin(), token.end(), ::isspace), token.end());
        std::transform(token.begin(), token.end(), token.begin(), ::toupper);
        if (!token.empty())
        {
            result.push_back(token);
        }
    }
    return result;
}

bool getConfig(const std::string &icao, nlohmann::ordered_json &configJson)
{
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
            std::cout << RED << "Error creating directory: " << e.what() << RESET << std::endl
                      << "Please create the directory manually: " << baseDir << std::endl;
            return false;
        }
    }

    bool fileExists = std::filesystem::exists(baseDir + icao + ".json");
    if (!fileExists)
    {
        std::cout << YELLOW << "Config file not found, creating a new one." << RESET << std::endl;
    }
    else
    {
        std::cout << GREEN << "Config file found: " << icao << ".json" << RESET << std::endl;
        std::ifstream inputFile(baseDir + icao + ".json");
        if (inputFile)
        {
            try
            {
                inputFile >> configJson;
            }
            catch (const std::exception &e)
            {
                std::cout << RED << "Error reading JSON: " << e.what() << RESET << std::endl;
                return false;
            }
        }
        else
        {
            std::cout << RED << "Error opening file for reading." << RESET << std::endl;
            return false;
        }
    }

    if (configJson.empty())
    {
        // Create default config
        configJson = {
            {"VERSION", "1.0.0"},
            {"ICAO", icao},
            {"STAND", {}}};
        std::cout << "Created default config structure." << std::endl;
    }

    return true;
}

void saveFile(const std::string &icao, const nlohmann::ordered_json &configJson)
{
    std::ofstream outputFile(baseDir + icao + ".json");
    if (outputFile)
    {
        nlohmann::ordered_json sortedStands;
        if (configJson.contains("STAND") && configJson["STAND"].is_object())
        {
            std::vector<std::string> standKeys;
            for (auto &[key, value] : configJson["STAND"].items())
            {
                standKeys.push_back(key);
            }
            std::sort(standKeys.begin(), standKeys.end());
            for (const auto &key : standKeys)
            {
                sortedStands[key] = configJson["STAND"][key];
            }
        }
        nlohmann::ordered_json finalJson = configJson;
        finalJson["STAND"] = sortedStands;
        outputFile << finalJson.dump(4);
        std::cout << GREEN << "Config file saved: " << icao << ".json" << RESET << std::endl;
        outputFile.close();
    }
    else
    {
        std::cout << RED << "Error opening file for writing." << RESET << std::endl;
        return;
    }
}

void printMenu()
{
    std::cout << GREY << "Available commands:" << std::endl;
    std::cout << " add <standName> : add new stand" << std::endl;
    std::cout << " remove <standName> : remove existing stand" << std::endl;
    std::cout << " copy <sourceStand> : copy existing stand settings" << std::endl;
    std::cout << " softcopy <sourceStand> : copy existing stand settings but iterate through them so you can modify" << std::endl;
    std::cout << " edit <standName> : edit existing stand" << std::endl;
    std::cout << " list : list all stands" << std::endl;
    std::cout << " save : save changes and exit" << std::endl;
    std::cout << " exit : exit without saving" << RESET << std::endl;
}

void printStandInfo(const nlohmann::ordered_json &standJson)
{
    if (standJson.contains("Coordinates"))
    {
        std::cout << GREY << " | Coordinates: " << standJson["Coordinates"] << "|";
    }
    if (standJson.contains("Code"))
    {
        std::cout << " Code: " << standJson["Code"] << "|";
    }
    if (standJson.contains("Use"))
    {
        std::cout << " Use: " << standJson["Use"] << "|";
    }
    if (standJson.contains("Schengen"))
    {
        std::cout << " Schengen: " << (standJson["Schengen"] ? "Yes" : "No") << "|";
    }
    if (standJson.contains("Callsigns"))
    {
        std::cout << " Callsigns: ";
        for (const auto &callsign : standJson["Callsigns"])
        {
            std::cout << callsign << " ";
        }
        std::cout << "|";
    }
    if (standJson.contains("Countries"))
    {
        std::cout << " Countries: ";
        for (const auto &country : standJson["Countries"])
        {
            std::cout << country << " ";
        }
        std::cout << "|";
    }
    if (standJson.contains("Block"))
    {
        std::cout << " Block: ";
        for (const auto &blocked : standJson["Block"])
        {
            std::cout << blocked << " ";
        }
        std::cout << "|";
    }
    if (standJson.contains("Priority"))
    {
        std::cout << " Priority: " << standJson["Priority"] << "|";
    }
    if (standJson.contains("Apron"))
    {
        std::cout << " Apron: ";
        std::cout << (standJson["Apron"].get<bool>() ? "Yes" : "No");
        std::cout << "|";
    }
    std::cout << RESET << std::endl;
}

void listAllStands(const nlohmann::ordered_json &configJson)
{
    if (configJson.contains("STAND") && configJson["STAND"].is_object())
    {
        std::cout << "Current stands:" << std::endl;
        for (auto &[key, value] : configJson["STAND"].items())
        {
            std::cout << " - " << BLUE << BOLD << key << RESET;
            printStandInfo(value);
        }
    }
    else
    {
        std::cout << "No stands available." << std::endl;
    }
}

bool isCoordinatesValid(const std::string &coordinates)
{
    // Basic validation of Degree decimal format
    size_t firstColon = coordinates.find(':');
    size_t secondColon = coordinates.find(':', firstColon + 1);
    if (firstColon == std::string::npos || secondColon == std::string::npos)
    {
        return false;
    }
    // Check if the parts are valid numbers
    std::string lat = coordinates.substr(0, firstColon);
    std::string lon = coordinates.substr(firstColon + 1, secondColon - firstColon - 1);
    std::string radius = coordinates.substr(secondColon + 1);

    // Valid format : 43.666359:7.216941:20
    try
    {
        double latVal = std::stod(lat);
        double lonVal = std::stod(lon);
        double radiusVal = std::stod(radius);
        if (latVal < -90 || latVal > 90 || lonVal < -180 || lonVal > 180 || radiusVal < 0)
        {
            return false;
        }
    }
    catch (...)
    {
        return false;
    }
    return true;
}

void addStand(nlohmann::ordered_json &configJson, const std::string &standName)
{
    std::string standNameUpper = standName;
    std::transform(standNameUpper.begin(), standNameUpper.end(), standNameUpper.begin(), ::toupper);
    if (!configJson.contains("STAND") || !configJson["STAND"].is_object())
    {
        configJson["STAND"] = nlohmann::ordered_json::object();
    }
    if (configJson["STAND"].contains(standNameUpper))
    {
        std::cout << RED << "Stand " << standNameUpper << " already exists." << RESET << std::endl;
    }
    else
    {
        configJson["STAND"][standNameUpper] = nlohmann::ordered_json::object();

        std::cout << "Enter coordinates (format: lat:lon:radius): ";
        std::string coordinates;
        while (true)
        {
            std::getline(std::cin, coordinates);
            if (!isCoordinatesValid(coordinates))
            {
                std::cout << RED << "Invalid coordinates format. Please use lat:lon:radius (e.g., 43.666359:7.216941:20)." << RESET << std::endl;
                std::cout << "Enter coordinates (format: lat:lon:radius): ";
                continue;
            }
            else
            {
                configJson["STAND"][standNameUpper]["Coordinates"] = coordinates;
                break;
            }
        }

        std::cout << "Enter code (optional): ";
        std::string code;
        while (true)
        {
            std::getline(std::cin, code);
            if (!code.empty())
            {
                std::transform(code.begin(), code.end(), code.begin(), ::toupper);
                configJson["STAND"][standNameUpper]["Code"] = code;
                break;
            }
            else
                break;
        }

        std::cout << "Enter use (optional): ";
        std::string use;
        while (true)
        {
            std::getline(std::cin, use);
            if (use.length() > 1)
            {
                std::cout << RED << "Use must be a single character." << RESET << std::endl;
                std::cout << "Enter use (optional): ";
                continue;
            }
            else if (!use.empty())
            {
                std::transform(use.begin(), use.end(), use.begin(), ::toupper);
                configJson["STAND"][standNameUpper]["Use"] = use;
                break;
            }
            else
                break;
        }

        std::cout << "Is it a Schengen stand? (y/n/empty): ";
        std::string schengenInput;
        std::getline(std::cin, schengenInput);
        bool schengen = (schengenInput == "y" || schengenInput == "Y");
        bool notSchengen = (schengenInput == "n" || schengenInput == "N");
        if (schengen || notSchengen)
        {
            configJson["STAND"][standNameUpper]["Schengen"] = schengen;
        }

        std::cout << "Enter callsigns (comma separated, optional): ";
        std::string callsignsInput;
        std::getline(std::cin, callsignsInput);
        if (!callsignsInput.empty())
        {
            std::vector<std::string> callsigns = splitString(callsignsInput);
            if (!callsigns.empty())
            {
                configJson["STAND"][standNameUpper]["Callsigns"] = callsigns;
            }
        }

        std::cout << "Enter countries (comma separated, optional): ";
        std::string countriesInput;
        std::getline(std::cin, countriesInput);
        if (!countriesInput.empty())
        {
            std::vector<std::string> countries = splitString(countriesInput);
            if (!countries.empty())
            {
                configJson["STAND"][standNameUpper]["Countries"] = countries;
            }
        }

        std::cout << "Enter blocked stands (comma separated, optional): ";
        std::string blockInput;
        std::getline(std::cin, blockInput);
        if (!blockInput.empty())
        {
            std::vector<std::string> blocked = splitString(blockInput);
            if (!blocked.empty())
            {
                configJson["STAND"][standNameUpper]["Block"] = blocked;
            }
        }

        std::cout << "Enter priority (integer, optional): ";
        std::string priorityInput;
        while (true)
        {
            std::getline(std::cin, priorityInput);
            if (!priorityInput.empty())
            {
                try
                {
                    int priority = std::stoi(priorityInput);
                    configJson["STAND"][standNameUpper]["Priority"] = priority;
                    break;
                }
                catch (const std::exception &e)
                {
                    std::cout << RED << "Invalid priority input." << RESET << std::endl;
                    std::cout << "Enter priority (integer, optional): ";
                    continue;
                }
            }
            else
                break;
        }

        std::cout << "Is it an apron stand? (y/n, default n): ";
        std::string apronInput;
        std::getline(std::cin, apronInput);
        bool apron = (apronInput == "y" || apronInput == "Y");
        if (apron)
        {
            configJson["STAND"][standNameUpper]["Apron"] = apron;
        }

        std::cout << "Stand " << BLUE << standNameUpper << " added." << RESET;
        printStandInfo(configJson["STAND"][standNameUpper]);
        std::cout << std::endl;
    }
}

void removeStand(nlohmann::ordered_json &configJson, const std::string &standName)
{
    std::string standNameUpper = standName;
    std::transform(standNameUpper.begin(), standNameUpper.end(), standNameUpper.begin(), ::toupper);
    if (configJson.contains("STAND") && configJson["STAND"].is_object())
    {
        if (configJson["STAND"].contains(standNameUpper))
        {
            configJson["STAND"].erase(standNameUpper);
            std::cout << GREEN << "Stand " << standNameUpper << " removed." << RESET << std::endl;
        }
        else
        {
            std::cout << RED << "Stand " << standNameUpper << " does not exist." << RESET << std::endl;
        }
    }
    else
    {
        std::cout << GREY << "No stands available to remove." << RESET << std::endl;
    }
}

void editStand(nlohmann::ordered_json &configJson, const std::string &standName)
{
    std::string standNameUpper = standName;
    std::transform(standNameUpper.begin(), standNameUpper.end(), standNameUpper.begin(), ::toupper);
    if (configJson.contains("STAND") && configJson["STAND"].is_object() && configJson["STAND"].contains(standNameUpper))
    {
        nlohmann::ordered_json &standJson = configJson["STAND"][standNameUpper];
        std::cout << "Editing stand " << BLUE << BOLD << standNameUpper << RESET << std::endl;
        printStandInfo(standJson);

        std::cout << "Enter new coordinates (current: " << (standJson.contains("Coordinates") ? standJson["Coordinates"].get<std::string>() : "none") << ", format: lat:lon:radius, empty to keep): ";
        std::string coordinates;
        while (true)
        {
            std::getline(std::cin, coordinates);
            if (coordinates.empty())
            {
                break; // Keep current
            }
            if (!isCoordinatesValid(coordinates))
            {
                std::cout << RED << "Invalid coordinates format. Please use lat:lon:radius (e.g., 43.666359:7.216941:20)." << RESET << std::endl;
                std::cout << "Enter new coordinates (format: lat:lon:radius, empty to keep): ";
                continue;
            }
            else
            {
                standJson["Coordinates"] = coordinates;
                break;
            }
        }

        std::cout << "Enter new code (current: " << (standJson.contains("Code") ? standJson["Code"].get<std::string>() : "none") << ", empty to keep, r to remove): ";
        std::string code;
        while (true)
        {
            std::getline(std::cin, code);
            if (code.empty())
            {
                break; // Keep current
            }
            else if (code == "r" || code == "R")
            {
                standJson.erase("Code");
                break;
            }
            else
            {
                std::transform(code.begin(), code.end(), code.begin(), ::toupper);
                standJson["Code"] = code;
                break;
            }
        }

        std::cout << "Enter new use (current: " << (standJson.contains("Use") ? standJson["Use"].get<std::string>() : "none") << ", single character, empty to keep, r to remove): ";
        std::string use;
        while (true)
        {
            std::getline(std::cin, use);
            if (use.length() > 1)
            {
                std::cout << RED << "Use must be a single character." << RESET << std::endl;
                std::cout << "Enter new use (single character, empty to keep/remove): ";
                continue;
            }
            else if (use.empty())
            {
                break; // Keep current
            }
            else if (use == "r" || use == "R")
            {
                standJson.erase("Use");
                break;
            }
            else
            {
                std::transform(use.begin(), use.end(), use.begin(), ::toupper);
                standJson["Use"] = use;
                break;
            }
        }

        std::cout << "Is it a Schengen stand? (current: " << (standJson.contains("Schengen") ? (standJson["Schengen"].get<bool>() ? "Yes" : "No") : "none") << " Y/N, empty to keep, r to remove): ";
        std::string schengen;
        while (true)
        {
            std::getline(std::cin, schengen);
            if (schengen.empty())
            {
                break; // Keep current
            }
            else if (schengen == "r" || schengen == "R")
            {
                standJson.erase("Schengen");
                break;
            }
            else if (schengen == "Y" || schengen == "y")
            {
                standJson["Schengen"] = true;
                break;
            }
            else if (schengen == "N" || schengen == "n")
            {
                standJson["Schengen"] = false;
                break;
            }
            else
            {
                std::cout << RED << "Invalid input. Please enter 'Y', 'N', 'R' to remove or leave empty to keep." << RESET << std::endl;
            }
        }

        std::cout << "Enter new callsigns (current: ";
        if (standJson.contains("Callsigns"))
        {
            for (const auto &cs : standJson["Callsigns"])
            {
                std::cout << cs << " ";
            }
        }
        else
        {
            std::cout << "none";
        }
        std::cout << ", comma separated, empty to keep, r to remove): ";
        std::string callsignsInput;
        while (true)
        {
            std::getline(std::cin, callsignsInput);
            if (callsignsInput.empty())
            {
                break; // Keep current
            }
            else if (callsignsInput == "r" || callsignsInput == "R")
            {
                standJson.erase("Callsigns");
                break;
            }
            else
            {
                std::vector<std::string> callsigns = splitString(callsignsInput);
                if (!callsigns.empty())
                {
                    standJson["Callsigns"] = callsigns;
                }
                else
                {
                    standJson.erase("Callsigns");
                }
                break;
            }
        }

        std::cout << "Enter new countries (current: ";
        if (standJson.contains("Countries"))
        {
            for (const auto &country : standJson["Countries"])
            {
                std::cout << country << " ";
            }
        }
        else
        {
            std::cout << "none";
        }

        std::cout << ", comma separated, empty to keep, r to remove): ";
        std::string countriesInput;
        while (true)
        {
            std::getline(std::cin, countriesInput);
            if (countriesInput.empty())
            {
                break; // Keep current
            }
            else if (countriesInput == "r" || countriesInput == "R")
            {
                standJson.erase("Countries");
                break;
            }
            else
            {
                std::vector<std::string> countries = splitString(countriesInput);
                if (!countries.empty())
                {
                    standJson["Countries"] = countries;
                }
                else
                {
                    standJson.erase("Countries");
                }
                break;
            }
        }

        std::cout << "Enter new blocked stands (current: ";
        if (standJson.contains("Block"))
        {
            for (const auto &blk : standJson["Block"])
            {
                std::cout << blk << " ";
            }
        }
        else
        {
            std::cout << "none";
        }
        std::cout << ", comma separated, empty to keep, r to remove): ";
        std::string blockInput;
        while (true)
        {
            std::getline(std::cin, blockInput);
            if (blockInput.empty())
            {
                break; // Keep current
            }
            else if (blockInput == "r" || blockInput == "R")
            {
                standJson.erase("Block");
                break;
            }
            else
            {
                std::vector<std::string> blocked = splitString(blockInput);
                if (!blocked.empty())
                {
                    standJson["Block"] = blocked;
                }
                else
                {
                    standJson.erase("Block");
                }
                break;
            }
        }

        std::cout << "Enter new priority (current: " << (standJson.contains("Priority") ? std::to_string(standJson["Priority"].get<int>()) : "none") << ", integer, empty to keep, r to remove): ";
        std::string priorityInput;
        while (true)
        {
            std::getline(std::cin, priorityInput);
            if (priorityInput.empty())
            {
                break; // Keep current
            }
            else if (priorityInput == "r" || priorityInput == "R")
            {
                standJson.erase("Priority");
                break;
            }
            else
            {
                try
                {
                    int priority = std::stoi(priorityInput);
                    standJson["Priority"] = priority;
                    break;
                }
                catch (const std::exception &e)
                {
                    std::cout << RED << "Invalid priority input." << RESET << std::endl;
                    std::cout << "Enter new priority (integer, empty to keep/remove): ";
                    continue;
                }
            }
        }

        std::cout << "Is it an apron stand? (current: " << (standJson.contains("Apron") ? (standJson["Apron"].get<bool>() ? "Yes" : "No") : "No") << " Y if apron, empty to keep, r to remove): ";
        std::string apronInput;
        while (true)
        {
            std::getline(std::cin, apronInput);
            if (apronInput.empty())
            {
                break; // Keep current
            }
            else if (apronInput == "r" || apronInput == "R")
            {
                standJson.erase("Apron");
                break;
            }
            else if (apronInput == "Y" || apronInput == "y")
            {
                standJson["Apron"] = true;
                break;
            }
            else
            {
                std::cout << RED << "Invalid input. Please enter 'Y', 'N', 'R' to remove or leave empty to keep." << RESET << std::endl;
            }
        }
        std::cout << GREEN << "Stand " << standNameUpper << " updated." << RESET;
        printStandInfo(standJson);
        std::cout << std::endl;
    }
    else
    {
        std::cout << RED << "Stand " << standNameUpper << " does not exist." << RESET << std::endl;
    }
}

void copyStand(nlohmann::ordered_json &configJson, const std::string &standName)
{
    std::string standNameUpper = standName;
    std::transform(standNameUpper.begin(), standNameUpper.end(), standNameUpper.begin(), ::toupper);
    if (configJson.contains("STAND") && configJson["STAND"].is_object() && configJson["STAND"].contains(standNameUpper))
    {
        std::string newStandName;
        std::cout << "Enter new stand name for the copy: ";
        while (true) {
            std::getline(std::cin, newStandName);
            std::transform(newStandName.begin(), newStandName.end(), newStandName.begin(), ::toupper);
            if (newStandName.empty())
            {
                std::cout << RED << "New stand name cannot be empty." << RESET << std::endl;
                std::cout << "Enter new stand name for the copy: ";
                continue;
            } else if (configJson["STAND"].contains(newStandName))
            {
                std::cout << RED << "Stand " << newStandName << " already exists." << RESET << std::endl;
                std::cout << "Enter new stand name for the copy: ";
                continue;
            } else {
                configJson["STAND"][newStandName] = configJson["STAND"][standNameUpper];
                break;
            }
        }
        std::cout << "Enter new coordinates for the copied stand (format: lat:lon:radius): ";
        std::string coordinates;
        while (true)
        {
            std::getline(std::cin, coordinates);
            if (!isCoordinatesValid(coordinates))
            {
                std::cout << RED << "Invalid coordinates format. Please use lat:lon:radius (e.g., 43.666359:7.216941:20)." << RESET << std::endl;
                std::cout << "Enter new coordinates for the copied stand (format: lat:lon:radius): ";
                continue;
            }
            else
            {
                configJson["STAND"][newStandName]["Coordinates"] = coordinates;
                break;
            }
        }
        std::cout << GREEN << "Stand " << standNameUpper << " copied to " << newStandName << "." << RESET << std::endl;
        printStandInfo(configJson["STAND"][newStandName]);
        std::cout << std::endl;
    }
    else
    {
        std::cout << RED << "Stand " << standNameUpper << " does not exist." << RESET << std::endl;
    }
}

void softStandCopy(nlohmann::ordered_json &configJson, const std::string &standName)
{
    std::string standNameUpper = standName;
    std::transform(standNameUpper.begin(), standNameUpper.end(), standNameUpper.begin(), ::toupper);
    if (configJson.contains("STAND") && configJson["STAND"].is_object() && configJson["STAND"].contains(standNameUpper))
    {
        std::string newStandName;
        std::cout << "Enter new stand name for the copy: ";
        while (true) {
            std::getline(std::cin, newStandName);
            std::transform(newStandName.begin(), newStandName.end(), newStandName.begin(), ::toupper);
            if (newStandName.empty())
            {
                std::cout << RED << "New stand name cannot be empty." << RESET << std::endl;
                std::cout << "Enter new stand name for the copy: ";
                continue;
            } else if (configJson["STAND"].contains(newStandName))
            {
                std::cout << RED << "Stand " << newStandName << " already exists." << RESET << std::endl;
                std::cout << "Enter new stand name for the copy: ";
                continue;
            } else {
                configJson["STAND"][newStandName] = configJson["STAND"][standNameUpper];
                break;
            }
        }
        std::cout << "Enter new coordinates for the copied stand (format: lat:lon:radius): ";
        std::string coordinates;
        while (true)
        {
            std::getline(std::cin, coordinates);
            if (!isCoordinatesValid(coordinates))
            {
                std::cout << RED << "Invalid coordinates format. Please use lat:lon:radius (e.g., 43.666359:7.216941:20)." << RESET << std::endl;
                std::cout << "Enter new coordinates for the copied stand (format: lat:lon:radius): ";
                continue;
            }
            else
            {
                configJson["STAND"][newStandName]["Coordinates"] = coordinates;
                break;
            }
        }

        nlohmann::ordered_json &standJson = configJson["STAND"][newStandName];
        std::cout << "Soft copying stand " << BLUE << BOLD << standNameUpper << RESET << std::endl;
        printStandInfo(standJson);

        std::cout << "Enter new code (current: " << (standJson.contains("Code") ? standJson["Code"].get<std::string>() : "none") << ", empty to keep, r to remove): ";
        std::string code;
        while (true)
        {
            std::getline(std::cin, code);
            if (code.empty())
            {
                break; // Keep current
            }
            else if (code == "r" || code == "R")
            {
                standJson.erase("Code");
                break;
            }
            else
            {
                std::transform(code.begin(), code.end(), code.begin(), ::toupper);
                standJson["Code"] = code;
                break;
            }
        }

        std::cout << "Enter new use (current: " << (standJson.contains("Use") ? standJson["Use"].get<std::string>() : "none") << ", single character, empty to keep, r to remove): ";
        std::string use;
        while (true)
        {
            std::getline(std::cin, use);
            if (use.length() > 1)
            {
                std::cout << RED << "Use must be a single character." << RESET << std::endl;
                std::cout << "Enter new use (single character, empty to keep/remove): ";
                continue;
            }
            else if (use.empty())
            {
                break; // Keep current
            }
            else if (use == "r" || use == "R")
            {
                standJson.erase("Use");
                break;
            }
            else
            {
                std::transform(use.begin(), use.end(), use.begin(), ::toupper);
                standJson["Use"] = use;
                break;
            }
        }

        std::cout << "Is it a Schengen stand? (current: " << (standJson.contains("Schengen") ? (standJson["Schengen"].get<bool>() ? "Yes" : "No") : "none") << " Y/N, empty to keep, r to remove): ";
        std::string schengen;
        while (true)
        {
            std::getline(std::cin, schengen);
            if (schengen.empty())
            {
                break; // Keep current
            }
            else if (schengen == "r" || schengen == "R")
            {
                standJson.erase("Schengen");
                break;
            }
            else if (schengen == "Y" || schengen == "y")
            {
                standJson["Schengen"] = true;
                break;
            }
            else if (schengen == "N" || schengen == "n")
            {
                standJson["Schengen"] = false;
                break;
            }
            else
            {
                std::cout << RED << "Invalid input. Please enter 'Y', 'N', 'R' to remove or leave empty to keep." << RESET << std::endl;
            }
        }

        std::cout << "Enter new callsigns (current: ";
        if (standJson.contains("Callsigns"))
        {
            for (const auto &cs : standJson["Callsigns"])
            {
                std::cout << cs << " ";
            }
        }
        else
        {
            std::cout << "none";
        }
        std::cout << ", comma separated, empty to keep, r to remove): ";
        std::string callsignsInput;
        while (true)
        {
            std::getline(std::cin, callsignsInput);
            if (callsignsInput.empty())
            {
                break; // Keep current
            }
            else if (callsignsInput == "r" || callsignsInput == "R")
            {
                standJson.erase("Callsigns");
                break;
            }
            else
            {
                std::vector<std::string> callsigns = splitString(callsignsInput);
                if (!callsigns.empty())
                {
                    standJson["Callsigns"] = callsigns;
                }
                else
                {
                    standJson.erase("Callsigns");
                }
                break;
            }
        }

        std::cout << "Enter new countries (current: ";
        if (standJson.contains("Countries"))
        {
            for (const auto &country : standJson["Countries"])
            {
                std::cout << country << " ";
            }
        }
        else
        {
            std::cout << "none";
        }

        std::cout << ", comma separated, empty to keep, r to remove): ";
        std::string countriesInput;
        while (true)
        {
            std::getline(std::cin, countriesInput);
            if (countriesInput.empty())
            {
                break; // Keep current
            }
            else if (countriesInput == "r" || countriesInput == "R")
            {
                standJson.erase("Countries");
                break;
            }
            else
            {
                std::vector<std::string> countries = splitString(countriesInput);
                if (!countries.empty())
                {
                    standJson["Countries"] = countries;
                }
                else
                {
                    standJson.erase("Countries");
                }
                break;
            }
        }

        std::cout << "Enter new blocked stands (current: ";
        if (standJson.contains("Block"))
        {
            for (const auto &blk : standJson["Block"])
            {
                std::cout << blk << " ";
            }
        }
        else
        {
            std::cout << "none";
        }
        std::cout << ", comma separated, empty to keep, r to remove): ";
        std::string blockInput;
        while (true)
        {
            std::getline(std::cin, blockInput);
            if (blockInput.empty())
            {
                break; // Keep current
            }
            else if (blockInput == "r" || blockInput == "R")
            {
                standJson.erase("Block");
                break;
            }
            else
            {
                std::vector<std::string> blocked = splitString(blockInput);
                if (!blocked.empty())
                {
                    standJson["Block"] = blocked;
                }
                else
                {
                    standJson.erase("Block");
                }
                break;
            }
        }

        std::cout << "Enter new priority (current: " << (standJson.contains("Priority") ? std::to_string(standJson["Priority"].get<int>()) : "none") << ", integer, empty to keep, r to remove): ";
        std::string priorityInput;
        while (true)
        {
            std::getline(std::cin, priorityInput);
            if (priorityInput.empty())
            {
                break; // Keep current
            }
            else if (priorityInput == "r" || priorityInput == "R")
            {
                standJson.erase("Priority");
                break;
            }
            else
            {
                try
                {
                    int priority = std::stoi(priorityInput);
                    standJson["Priority"] = priority;
                    break;
                }
                catch (const std::exception &e)
                {
                    std::cout << RED << "Invalid priority input." << RESET << std::endl;
                    std::cout << "Enter new priority (integer, empty to keep/remove): ";
                    continue;
                }
            }
        }

        std::cout << "Is it an apron stand? (current: " << (standJson.contains("Apron") ? (standJson["Apron"].get<bool>() ? "Yes" : "No") : "No") << " Y if apron, empty to keep, r to remove): ";
        std::string apronInput;
        while (true)
        {
            std::getline(std::cin, apronInput);
            if (apronInput.empty())
            {
                break; // Keep current
            }
            else if (apronInput == "r" || apronInput == "R")
            {
                standJson.erase("Apron");
                break;
            }
            else if (apronInput == "Y" || apronInput == "y")
            {
                standJson["Apron"] = true;
                break;
            }
            else
            {
                std::cout << RED << "Invalid input. Please enter 'Y', 'N', 'R' to remove or leave empty to keep." << RESET << std::endl;
            }
        }
        std::cout << GREEN << "Stand " << newStandName << " added." << RESET;
        printStandInfo(standJson);
        std::cout << std::endl;
    }
    else
    {
        std::cout << RED << "Stand " << standNameUpper << " does not exist." << RESET << std::endl;
    }
}

int main()
{
    std::cout << "Config Creator " + std::string(version) << std::endl;

    std::string icao;
    std::cout << "Select config file (ICAO, if not found, new one is created): ";
    std::getline(std::cin, icao);
    std::transform(icao.begin(), icao.end(), icao.begin(), ::toupper);

    nlohmann::ordered_json configJson;

    if (!getConfig(icao, configJson))
    {
        return 1;
    }

    // Ready to edit the JSON
    std::cout << "JSON edition ready." << std::endl;
    printMenu();

    std::string command;
    while (true)
    {
        std::cout << "> ";
        std::getline(std::cin, command);
        std::transform(command.begin(), command.end(), command.begin(), ::tolower);

        if (command == "exit")
        {
            break;
        }
        else if (command == "save")
        {
            saveFile(icao, configJson);
            break;
        }
        else if (command == "list")
        {
            listAllStands(configJson);
        }
        else if (command.rfind("add ", 0) == 0)
        {
            std::string standName = command.substr(4);
            addStand(configJson, standName);
        }
        else if (command.rfind("remove ", 0) == 0)
        {
            std::string standName = command.substr(7);
            removeStand(configJson, standName);
        }
        else if (command.rfind("copy ", 0) == 0)
        {
            std::string standName = command.substr(5);
            copyStand(configJson, standName);
        }
        else if (command.rfind("softcopy ", 0) == 0)
        {
            std::string standName = command.substr(9);
            softStandCopy(configJson, standName);
        }
        else if (command.rfind("edit ", 0) == 0)
        {
            std::string standName = command.substr(5);
            editStand(configJson, standName);
        }
        else if (command == "help")
        {
            printMenu();
        }
        else
        {
            std::cout << "Unknown command: " << command << std::endl;
        }
    }

    return 0;
}