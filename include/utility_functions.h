#ifndef CPP_PERSONAL_CLOUD_UTILITY_FUNCTIONS_H
#define CPP_PERSONAL_CLOUD_UTILITY_FUNCTIONS_H

#include <string>

inline std::string trimString(std::string notTrimmed){
    std::string trimmed = notTrimmed;

    trimmed.erase(0, trimmed.find_first_not_of(" \t\n"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n") + 1);

    return trimmed;
}


#endif //CPP_PERSONAL_CLOUD_UTILITY_FUNCTIONS_H