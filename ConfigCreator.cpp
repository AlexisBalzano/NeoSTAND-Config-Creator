// Minimal driver: orchestrates the refactored modules only
#include <iostream>
#include <algorithm>
#include "nlohmann/json.hpp"
#include "utils.h"
#include "config_manager.h"
#include "map_generator.h"
#include "stands.h"
#include "live_reload.h"

constexpr auto version = "v1.0.9";

static void printBanner()
{
    std::string banner = R"(                                                                         
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
    std::cout << CYAN << banner << "version: " << version << RESET << std::endl;
}

static int initConfig(nlohmann::ordered_json &configJson, bool &mapGenerated, std::string &icao)
{
    mapGenerated = false;
    printBanner();
    std::cout << "Select config file (ICAO, if not found, new one is created): ";
    std::getline(std::cin, icao);
    std::transform(icao.begin(), icao.end(), icao.begin(), ::toupper);
    if (!getConfig(icao, configJson, mapGenerated))
        return 1;
    std::cout << "JSON edition ready." << std::endl;
    printMenu();
    return 0;
}

int main()
{
    bool mapGenerated = false;
    nlohmann::ordered_json configJson;
    std::string icao;

    auto cleanup = []()
    {
        if (g_liveServer)
        {
            g_liveServer->stop();
            g_liveServer.reset();
        }
    };

    if (initConfig(configJson, mapGenerated, icao) != 0)
        return 1;

    std::string command;
    while (true)
    {
        std::cout << "> ";
        if (!std::getline(std::cin, command))
            break;
        std::string cmdLower = command;
        std::transform(cmdLower.begin(), cmdLower.end(), cmdLower.begin(), ::tolower);

        if (cmdLower == "exit")
        {
            cleanup();
            break;
        }
        if (cmdLower == "save")
        {
            saveFile(icao, configJson);
            if (mapGenerated)
                generateMap(configJson, icao, false);
            continue;
        }
        if (cmdLower == "list")
        {
            listAllStands(configJson);
            continue;
        }
        if (cmdLower == "map")
        {
            generateMap(configJson, icao, true);
            mapGenerated = true;
            continue;
        }

        // commands with args
        if (cmdLower.rfind("add ", 0) == 0)
        {
            addStand(configJson, command.substr(4));
            if (mapGenerated)
                generateMap(configJson, icao, false);
            continue;
        }
        if (cmdLower.rfind("remove ", 0) == 0)
        {
            removeStand(configJson, command.substr(7));
            if (mapGenerated)
                generateMap(configJson, icao, false);
            continue;
        }
        if (cmdLower.rfind("copy ", 0) == 0)
        {
            copyStand(configJson, command.substr(5));
            if (mapGenerated)
                generateMap(configJson, icao, false);
            continue;
        }
        if (cmdLower.rfind("batchcopy ", 0) == 0)
        {
            batchcopy(configJson, command.substr(10));
            if (mapGenerated)
                generateMap(configJson, icao, false);
            continue;
        }
        if (cmdLower.rfind("softcopy ", 0) == 0)
        {
            softStandCopy(configJson, command.substr(9));
            if (mapGenerated)
                generateMap(configJson, icao, false);
            continue;
        }
        if (cmdLower.rfind("edit ", 0) == 0)
        {
            editStand(configJson, command.substr(5));
            if (mapGenerated)
                generateMap(configJson, icao, false);
            continue;
        }
        if (cmdLower.rfind("radius ", 0) == 0)
        {
            editStandRadius(configJson, command.substr(7));
            if (mapGenerated)
                generateMap(configJson, icao, false);
            continue;
        }

        if (cmdLower == "config")
        {
            if (initConfig(configJson, mapGenerated, icao) != 0)
                return 1;
            continue;
        }
        if (cmdLower == "help")
        {
            printMenu();
            continue;
        }

        std::cout << "Unknown command: " << command << std::endl;
    }

    return 0;
}