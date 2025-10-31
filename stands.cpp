#include "stands.h"
#include "utils.h"
#include <iostream>
#include <algorithm>
#include <sstream>

void printMenu()
{
    std::cout << GREY;
    std::cout << "Available commands:" << std::endl;
    std::cout << " add <standName> : add new stand" << std::endl;
    std::cout << " remove <standName> : remove existing stand" << std::endl;
    std::cout << " copy <sourceStand> : copy existing stand settings" << std::endl;
    std::cout << " batchcopy <sourceStand> : copy existing stand settings to stand list provided" << std::endl;
    std::cout << " softcopy <sourceStand> : copy existing stand settings but iterate through them so you can modify" << std::endl;
    std::cout << " edit <standName> : edit existing stand" << std::endl;
    std::cout << " radius <standName> : edit existing stand radius only" << std::endl;
    std::cout << " list : list all stands" << std::endl;
    std::cout << " map : generate HTML map visualization for debugging" << std::endl;
    std::cout << " save : save changes and exit" << std::endl;
    std::cout << " config : select another config (will not save current changes)" << std::endl;
    std::cout << " exit : exit without saving" << std::endl;
    std::cout << RESET;
}

void printStandInfo(const nlohmann::ordered_json &standJson)
{
    std::cout << GREY;
    if (standJson.contains("Coordinates"))
    {
        std::cout << " | Coordinates: " << standJson["Coordinates"] << "|";
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
    if (standJson.contains("Remark"))
    {
        std::cout << " Remark: ";
        for (const auto &[key, value] : standJson["Remark"].items())
        {
            std::cout << key << " : " << value << " ";
        }
        std::cout << "|";
    }
    if (standJson.contains("Wingspan"))
    {
        std::cout << " Wingspan: " << standJson["Wingspan"] << "m |";
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
    if (configJson.contains("Stands") && configJson["Stands"].is_object())
    {
        std::cout << "Current stands:" << std::endl;
        for (auto &[key, value] : configJson["Stands"].items())
        {
            std::cout << " - " << CYAN << key << RESET;
            printStandInfo(value);
        }
    }
    else
    {
        std::cout << "No stands available." << std::endl;
    }
}

void addStand(nlohmann::ordered_json &configJson, const std::string &standName)
{
    std::string standNameUpper = standName;
    std::transform(standNameUpper.begin(), standNameUpper.end(), standNameUpper.begin(), ::toupper);
    if (!configJson.contains("Stands") || !configJson["Stands"].is_object())
    {
        configJson["Stands"] = nlohmann::ordered_json::object();
    }
    if (configJson["Stands"].contains(standNameUpper))
    {
        std::cout << "Stand " << standNameUpper << " already exists." << std::endl;
    }
    else
    {
        configJson["Stands"][standNameUpper] = nlohmann::ordered_json::object();

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
                configJson["Stands"][standNameUpper]["Coordinates"] = coordinates;
                break;
            }
        }

        std::string code;
        while (true)
        {
            std::cout << "Enter code (optional): ";
            std::getline(std::cin, code);
            if (!code.empty())
            {
                if (!codeIsValid(code))
                {
                    std::cout << RED << "Invalid code format. Allowed characters combination: A,B,C,D,E,F." << RESET << std::endl;
                    continue;
                }
                std::transform(code.begin(), code.end(), code.begin(), ::toupper);
                configJson["Stands"][standNameUpper]["Code"] = code;
            }
            break;
        }

        std::string use;
        while (true)
        {
            std::cout << "Enter use (optional): ";
            std::getline(std::cin, use);
            if (!use.empty())
            {
                if (!useIsValid(use))
                {
                    std::cout << RED << "Invalid use format. Allowed characters: A, C, H, M, P." << RESET << std::endl;
                    continue;
                }
                std::transform(use.begin(), use.end(), use.begin(), ::toupper);
                configJson["Stands"][standNameUpper]["Use"] = use;
            }
            break;
        }
        std::cout << "Is it a Schengen stand? (y/n/empty): ";
        std::string schengenInput;
        std::getline(std::cin, schengenInput);
        bool schengen = (schengenInput == "y" || schengenInput == "Y");
        bool notSchengen = (schengenInput == "n" || schengenInput == "N");
        if (schengen || notSchengen)
        {
            configJson["Stands"][standNameUpper]["Schengen"] = schengen;
        }

        std::cout << "Enter callsigns (comma separated, optional): ";
        std::string callsignsInput;
        std::getline(std::cin, callsignsInput);
        if (!callsignsInput.empty())
        {
            std::vector<std::string> callsigns = splitString(callsignsInput);
            if (!callsigns.empty())
            {
                configJson["Stands"][standNameUpper]["Callsigns"] = callsigns;
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
                configJson["Stands"][standNameUpper]["Countries"] = countries;
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
                configJson["Stands"][standNameUpper]["Block"] = blocked;
            }
        }

        std::cout << "Enter Remark (format \"Code\":\"Remark\", comma separated, optional): ";
        std::string remarkInput;
        std::getline(std::cin, remarkInput);
        if (!remarkInput.empty())
        {
            std::vector<std::string> remarks = splitRemark(remarkInput);
            if (!remarks.empty())
            {
                for (const auto &remark : remarks)
                {
                    size_t colonPos = remark.find(':');
                    std::string key = remark.substr(0, colonPos);
                    key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end());
                    std::transform(key.begin(), key.end(), key.begin(), ::toupper);
                    std::string value = remark.substr(colonPos + 1);
                    configJson["Stands"][standNameUpper]["Remark"][key] = value;
                }
            }
        }

        std::cout << "Enter max Wingspan (integer, optional): ";
        std::string wingspanInput;
        while (true)
        {
            std::getline(std::cin, wingspanInput);
            if (!wingspanInput.empty())
            {
                try
                {
                    int wingspan = std::stoi(wingspanInput);
                    configJson["Stands"][standNameUpper]["Wingspan"] = wingspan;
                    break;
                }
                catch (const std::exception &e)
                {
                    std::cout << RED << "Invalid wingspan input." << RESET << std::endl;
                    std::cout << "Enter wingspan (integer, optional): ";
                    continue;
                }
            }
            else
                break;
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
                    configJson["Stands"][standNameUpper]["Priority"] = priority;
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
            configJson["Stands"][standNameUpper]["Apron"] = apron;
        }

        std::cout << "Stand " << standNameUpper << " added." << std::endl;
        printStandInfo(configJson["Stands"][standNameUpper]);
        std::cout << std::endl;
    }
}

void removeStand(nlohmann::ordered_json &configJson, const std::string &standName)
{
    std::string standNameUpper = standName;
    std::transform(standNameUpper.begin(), standNameUpper.end(), standNameUpper.begin(), ::toupper);
    if (configJson.contains("Stands") && configJson["Stands"].is_object())
    {
        if (configJson["Stands"].contains(standNameUpper))
        {
            configJson["Stands"].erase(standNameUpper);
            std::cout << RED << "Stand " << standNameUpper << " removed." << RESET << std::endl;
        }
        else
        {
            std::cout << "Stand " << standNameUpper << " does not exist." << std::endl;
        }
    }
    else
    {
        std::cout << "No stands available to remove." << std::endl;
    }
}

void editStand(nlohmann::ordered_json &configJson, const std::string &standName)
{
    std::string standNameUpper = standName;
    std::transform(standNameUpper.begin(), standNameUpper.end(), standNameUpper.begin(), ::toupper);
    if (configJson.contains("Stands") && configJson["Stands"].is_object() && configJson["Stands"].contains(standNameUpper))
    {
        nlohmann::ordered_json &standJson = configJson["Stands"][standNameUpper];
        std::cout << "Editing stand " << standNameUpper << std::endl;
        printStandInfo(standJson);

        iterateAndModifyStandSettings(standJson, standNameUpper);

        std::cout << "Stand " << standNameUpper << " updated." << std::endl;
        printStandInfo(standJson);
        std::cout << std::endl;
    }
    else
    {
        std::cout << "Stand " << standNameUpper << " does not exist." << std::endl;
    }
}

void editStandRadius(nlohmann::ordered_json &configJson, const std::string &standName)
{
    std::string standNameUpper = standName;
    std::transform(standNameUpper.begin(), standNameUpper.end(), standNameUpper.begin(), ::toupper);
    if (configJson.contains("Stands") && configJson["Stands"].is_object() && configJson["Stands"].contains(standNameUpper))
    {
        nlohmann::ordered_json &standJson = configJson["Stands"][standNameUpper];
        std::cout << "Editing radius for stand " << standNameUpper << std::endl;
        printStandInfo(standJson);

        std::string coordinatesStr = standJson["Coordinates"].get<std::string>();
        size_t lastColon = coordinatesStr.find_last_of(':');
        std::string coordinates = coordinatesStr.substr(0, lastColon);
        std::string radius = (lastColon != std::string::npos) ? coordinatesStr.substr(lastColon + 1) : "";

        std::cout << "Enter new radius (current: " << radius << ") : ";
        std::string radiusInput;
        while (true)
        {
            std::getline(std::cin, radiusInput);
            bool isPositiveNumber = !radiusInput.empty() && std::all_of(radiusInput.begin(), radiusInput.end(), ::isdigit);
            if (radiusInput.empty() || !isPositiveNumber)
            {
                std::cout << "Invalid radius format. Please enter a positive number." << std::endl;
                std::cout << "Enter new radius (current: " << radius << "): ";
                continue;
            }
            else
            {
                standJson["Coordinates"] = coordinates + ":" + radiusInput;
                break;
            }
        }
        std::cout << "Stand " << standNameUpper << " radius updated." << std::endl;
        printStandInfo(standJson);
        std::cout << std::endl;
    }
    else
    {
        std::cout << "Stand " << standNameUpper << " does not exist." << std::endl;
    }
}

void copyStand(nlohmann::ordered_json &configJson, const std::string &standName)
{
    std::string standNameUpper = standName;
    std::transform(standNameUpper.begin(), standNameUpper.end(), standNameUpper.begin(), ::toupper);
    if (configJson.contains("Stands") && configJson["Stands"].is_object() && configJson["Stands"].contains(standNameUpper))
    {
        std::string newStandName;
        std::cout << "Enter new stand name for the copy: ";
        while (true)
        {
            std::getline(std::cin, newStandName);
            std::transform(newStandName.begin(), newStandName.end(), newStandName.begin(), ::toupper);
            if (newStandName.empty())
            {
                std::cout << "New stand name cannot be empty." << std::endl;
                std::cout << "Enter new stand name for the copy: ";
                continue;
            }
            else if (configJson["Stands"].contains(newStandName))
            {
                std::cout << "Stand " << newStandName << " already exists." << std::endl;
                std::cout << "Enter new stand name for the copy: ";
                continue;
            }
            else
            {
                configJson["Stands"][newStandName] = configJson["Stands"][standNameUpper];
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
                std::cout << "Invalid coordinates format. Please use lat:lon:radius (e.g., 43.666359:7.216941:20)." << std::endl;
                std::cout << "Enter new coordinates for the copied stand (format: lat:lon:radius): ";
                continue;
            }
            else
            {
                configJson["Stands"][newStandName]["Coordinates"] = coordinates;
                break;
            }
        }
        std::cout << "Stand " << standNameUpper << " copied to " << newStandName << "." << std::endl;
        printStandInfo(configJson["Stands"][newStandName]);
        std::cout << std::endl;
    }
    else
    {
        std::cout << "Stand " << standNameUpper << " does not exist." << std::endl;
    }
}

void batchcopy(nlohmann::ordered_json &configJson, const std::string &standName)
{
    std::string standNameUpper = standName;
    std::transform(standNameUpper.begin(), standNameUpper.end(), standNameUpper.begin(), ::toupper);

    if (!configJson.contains("Stands") || !configJson["Stands"].is_object() || !configJson["Stands"].contains(standNameUpper))
    {
        std::cout << "Stand " << standNameUpper << " does not exist." << std::endl;
        return;
    }

    std::cout << "Batch copying from stand: " << standNameUpper << std::endl;
    printStandInfo(configJson["Stands"][standNameUpper]);
    std::cout << std::endl;

    std::cout << "Enter new stand entries (format: name:lat:lon:radius)" << std::endl;
    std::cout << "Example: A1:43.666359:7.216941:20" << std::endl;
    std::cout << "Press Enter on empty line to finish:" << std::endl;

    int copiedCount = 0;
    std::string line;

    while (true)
    {
        std::cout << "> ";
        std::getline(std::cin, line);

        if (line.empty())
        {
            break; // Exit on empty line
        }

        // Parse the line format: name:lat:lon:radius
        std::vector<std::string> parts;
        std::istringstream stream(line);
        std::string part;

        while (std::getline(stream, part, ':'))
        {
            parts.push_back(part);
        }

        if (parts.size() != 4)
        {
            std::cout << "Invalid format. Expected: name:lat:lon:radius" << std::endl;
            continue;
        }

        std::string newStandName = parts[0];
        std::string coordinates = parts[1] + ":" + parts[2] + ":" + parts[3];

        // Convert stand name to uppercase
        std::transform(newStandName.begin(), newStandName.end(), newStandName.begin(), ::toupper);

        // Check if stand already exists
        if (configJson["Stands"].contains(newStandName))
        {
            std::cout << "Stand " << newStandName << " already exists. Skipping." << std::endl;
            continue;
        }

        // Validate coordinates
        std::string coordsCopy = coordinates; // Make a copy since isCoordinatesValid modifies it
        if (!isCoordinatesValid(coordsCopy))
        {
            std::cout << "Invalid coordinates format for " << newStandName << ": " << coordinates << std::endl;
            std::cout << "Expected format: lat:lon:radius (e.g., 43.666359:7.216941:20)" << std::endl;
            continue;
        }

        // Copy stand settings from source
        configJson["Stands"][newStandName] = configJson["Stands"][standNameUpper];

        // Update with new coordinates
        configJson["Stands"][newStandName]["Coordinates"] = coordsCopy;

        std::cout << "Created " << newStandName << " at " << coordsCopy << std::endl;
        copiedCount++;
    }

    if (copiedCount > 0)
    {
        std::cout << std::endl
                  << "Batch copy completed! Created " << copiedCount << " new stands based on " << standNameUpper << "." << std::endl;
    }
    else
    {
        std::cout << "No stands were created." << std::endl;
    }
}

void softStandCopy(nlohmann::ordered_json &configJson, const std::string &standName)
{
    std::string standNameUpper = standName;
    std::transform(standNameUpper.begin(), standNameUpper.end(), standNameUpper.begin(), ::toupper);
    if (configJson.contains("Stands") && configJson["Stands"].is_object() && configJson["Stands"].contains(standNameUpper))
    {
        std::string newStandName;
        std::cout << "Enter new stand name for the copy: ";
        while (true)
        {
            std::getline(std::cin, newStandName);
            std::transform(newStandName.begin(), newStandName.end(), newStandName.begin(), ::toupper);
            if (newStandName.empty())
            {
                std::cout << "New stand name cannot be empty." << std::endl;
                std::cout << "Enter new stand name for the copy: ";
                continue;
            }
            else if (configJson["Stands"].contains(newStandName))
            {
                std::cout << "Stand " << newStandName << " already exists." << std::endl;
                std::cout << "Enter new stand name for the copy: ";
                continue;
            }
            else
            {
                configJson["Stands"][newStandName] = configJson["Stands"][standNameUpper];
                break;
            }
        }

        iterateAndModifyStandSettings(configJson["Stands"][newStandName], newStandName);

        std::cout << "Stand " << newStandName << " added." << std::endl;
        printStandInfo(configJson["Stands"][newStandName]);
        std::cout << std::endl;
    }
    else
    {
        std::cout << "Stand " << standNameUpper << " does not exist." << std::endl;
    }
}

void iterateAndModifyStandSettings(nlohmann::ordered_json &configJson, const std::string &newStandName)
{
    std::cout << "Enter new coordinates (format: lat:lon:radius), empty to keep: ";
    std::string coordinates;
    while (true)
    {
        std::getline(std::cin, coordinates);
        if (coordinates.empty())
        {
            break; // Keep current
        }
        else if (!isCoordinatesValid(coordinates))
        {
            std::cout << RED << "Invalid coordinates format. Please use lat:lon:radius (e.g., 43.666359:7.216941:20)." << RESET << std::endl;
            std::cout << "Enter new coordinates (format: lat:lon:radius): ";
            continue;
        }
        else
        {
            configJson["Coordinates"] = coordinates;
            break;
        }
    }

    std::cout << "Enter new code (current: " << (configJson.contains("Code") ? configJson["Code"].get<std::string>() : "none") << ", empty to keep, r to remove): ";
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
            configJson.erase("Code");
            break;
        }
        else
        {
            std::transform(code.begin(), code.end(), code.begin(), ::toupper);
            if (codeIsValid(code) == false)
            {
                std::cout << RED << "Invalid code format. Allowed characters combination: A,B,C,D,E,F." << RESET << std::endl;
                std::cout << "Enter new code (empty to keep, r to remove): ";
                continue;
            }
            configJson["Code"] = code;
            break;
        }
    }

    std::cout << "Enter new use (current: " << (configJson.contains("Use") ? configJson["Use"].get<std::string>() : "none") << ", single character, empty to keep, r to remove): ";
    std::string use;
    while (true)
    {
        std::getline(std::cin, use);
        if (use.empty())
        {
            break; // Keep current
        }
        else if (use == "r" || use == "R")
        {
            configJson.erase("Use");
            break;
        }
        else
        {
            if (useIsValid(use) == false)
            {
                std::cout << RED << "Invalid use format. Allowed characters: A, C, H, M, P." << RESET << std::endl;
                std::cout << "Enter new use (empty to keep, r to remove): ";
                continue;
            }
            std::transform(use.begin(), use.end(), use.begin(), ::toupper);
            configJson["Use"] = use;
            break;
        }
    }

    std::cout << "Is it a Schengen stand? (current: " << (configJson.contains("Schengen") ? (configJson["Schengen"].get<bool>() ? "Yes" : "No") : "none") << " Y/N, empty to keep, r to remove): ";
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
            configJson.erase("Schengen");
            break;
        }
        else if (schengen == "Y" || schengen == "y")
        {
            configJson["Schengen"] = true;
            break;
        }
        else if (schengen == "N" || schengen == "n")
        {
            configJson["Schengen"] = false;
            break;
        }
        else
        {
            std::cout << RED << "Invalid input. Please enter 'Y', 'N', 'R' to remove or leave empty to keep." << RESET << std::endl;
        }
    }

    std::cout << "Enter new callsigns (current: ";
    if (configJson.contains("Callsigns"))
    {
        for (const auto &cs : configJson["Callsigns"])
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
            configJson.erase("Callsigns");
            break;
        }
        else
        {
            std::vector<std::string> callsigns = splitString(callsignsInput);
            if (!callsigns.empty())
            {
                configJson["Callsigns"] = callsigns;
            }
            else
            {
                configJson.erase("Callsigns");
            }
            break;
        }
    }

    std::cout << "Enter new countries (current: ";
    if (configJson.contains("Countries"))
    {
        for (const auto &country : configJson["Countries"])
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
            configJson.erase("Countries");
            break;
        }
        else
        {
            std::vector<std::string> countries = splitString(countriesInput);
            if (!countries.empty())
            {
                configJson["Countries"] = countries;
            }
            else
            {
                configJson.erase("Countries");
            }
            break;
        }
    }

    std::cout << "Enter new blocked stands (current: ";
    if (configJson.contains("Block"))
    {
        for (const auto &blk : configJson["Block"])
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
            configJson.erase("Block");
            break;
        }
        else
        {
            std::vector<std::string> blocked = splitString(blockInput);
            if (!blocked.empty())
            {
                configJson["Block"] = blocked;
            }
            else
            {
                configJson.erase("Block");
            }
            break;
        }
    }

    std::cout << "Enter new Remark (current: ";
    if (configJson.contains("Remark"))
    {
        for (const auto &[key, value] : configJson["Remark"].items())
        {
            std::cout << key << " : " << value << " ";
        }
    }
    else
    {
        std::cout << "none";
    }
    std::cout << ", format \"Code\":\"Remark\", comma separated, empty to keep, r to remove): ";
    std::string remarkInput;
    while (true)
    {
        std::getline(std::cin, remarkInput);
        if (remarkInput.empty())
        {
            break; // Keep current
        }
        else if (remarkInput == "r" || remarkInput == "R")
        {
            configJson.erase("Remark");
            break;
        }
        else
        {
            std::vector<std::string> remarks = splitRemark(remarkInput);
            if (!remarks.empty())
            {
                nlohmann::ordered_json newRemarks;
                for (const auto &remark : remarks)
                {
                    size_t colonPos = remark.find(':');
                    std::string key = remark.substr(0, colonPos);
                    key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end());
                    std::transform(key.begin(), key.end(), key.begin(), ::toupper);
                    std::string value = remark.substr(colonPos + 1);
                    newRemarks[key] = value;
                }
                configJson["Remark"] = newRemarks;
            }
            else
            {
                configJson.erase("Remark");
            }
            break;
        }
    }

    std::cout << "Enter new max Wingspan (current: " << (configJson.contains("Wingspan") ? std::to_string(configJson["Wingspan"].get<int>()) : "none") << ", integer, empty to keep, r to remove): ";
    std::string wingspanInput;
    while (true)
    {
        std::getline(std::cin, wingspanInput);
        if (wingspanInput.empty())
        {
            break; // Keep current
        }
        else if (wingspanInput == "r" || wingspanInput == "R")
        {
            configJson.erase("Wingspan");
            break;
        }
        else
        {
            try
            {
                int wingspan = std::stoi(wingspanInput);
                configJson["Wingspan"] = wingspan;
                break;
            }
            catch (const std::exception &e)
            {
                std::cout << RED << "Invalid wingspan input." << RESET << std::endl;
                std::cout << "Enter new max Wingspan (integer, empty to keep/remove): ";
                continue;
            }
        }
    }

    std::cout << "Enter new priority (current: " << (configJson.contains("Priority") ? std::to_string(configJson["Priority"].get<int>()) : "none") << ", integer, empty to keep, r to remove): ";
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
            configJson.erase("Priority");
            break;
        }
        else
        {
            try
            {
                int priority = std::stoi(priorityInput);
                configJson["Priority"] = priority;
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

    std::cout << "Is it an apron stand? (current: " << (configJson.contains("Apron") ? (configJson["Apron"].get<bool>() ? "Yes" : "No") : "No") << " Y if apron, empty to keep, r to remove): ";
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
            configJson.erase("Apron");
            break;
        }
        else if (apronInput == "Y" || apronInput == "y")
        {
            configJson["Apron"] = true;
            break;
        }
        else
        {
            std::cout << RED << "Invalid input. Please enter 'Y', 'N', 'R' to remove or leave empty to keep." << RESET << std::endl;
        }
    }
}
