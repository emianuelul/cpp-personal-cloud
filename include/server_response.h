#ifndef CPP_PERSONAL_CLOUD_SERVER_RESPONSE_H
#define CPP_PERSONAL_CLOUD_SERVER_RESPONSE_H
#include <string>

struct ServerResponse {
    int status_code;
    std::string status_message;
    std::string response_data_json;
};

#endif //CPP_PERSONAL_CLOUD_SERVER_RESPONSE_H
