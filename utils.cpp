#include "utils.h"
#include <algorithm>
#include <sstream>
#include <filesystem>
#include <regex>
#include <iostream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

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

std::vector<std::string> splitRemark(const std::string &str)
{
    std::vector<std::string> result;
    std::istringstream stream(str);
    std::string token;
    while (std::getline(stream, token, ','))
    {
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
#elif defined(__APPLE__)
    char buffer[1024];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) != 0) {
        return "./"; // Fallback to current directory
    }
    std::string executablePath = buffer;
    size_t pos = executablePath.find_last_of('/');
    return (pos == std::string::npos) ? "./" : executablePath.substr(0, pos + 1);
#else
    // Assume Linux / other unix: readlink on /proc/self/exe
    char buffer[1024];
    ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (count == -1) {
        return "./";
    }
    buffer[count] = '\0';
    std::string executablePath = buffer;
    size_t pos = executablePath.find_last_of('/');
    return (pos == std::string::npos) ? "./" : executablePath.substr(0, pos + 1);
#endif
}

std::string getBaseDir() {
    std::string execDir = getExecutableDir();
    return execDir + "configs/";
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

bool useIsValid(const std::string &use)
{
    std::regex validUseRegex(R"(^[A,C,H,M,P]+$)");
    return std::regex_match(use, validUseRegex);
}

bool codeIsValid(const std::string &code)
{
    std::regex validCodeRegex(R"(^[A,B,C,D,E,F]+$)");
    return std::regex_match(code, validCodeRegex);
}
