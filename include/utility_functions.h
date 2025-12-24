#ifndef CPP_PERSONAL_CLOUD_UTILITY_FUNCTIONS_H
#define CPP_PERSONAL_CLOUD_UTILITY_FUNCTIONS_H

#include <sstream>
#include <string>
#include <vector>

inline std::string trimString(std::string notTrimmed) {
    std::string trimmed = notTrimmed;

    trimmed.erase(0, trimmed.find_first_not_of(" \t\n"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n") + 1);

    return trimmed;
}

inline std::vector<std::string> getArguments(std::string command) {
    std::vector<std::string> args;
    std::istringstream iss(command);
    std::string token;

    iss >> token;

    while (iss >> token) {
        args.push_back(token);
    }

    return args;
}

#endif //CPP_PERSONAL_CLOUD_UTILITY_FUNCTIONS_H
