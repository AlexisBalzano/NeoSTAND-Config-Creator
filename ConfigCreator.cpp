#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <algorithm>
#include <sstream>
#include <vector>
#include <regex>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include "nlohmann/json.hpp"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #ifdef __APPLE__
        #include <mach-o/dyld.h>
    #endif
#endif

constexpr auto version = "v1.0.4";

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

// Live reload server using Python's built-in HTTP server
class LiveReloadServer {
private:
    std::atomic<bool> running{false};
    std::thread serverThread;
    std::thread watchThread;
    std::string mapFilePath;
    std::filesystem::file_time_type lastModified;
    int port;
    
public:
    LiveReloadServer() : port(4000) {}
    
    void startServer(const std::string& mapFile) {
        if (running.load()) return;
        
        mapFilePath = mapFile;
        if (std::filesystem::exists(mapFile)) {
            lastModified = std::filesystem::last_write_time(mapFile);
        }
        
        running.store(true);
        
        // Start Python HTTP server
        serverThread = std::thread([this]() {
            std::string baseDir = std::filesystem::path(mapFilePath).parent_path().string();
#ifdef _WIN32
            std::string command = "cd \"" + baseDir + "\" && py -m http.server " + std::to_string(port) + " 2>nul";
#else
            std::string command = "cd \"" + baseDir + "\" && python -m http.server " + std::to_string(port) + " 2>nul";
#endif
            std::system(command.c_str());
        });
        
        // Start file watcher
        watchThread = std::thread([this]() {
            watchForChanges();
        });
        
        std::cout << CYAN << "🚀 Live reload server started at http://localhost:" << port << RESET << std::endl;
        std::cout << YELLOW << "💡 Your map will auto-reload when you make changes!" << RESET << std::endl;
    }
    
    void stop() {
        running.store(false);
        if (serverThread.joinable()) {
            serverThread.join();
        }
        if (watchThread.joinable()) {
            watchThread.join();
        }
    }
    
    int getPort() const { return port; }
    
private:
    void watchForChanges() {
        while (running.load()) {
            if (!mapFilePath.empty() && std::filesystem::exists(mapFilePath)) {
                auto currentModified = std::filesystem::last_write_time(mapFilePath);
                if (currentModified > lastModified) {
                    lastModified = currentModified;
                    // Create a simple signal file that the browser can poll
                    createReloadSignal();
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    
    void createReloadSignal() {
        std::string signalFile = std::filesystem::path(mapFilePath).parent_path().string() + "/reload_signal.txt";
        std::ofstream signal(signalFile);
        if (signal) {
            signal << std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            signal.close();
        }
    }
};

// Global server instance
static std::unique_ptr<LiveReloadServer> g_liveServer = nullptr;

bool isCoordinatesValid(std::string &coordinates);


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

std::string getExecutableDir() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string executablePath = buffer;
    size_t pos = executablePath.find_last_of("\\/");
    return (pos == std::string::npos) ? "" : executablePath.substr(0, pos + 1);
#else
    char buffer[1024];
    ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (count == -1) {
        // Fallback for macOS
        uint32_t size = sizeof(buffer);
        if (_NSGetExecutablePath(buffer, &size) != 0) {
            return "./"; // Fallback to current directory
        }
    } else {
        buffer[count] = '\0';
    }
    std::string executablePath = buffer;
    size_t pos = executablePath.find_last_of('/');
    return (pos == std::string::npos) ? "./" : executablePath.substr(0, pos + 1);
#endif
}

// Update the getBaseDir function
std::string getBaseDir() {
    std::string execDir = getExecutableDir();
    return execDir + "configs/";
}

bool getConfig(const std::string &icao, nlohmann::ordered_json &configJson)
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
        std::string coordinates;
        std::cout << "Enter airport coordinates (format: lat:lon:radius): ";
        while (true)
        {
            std::getline(std::cin, coordinates);
            if (!isCoordinatesValid(coordinates))
            {
                std::cout << RED << "Invalid coordinates format. Please use lat:lon:radius (e.g., 43.666359:7.216941:20)." << RESET << std::endl;
                std::cout << "Enter airport coordinates (format: lat:lon:radius): ";
                continue;
            }
            else
            {
                break;
            }
        }

        // Create default config
        configJson = {
            {"ICAO", icao},
            {"Coordinates", coordinates},
            {"Stands", {}}};
        std::cout << "Created default config structure." << std::endl;
    }

    return true;
}

std::vector<std::pair<std::string, std::string>> parseStandName(const std::string& standName) {
    std::vector<std::pair<std::string, std::string>> parts;
    std::string current = "";
    bool isNumber = false;
    
    for (char c : standName) {
        bool currentIsDigit = std::isdigit(c);
        
        if (currentIsDigit != isNumber && !current.empty()) {
            parts.push_back({current, isNumber ? "number" : "text"});
            current = "";
        }
        
        current += c;
        isNumber = currentIsDigit;
    }
    
    if (!current.empty()) {
        parts.push_back({current, isNumber ? "number" : "text"});
    }
    
    return parts;
}

bool naturalSort(const std::string& a, const std::string& b) {
    auto partsA = parseStandName(a);
    auto partsB = parseStandName(b);
    
    size_t minSize = std::min(partsA.size(), partsB.size());
    
    for (size_t i = 0; i < minSize; ++i) {
        const auto& partA = partsA[i];
        const auto& partB = partsB[i];
        
        // If both are numbers, compare numerically
        if (partA.second == "number" && partB.second == "number") {
            int numA = std::stoi(partA.first);
            int numB = std::stoi(partB.first);
            if (numA != numB) {
                return numA < numB;
            }
        }
        // If both are text, compare lexicographically
        else if (partA.second == "text" && partB.second == "text") {
            if (partA.first != partB.first) {
                return partA.first < partB.first;
            }
        }
        // If types differ, numbers come before text
        else {
            return partA.second == "number" && partB.second == "text";
        }
    }
    
    // If all compared parts are equal, shorter string comes first
    return partsA.size() < partsB.size();
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
    std::cout << " batchcopy <sourceStand> : copy existing stand settings to stand list provided" << std::endl;
    std::cout << " softcopy <sourceStand> : copy existing stand settings but iterate through them so you can modify" << std::endl;
    std::cout << " edit <standName> : edit existing stand" << std::endl;
    std::cout << " list : list all stands" << std::endl;
    std::cout << " map : generate HTML map visualization for debugging" << std::endl;
    std::cout << " save : save changes and exit" << std::endl;
    std::cout << " config : select another config (will not save current changes)" << std::endl;
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
    if (configJson.contains("Stands") && configJson["Stands"].is_object())
    {
        std::cout << "Current stands:" << std::endl;
        for (auto &[key, value] : configJson["Stands"].items())
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

bool isCoordinatesValid(std::string &coordinates)
{
    // Remove "COORD:" prefix if present
    if (coordinates.substr(0, 6) == "COORD:")
    {
        coordinates = coordinates.substr(6);
    }
    
    // Basic validation of Degree decimal format
    size_t firstColon = coordinates.find(':');
    size_t secondColon = coordinates.find(':', firstColon + 1);
    if (firstColon == std::string::npos || secondColon == std::string::npos)
    {
        return false;
    }
    
    // Check if already in decimal format - allow empty radius
    std::regex degreeDecimalRegex(R"(([-+]?\d{1,3}\.\d+):([-+]?\d{1,3}\.\d+):(\d*))");
    if (std::regex_match(coordinates, degreeDecimalRegex))
    {
        // Valid format : 43.666359:7.216941:20
        std::string lat = coordinates.substr(0, firstColon);
        std::string lon = coordinates.substr(firstColon + 1, secondColon - firstColon - 1);
        std::string radius = coordinates.substr(secondColon + 1);
        try
        {
            double latVal = std::stod(lat);
            double lonVal = std::stod(lon);
            if (latVal < -90 || latVal > 90 || lonVal < -180 || lonVal > 180)
            {
                return false;
            }
            // Validate radius if provided
            if (!radius.empty())
            {
                double radiusVal = std::stod(radius);
                if (radiusVal < 0)
                {
                    return false;
                }
            }
            return true;
        }
        catch (...)
        {
            return false;
        }
    }
    
    // Try to convert from DMS format like N043.37.40.861:E001.22.36.064:25 or COORD:N043.37.40.861:E001.22.36.064:25
    std::regex dmsRegex(R"([NS](\d{3})\.(\d{2})\.(\d{2})\.(\d{3}):[EW](\d{3})\.(\d{2})\.(\d{2})\.(\d{3}):(\d+))");
    std::smatch match;
    if (std::regex_match(coordinates, match, dmsRegex))
    {
        // Convert latitude
        int latDeg = std::stoi(match[1]);
        int latMin = std::stoi(match[2]);
        int latSec = std::stoi(match[3]);
        int latMillisec = std::stoi(match[4]);
        double latDecimal = latDeg + latMin / 60.0 + (latSec + latMillisec / 1000.0) / 3600.0;
        if (coordinates[0] == 'S') latDecimal = -latDecimal;
        
        // Convert longitude
        int lonDeg = std::stoi(match[5]);
        int lonMin = std::stoi(match[6]);
        int lonSec = std::stoi(match[7]);
        int lonMillisec = std::stoi(match[8]);
        double lonDecimal = lonDeg + lonMin / 60.0 + (lonSec + lonMillisec / 1000.0) / 3600.0;
        if (coordinates.find("W") != std::string::npos) lonDecimal = -lonDecimal;
        
        std::string radius = match[9].str();
        
        // Validate ranges
        if (latDecimal < -90 || latDecimal > 90 || lonDecimal < -180 || lonDecimal > 180)
        {
            return false;
        }
        
        // Update coordinates to decimal format
        coordinates = std::to_string(latDecimal) + ":" + std::to_string(lonDecimal) + ":" + radius;
        return true;
    }
    
    return false;
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
        std::cout << RED << "Stand " << standNameUpper << " already exists." << RESET << std::endl;
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

        std::cout << "Enter code (optional): ";
        std::string code;
        while (true)
        {
            std::getline(std::cin, code);
            if (!code.empty())
            {
                std::transform(code.begin(), code.end(), code.begin(), ::toupper);
                configJson["Stands"][standNameUpper]["Code"] = code;
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
            if (!use.empty())
            {
                std::transform(use.begin(), use.end(), use.begin(), ::toupper);
                configJson["Stands"][standNameUpper]["Use"] = use;
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

        std::cout << "Stand " << BLUE << standNameUpper << " added." << RESET;
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
    if (configJson.contains("Stands") && configJson["Stands"].is_object() && configJson["Stands"].contains(standNameUpper))
    {
        nlohmann::ordered_json &standJson = configJson["Stands"][standNameUpper];
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
            if (use.empty())
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
                std::cout << RED << "New stand name cannot be empty." << RESET << std::endl;
                std::cout << "Enter new stand name for the copy: ";
                continue;
            }
            else if (configJson["Stands"].contains(newStandName))
            {
                std::cout << RED << "Stand " << newStandName << " already exists." << RESET << std::endl;
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
                std::cout << RED << "Invalid coordinates format. Please use lat:lon:radius (e.g., 43.666359:7.216941:20)." << RESET << std::endl;
                std::cout << "Enter new coordinates for the copied stand (format: lat:lon:radius): ";
                continue;
            }
            else
            {
                configJson["Stands"][newStandName]["Coordinates"] = coordinates;
                break;
            }
        }
        std::cout << GREEN << "Stand " << standNameUpper << " copied to " << newStandName << "." << RESET << std::endl;
        printStandInfo(configJson["Stands"][newStandName]);
        std::cout << std::endl;
    }
    else
    {
        std::cout << RED << "Stand " << standNameUpper << " does not exist." << RESET << std::endl;
    }
}

void batchcopy(nlohmann::ordered_json &configJson, const std::string &standName) {
    std::string standNameUpper = standName;
    std::transform(standNameUpper.begin(), standNameUpper.end(), standNameUpper.begin(), ::toupper);
    
    if (!configJson.contains("Stands") || !configJson["Stands"].is_object() || !configJson["Stands"].contains(standNameUpper))
    {
        std::cout << RED << "Stand " << standNameUpper << " does not exist." << RESET << std::endl;
        return;
    }

    std::cout << GREEN << "Batch copying from stand: " << standNameUpper << RESET << std::endl;
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
        
        if (line.empty()) {
            break; // Exit on empty line
        }
        
        // Parse the line format: name:lat:lon:radius
        std::vector<std::string> parts;
        std::istringstream stream(line);
        std::string part;
        
        while (std::getline(stream, part, ':')) {
            parts.push_back(part);
        }
        
        if (parts.size() != 4) {
            std::cout << RED << "Invalid format. Expected: name:lat:lon:radius" << RESET << std::endl;
            continue;
        }
        
        std::string newStandName = parts[0];
        std::string coordinates = parts[1] + ":" + parts[2] + ":" + parts[3];
        
        // Convert stand name to uppercase
        std::transform(newStandName.begin(), newStandName.end(), newStandName.begin(), ::toupper);
        
        // Check if stand already exists
        if (configJson["Stands"].contains(newStandName))
        {
            std::cout << RED << "Stand " << newStandName << " already exists. Skipping." << RESET << std::endl;
            continue;
        }
        
        // Validate coordinates
        std::string coordsCopy = coordinates; // Make a copy since isCoordinatesValid modifies it
        if (!isCoordinatesValid(coordsCopy))
        {
            std::cout << RED << "Invalid coordinates format for " << newStandName << ": " << coordinates << RESET << std::endl;
            std::cout << "Expected format: lat:lon:radius (e.g., 43.666359:7.216941:20)" << std::endl;
            continue;
        }
        
        // Copy stand settings from source
        configJson["Stands"][newStandName] = configJson["Stands"][standNameUpper];
        
        // Update with new coordinates
        configJson["Stands"][newStandName]["Coordinates"] = coordsCopy;
        
        std::cout << GREEN << "✓ Created " << newStandName << " at " << coordsCopy << RESET << std::endl;
        copiedCount++;
    }
    
    if (copiedCount > 0)
    {
        std::cout << std::endl << GREEN << "Batch copy completed! Created " << copiedCount << " new stands based on " << standNameUpper << "." << RESET << std::endl;
    }
    else
    {
        std::cout << YELLOW << "No stands were created." << RESET << std::endl;
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
                std::cout << RED << "New stand name cannot be empty." << RESET << std::endl;
                std::cout << "Enter new stand name for the copy: ";
                continue;
            }
            else if (configJson["Stands"].contains(newStandName))
            {
                std::cout << RED << "Stand " << newStandName << " already exists." << RESET << std::endl;
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
                std::cout << RED << "Invalid coordinates format. Please use lat:lon:radius (e.g., 43.666359:7.216941:20)." << RESET << std::endl;
                std::cout << "Enter new coordinates for the copied stand (format: lat:lon:radius): ";
                continue;
            }
            else
            {
                configJson["Stands"][newStandName]["Coordinates"] = coordinates;
                break;
            }
        }

        nlohmann::ordered_json &standJson = configJson["Stands"][newStandName];
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
            if (use.empty())
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

void generateMap(const nlohmann::ordered_json &configJson, const std::string &icao, bool openBrowser = true)
{
    if (!configJson.contains("Stands") || !configJson["Stands"].is_object() || configJson["Stands"].empty())
    {
        std::cout << RED << "No stands available to visualize." << RESET << std::endl;
        return;
    }

    std::string filename = getBaseDir() + icao + "_map.html";
    std::ofstream htmlFile(filename);
    
    if (!htmlFile)
    {
        std::cout << RED << "Error creating HTML file." << RESET << std::endl;
        return;
    }

    // Calculate center coordinates from all stands
    double totalLat = 0, totalLon = 0;
    int validStands = 0;
    
    for (auto &[standName, standData] : configJson["Stands"].items())
    {
        if (standData.contains("Coordinates"))
        {
            std::string coords = standData["Coordinates"];
            size_t firstColon = coords.find(':');
            size_t secondColon = coords.find(':', firstColon + 1);
            if (firstColon != std::string::npos && secondColon != std::string::npos)
            {
                try
                {
                    double lat = std::stod(coords.substr(0, firstColon));
                    double lon = std::stod(coords.substr(firstColon + 1, secondColon - firstColon - 1));
                    totalLat += lat;
                    totalLon += lon;
                    validStands++;
                }
                catch (...) { /* Skip invalid coordinates */ }
            }
        }
    }
    
    double centerLat = validStands > 0 ? totalLat / validStands : 0;
    double centerLon = validStands > 0 ? totalLon / validStands : 0;

    htmlFile << R"(<!DOCTYPE html>
<html>
<head>
    <title>)" << icao << R"( - Airport Stands Debug Map</title>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
    <style>
        #map { height: 100vh; width: 100%; }
        .stand-info { font-weight: bold; }
        .legend { 
            background: white; 
            padding: 10px; 
            border-radius: 5px; 
            box-shadow: 0 2px 5px rgba(0,0,0,0.2);
        }
    </style>
</head>
<body>
    <div id="map"></div>
    <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
    <script>
        var map = L.map('map', {
            maxZoom: 19  // Increase maximum zoom level
        }).setView([)" << centerLat << ", " << centerLon << R"(], 16);
        
        // Add satellite tile layer
        L.tileLayer('https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}', {
            attribution: 'Tiles &copy; Esri &mdash; Source: Esri, i-cubed, USDA, USGS, AEX, GeoEye, Getmapping, Aerogrid, IGN, IGP, UPR-EGP, and the GIS User Community',
            maxZoom: 19  // Set tile layer max zoom
        }).addTo(map);
        
        // Add OpenStreetMap overlay for airport details
        var osmLayer = L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
            attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors',
            opacity: 0.3,
            maxZoom: 19  // OSM has lower max zoom
        }).addTo(map);
        
        // Store references to current stands for cleanup
        var currentStandElements = [];
        
        // Color function for different stand types
        function getStandColor(standData) {
            if (standData.Apron) return '#FF6B6B';  // Red for apron
            if (standData.Schengen === false) return '#4ECDC4';  // Teal for non-Schengen
            if (standData.Schengen === true) return '#45B7D1';   // Blue for Schengen
            return '#96CEB4';  // Green for default
        }
        
        function getStandOpacity(standData) {
            return standData.Use ? 0.4 : 0.2;
        }
)";

    // Add stands to the map
    for (auto &[standName, standData] : configJson["Stands"].items())
    {
        if (standData.contains("Coordinates"))
        {
            std::string coords = standData["Coordinates"];
            size_t firstColon = coords.find(':');
            size_t secondColon = coords.find(':', firstColon + 1);
            if (firstColon != std::string::npos && secondColon != std::string::npos)
            {
                try
                {
                    double lat = std::stod(coords.substr(0, firstColon));
                    double lon = std::stod(coords.substr(firstColon + 1, secondColon - firstColon - 1));
                    std::string radiusStr = coords.substr(secondColon + 1);
                    double radius = radiusStr.empty() ? 20 : std::stod(radiusStr);
                    
                    htmlFile << "        // Stand " << standName << "\n";
                    htmlFile << "        var stand_" << standName << " = {\n";
                    htmlFile << "            name: '" << standName << "',\n";
                    htmlFile << "            lat: " << lat << ",\n";
                    htmlFile << "            lon: " << lon << ",\n";
                    htmlFile << "            radius: " << radius << ",\n";
                    if (standData.contains("Code"))
                        htmlFile << "            code: '" << standData["Code"] << "',\n";
                    if (standData.contains("Use"))
                        htmlFile << "            use: '" << standData["Use"] << "',\n";
                    if (standData.contains("Schengen"))
                        htmlFile << "            Schengen: " << (standData["Schengen"].get<bool>() ? "true" : "false") << ",\n";
                    if (standData.contains("Apron"))
                        htmlFile << "            Apron: " << (standData["Apron"].get<bool>() ? "true" : "false") << ",\n";
                    if (standData.contains("Priority"))
                        htmlFile << "            Priority: " << standData["Priority"] << ",\n";
                    htmlFile << "        };\n";
                    
                    htmlFile << "        var circle_" << standName << " = L.circle([" << lat << ", " << lon << "], {\n";
                    htmlFile << "            radius: " << radius << ",\n";
                    htmlFile << "            color: getStandColor(stand_" << standName << "),\n";
                    htmlFile << "            fillColor: getStandColor(stand_" << standName << "),\n";
                    htmlFile << "            fillOpacity: getStandOpacity(stand_" << standName << ")\n";
                    htmlFile << "        }).addTo(map);\n";
                    htmlFile << "        currentStandElements.push(circle_" << standName << ");\n";
                    
                    // Create popup content
                    htmlFile << "        var popupContent_" << standName << " = '<div class=\"stand-info\">Stand: " << standName << "</div>';\n";
                    if (standData.contains("Code"))
                        htmlFile << "        popupContent_" << standName << " += '<br>Code: " << standData["Code"] << "';\n";
                    if (standData.contains("Use"))
                        htmlFile << "        popupContent_" << standName << " += '<br>Use: " << standData["Use"] << "';\n";
                    if (standData.contains("Schengen"))
                        htmlFile << "        popupContent_" << standName << " += '<br>Schengen: " << (standData["Schengen"].get<bool>() ? "Yes" : "No") << "';\n";
                    if (standData.contains("Apron"))
                        htmlFile << "        popupContent_" << standName << " += '<br>Apron: " << (standData["Apron"].get<bool>() ? "Yes" : "No") << "';\n";
                    if (standData.contains("Priority"))
                        htmlFile << "        popupContent_" << standName << " += '<br>Priority: " << standData["Priority"] << "';\n";
                    htmlFile << "        popupContent_" << standName << " += '<br>Radius: " << radius << "m';\n";
                    htmlFile << "        popupContent_" << standName << " += '<br>Coordinates: " << coords << "';\n";
                    
                    // Add arrays if they exist
                    if (standData.contains("Callsigns") && standData["Callsigns"].is_array())
                    {
                        htmlFile << "        popupContent_" << standName << " += '<br>Callsigns: ";
                        for (auto it = standData["Callsigns"].begin(); it != standData["Callsigns"].end(); ++it)
                        {
                            if (it != standData["Callsigns"].begin()) htmlFile << ", ";
                            htmlFile << *it;
                        }
                        htmlFile << "';\n";
                    }
                    
                    if (standData.contains("Countries") && standData["Countries"].is_array())
                    {
                        htmlFile << "        popupContent_" << standName << " += '<br>Countries: ";
                        for (auto it = standData["Countries"].begin(); it != standData["Countries"].end(); ++it)
                        {
                            if (it != standData["Countries"].begin()) htmlFile << ", ";
                            htmlFile << *it;
                        }
                        htmlFile << "';\n";
                    }
                    
                    if (standData.contains("Block") && standData["Block"].is_array())
                    {
                        htmlFile << "        popupContent_" << standName << " += '<br>Blocked: ";
                        for (auto it = standData["Block"].begin(); it != standData["Block"].end(); ++it)
                        {
                            if (it != standData["Block"].begin()) htmlFile << ", ";
                            htmlFile << *it;
                        }
                        htmlFile << "';\n";
                    }
                    
                    htmlFile << "        circle_" << standName << ".bindPopup(popupContent_" << standName << ");\n";
                    
                    // Add click event to circle for coordinate copying
                    htmlFile << "        circle_" << standName << ".on('click', function(e) {\n";
                    htmlFile << "            var lat = e.latlng.lat.toFixed(6);\n";
                    htmlFile << "            var lng = e.latlng.lng.toFixed(6);\n";
                    htmlFile << "            var coordString = lat + ':' + lng;\n";
                    htmlFile << "            \n";
                    htmlFile << "            // Copy to clipboard\n";
                    htmlFile << "            if (navigator.clipboard && window.isSecureContext) {\n";
                    htmlFile << "                navigator.clipboard.writeText(coordString).then(function() {\n";
                    htmlFile << "                    console.log('Coordinates copied: ' + coordString);\n";
                    htmlFile << "                }).catch(function(err) {\n";
                    htmlFile << "                    console.error('Failed to copy coordinates: ', err);\n";
                    htmlFile << "                });\n";
                    htmlFile << "            } else {\n";
                    htmlFile << "                // Fallback for older browsers\n";
                    htmlFile << "                var textArea = document.createElement('textarea');\n";
                    htmlFile << "                textArea.value = coordString;\n";
                    htmlFile << "                document.body.appendChild(textArea);\n";
                    htmlFile << "                textArea.select();\n";
                    htmlFile << "                document.execCommand('copy');\n";
                    htmlFile << "                document.body.removeChild(textArea);\n";
                    htmlFile << "            }\n";
                    htmlFile << "            // Don't prevent the popup from showing\n";
                    htmlFile << "        });\n\n";

                    // Calculate width based on stand name length
                    int labelWidth = std::max(30, static_cast<int>(standName.length()) * 8);


                    // Add stand label
                    htmlFile << "        var marker_" << standName << " = L.marker([" << lat << ", " << lon << "], {\n";
                    htmlFile << "            icon: L.divIcon({\n";
                    htmlFile << "                className: 'stand-label',\n";
                    htmlFile << "                html: '<div style=\"background-color: rgba(255,255,255,0.8); padding: 2px 4px; border-radius: 3px; font-weight: bold; font-size: 12px; color: black; text-align: center; display: flex; align-items: center; justify-content: center; width: 100%; height: 100%; box-sizing: border-box;\">" << standName << "</div>',\n";
                    htmlFile << "                iconSize: [" << labelWidth << ", 20],\n";
                    htmlFile << "                iconAnchor: [" << labelWidth / 2 << ", 10]\n";
                    htmlFile << "            })\n";
                    htmlFile << "        }).addTo(map);\n";
                    htmlFile << "        currentStandElements.push(marker_" << standName << ");\n\n";
                }
                catch (...)
                {
                    std::cout << YELLOW << "Warning: Invalid coordinates for stand " << standName << RESET << std::endl;
                }
            }
        }
    }

    // Generate timestamp for JavaScript use
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    
    htmlFile << R"(        
        // Add legend
        var legend = L.control({position: 'topright'});
        legend.onAdd = function (map) {
            var div = L.DomUtil.create('div', 'legend');
            div.innerHTML = '<h4>)" << icao << R"( Stands Legend</h4>' +
                '<i style="background:#96CEB4; width:18px; height:18px; float:left; margin-right:8px; opacity:0.7; border-radius:50%;"></i> Default<br>' +
                '<i style="background:#45B7D1; width:18px; height:18px; float:left; margin-right:8px; opacity:0.7; border-radius:50%;"></i> Schengen<br>' +
                '<i style="background:#4ECDC4; width:18px; height:18px; float:left; margin-right:8px; opacity:0.7; border-radius:50%;"></i> Non-Schengen<br>' +
                '<i style="background:#FF6B6B; width:18px; height:18px; float:left; margin-right:8px; opacity:0.7; border-radius:50%;"></i> Apron<br><br>' +
                '<div style="background:#e8f5e8; padding:4px; border-radius:3px; margin:5px 0;"><strong>🔄 Auto-reload: ON</strong><br><small>Smart updates</small></div>' +
                '<small>Click circles for details<br>Click map to copy coordinates</small>';
            return div;
        };
        legend.addTo(map);
        
        // Add click event to copy coordinates to clipboard
        map.on('click', function(e) {
            var lat = e.latlng.lat.toFixed(6);
            var lng = e.latlng.lng.toFixed(6);
            var coordString = lat + ':' + lng;
            
            // Copy to clipboard
            if (navigator.clipboard && window.isSecureContext) {
                navigator.clipboard.writeText(coordString).then(function() {
                    // Show temporary popup at click location
                    var popup = L.popup()
                        .setLatLng(e.latlng)
                        .setContent('<div style="text-align: center;"><strong>Coordinates copied!</strong><br>' + coordString + '</div>')
                        .openOn(map);
                    
                    // Auto-close popup after 2 seconds
                    setTimeout(function() {
                        map.closePopup(popup);
                    }, 2000);
                }).catch(function(err) {
                    console.error('Failed to copy coordinates: ', err);
                    // Fallback method
                    fallbackCopyTextToClipboard(coordString, e.latlng);
                });
            } else {
                // Fallback for older browsers
                fallbackCopyTextToClipboard(coordString, e.latlng);
            }
        });
        
        // Fallback copy function for older browsers
        function fallbackCopyTextToClipboard(text, latlng) {
            var textArea = document.createElement("textarea");
            textArea.value = text;
            
            // Avoid scrolling to bottom
            textArea.style.top = "0";
            textArea.style.left = "0";
            textArea.style.position = "fixed";
            
            document.body.appendChild(textArea);
            textArea.focus();
            textArea.select();
            
            try {
                var successful = document.execCommand('copy');
                var popup = L.popup()
                    .setLatLng(latlng)
                    .setContent('<div style="text-align: center;"><strong>' + 
                               (successful ? 'Coordinates copied!' : 'Copy failed - please copy manually') + 
                               '</strong><br>' + text + '</div>')
                    .openOn(map);
                
                setTimeout(function() {
                    map.closePopup(popup);
                }, 2000);
            } catch (err) {
                console.error('Fallback: Oops, unable to copy', err);
            }
            
            document.body.removeChild(textArea);
        }
        
        // Add layer control
        var baseMaps = {
            "Satellite": L.tileLayer('https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}'),
            "OpenStreetMap": L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png')
        };
        L.control.layers(baseMaps).addTo(map);
        
        // Live reload system for localhost server
        if (window.location.protocol === 'http:' && window.location.hostname === 'localhost') {
            console.log('🔄 Live reload enabled - monitoring for changes');
            
            var lastReloadCheck = 0;
            var reloadCheckInterval;
            
            function checkForReload() {
                fetch('/reload_signal.txt?t=' + Date.now())
                    .then(response => response.text())
                    .then(timestamp => {
                        var currentCheck = parseInt(timestamp);
                        if (currentCheck > lastReloadCheck && lastReloadCheck > 0) {
                            console.log('✅ File updated! Reloading page...');
                            window.location.reload(true);
                        }
                        lastReloadCheck = currentCheck;
                    })
                    .catch(error => {
                        console.log('Reload check failed:', error.message);
                    });
            }
            
            // Check every 2 seconds
            reloadCheckInterval = setInterval(checkForReload, 2000);
            
            // Initialize check
            setTimeout(checkForReload, 1000);
            
            // Add visual indicator
            var indicator = document.createElement('div');
            indicator.innerHTML = '🔄 Live Reload Active';
            indicator.style.cssText = 'position:fixed;top:10px;right:10px;background:linear-gradient(45deg, #4CAF50, #45a049);color:white;padding:8px 12px;border-radius:8px;font-size:12px;z-index:10000;font-family:Arial,sans-serif;box-shadow:0 2px 10px rgba(0,0,0,0.3);border:1px solid rgba(255,255,255,0.2);';
            document.body.appendChild(indicator);
            
            // Animate indicator
            indicator.style.transform = 'translateY(-100px)';
            indicator.style.transition = 'all 0.3s ease';
            setTimeout(function() {
                indicator.style.transform = 'translateY(0)';
            }, 100);
            
            // Fade after 3 seconds
            setTimeout(function() {
                if (indicator && indicator.parentNode) {
                    indicator.style.opacity = '0.6';
                    indicator.innerHTML = '🔄 Monitoring...';
                }
            }, 3000);
        }
          
    </script>
</body>
<!-- Generated: )" << timestamp << R"( -->
</html>)";

    htmlFile.close();
    
    std::cout << GREEN << "HTML map generated: " << filename << RESET << std::endl;
    
    // Start or update live reload server
    if (!g_liveServer) {
        g_liveServer = std::make_unique<LiveReloadServer>();
        g_liveServer->startServer(filename);
        
        // Give server time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
    
    // Open the localhost URL in browser if requested
    if (openBrowser)
    {
        std::string mapFileName = std::filesystem::path(filename).filename().string();
        std::string localhost_url = "http://localhost:" + std::to_string(g_liveServer->getPort()) + "/" + mapFileName;
        
        std::string openCommand;
#ifdef _WIN32
        openCommand = "start \"\" \"" + localhost_url + "\"";
#endif
#ifdef __APPLE__
        openCommand = "open \"" + localhost_url + "\"";
#endif
#ifdef __linux__
        openCommand = "xdg-open \"" + localhost_url + "\"";
#endif
        if (!openCommand.empty())
        {
            std::system(openCommand.c_str());
            std::cout << CYAN << "🌐 Map opened at " << localhost_url << RESET << std::endl;
            std::cout << GREEN << "✨ Live reload is active - changes will appear automatically!" << RESET << std::endl;
        }
        else
        {
            std::cout << YELLOW << "Please open " << localhost_url << " in your browser." << RESET << std::endl;
        }
    }
    else
    {
        std::cout << CYAN << "🔄 Map updated - live reload will refresh your browser automatically" << RESET << std::endl;
    }
}

void printBanner() {
const std::string banner = R"(                                                                         
 ________________________________________________________________________
/_____/_____/_____/_____/_____/_____/_____/_____/_____/_____/_____/_____/
    ____  ___    __  _______     ___   _____________   ________          
   / __ \/   |  /  |/  / __ \   /   | / ____/ ____/ | / /_  __/          
  / /_/ / /| | / /|_/ / /_/ /  / /| |/ / __/ __/ /  |/ / / /             
 / _, _/ ___ |/ /  / / ____/  / ___ / /_/ / /___/ /|  / / /              
/_/ |_/_/  |_/_/  /_/_/      /_/  |_\____/_____/_/ |_/ /_/               
                                                                         
   ______            _____          ______                __             
  / ____/___  ____  / __(_)___ _   / ____/_______  ____ _/ /_____  _____ 
 / /   / __ \/ __ \/ /_/ / __ `/  / /   / ___/ _ \/ __ `/ __/ __ \/ ___/ 
/ /___/ /_/ / / / / __/ / /_/ /  / /___/ /  /  __/ /_/ / /_/ /_/ / /     
\____/\____/_/ /_/_/ /_/\__, /   \____/_/   \___/\__,_/\__/\____/_/      
                       /____/                                            
 ________________________________________________________________________
/_____/_____/_____/_____/_____/_____/_____/_____/_____/_____/_____/_____/
                                                                         
                                                                         )";
    std::cout << CYAN << banner << RESET << std::endl;
}

int init(nlohmann::ordered_json &configJson, bool& mapGenerated, std::string &icao) {
    mapGenerated = false;

    // Print pretty banner
    printBanner();

    std::cout << "Select config file (ICAO, if not found, new one is created): ";
    std::getline(std::cin, icao);
    std::transform(icao.begin(), icao.end(), icao.begin(), ::toupper);


    if (!getConfig(icao, configJson))
    {
        return 1;
    }
    
    // Ready to edit the JSON
    std::cout << "JSON edition ready." << std::endl;
    printMenu();
    return 0;
}

int main()
{
    bool mapGenerated = false;
    nlohmann::ordered_json configJson;
    std::string icao;
    
    // Cleanup handler for live server
    auto cleanup = []() {
        if (g_liveServer) {
            std::cout << YELLOW << "\n🛑 Shutting down live reload server..." << RESET << std::endl;
            g_liveServer->stop();
            g_liveServer.reset();
        }
    };
    
    if (init(configJson, mapGenerated, icao) == 1) {
        return 1;
    }

    // Track if map has been generated to auto-update on save
    std::string command;
    while (true)
    {
        std::cout << "> ";
        std::getline(std::cin, command);
        std::transform(command.begin(), command.end(), command.begin(), ::tolower);

        if (command == "exit")
        {
            cleanup();
            break;
        }
        else if (command == "save")
        {
            saveFile(icao, configJson);
            // Auto-regenerate map if it was previously generated
            if (mapGenerated)
            {
                std::cout << CYAN << "Updating map visualization..." << RESET << std::endl;
                generateMap(configJson, icao, false); // Don't open browser on updates
            }
        }
        else if (command == "list")
        {
            listAllStands(configJson);
        }
        else if (command == "map")
        {
            generateMap(configJson, icao, true); // Open browser on first generation
            mapGenerated = true;
        }
        else if (command.rfind("add ", 0) == 0)
        {
            std::string standName = command.substr(4);
            addStand(configJson, standName);
            // Auto-update map if it was previously generated
            if (mapGenerated)
            {
                generateMap(configJson, icao, false);
            }
        }
        else if (command.rfind("remove ", 0) == 0)
        {
            std::string standName = command.substr(7);
            removeStand(configJson, standName);
            // Auto-update map if it was previously generated
            if (mapGenerated)
            {
                generateMap(configJson, icao, false);
            }
        }
        else if (command.rfind("copy ", 0) == 0)
        {
            std::string standName = command.substr(5);
            copyStand(configJson, standName);
            // Auto-update map if it was previously generated
            if (mapGenerated)
            {
                generateMap(configJson, icao, false);
            }
        }
        else if (command.rfind("batchcopy ", 0) == 0)
        {
            std::string standName = command.substr(10);
            batchcopy(configJson, standName);
            // Auto-update map if it was previously generated
            if (mapGenerated)
            {
                generateMap(configJson, icao, false);
            }
        }
        else if (command.rfind("softcopy ", 0) == 0)
        {
            std::string standName = command.substr(9);
            softStandCopy(configJson, standName);
            // Auto-update map if it was previously generated
            if (mapGenerated)
            {
                generateMap(configJson, icao, false);
            }
        }
        else if (command.rfind("edit ", 0) == 0)
        {
            std::string standName = command.substr(5);
            editStand(configJson, standName);
            // Auto-update map if it was previously generated
            if (mapGenerated)
            {
                generateMap(configJson, icao, false);
            }
        }
        else if (command == "config")
        {
            if (init(configJson, mapGenerated, icao) == 1) {
                return 1;
            }
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