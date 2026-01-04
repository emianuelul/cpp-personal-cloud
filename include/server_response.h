#ifndef CPP_PERSONAL_CLOUD_SERVER_RESPONSE_H
#define CPP_PERSONAL_CLOUD_SERVER_RESPONSE_H
#include <string>

struct ServerResponse {
    int status_code;
    std::string status_message;
    std::string response_data_json;
};

inline void to_json(json &j, const ServerResponse &p) {
    j = json{
        {"status_code", p.status_code},
        {"status_message", p.status_message},
        {"response_data_json", p.response_data_json},
    };
}

inline void from_json(const json &j, ServerResponse &p) {
    j.at("status_code").get_to(p.status_code);
    j.at("status_message").get_to(p.status_message);
    j.at("response_data_json").get_to(p.response_data_json);
}

#endif //CPP_PERSONAL_CLOUD_SERVER_RESPONSE_H
