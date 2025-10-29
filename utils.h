#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

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


std::vector<std::string> splitString(const std::string &str);
std::vector<std::string> splitRemark(const std::string &str);
std::string getExecutableDir();
std::string getBaseDir();
bool isCoordinatesValid(std::string &coordinates);
bool useIsValid(const std::string &use);
bool codeIsValid(const std::string &code);
std::vector<std::pair<std::string, std::string>> parseStandName(const std::string& standName);
bool naturalSort(const std::string& a, const std::string& b);
