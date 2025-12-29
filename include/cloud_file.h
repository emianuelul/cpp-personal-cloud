#ifndef CPP_PERSONAL_CLOUD_CLOUD_FILE_H
#define CPP_PERSONAL_CLOUD_CLOUD_FILE_H

#include <string>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

struct CloudFile {
    unsigned long long size = 0;
    std::string name;
};

inline void to_json(json &j, const CloudFile &p) {
    j = json{
        {"size", p.size},
        {"name", p.name},
    };
}

inline void from_json(const json &j, CloudFile &p) {
    j.at("size").get_to(p.size);
    j.at("name").get_to(p.name);
}

#endif //CPP_PERSONAL_CLOUD_CLOUD_FILE_H
