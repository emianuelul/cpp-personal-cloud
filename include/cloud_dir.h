#ifndef CPP_PERSONAL_CLOUD_CLOUD_DIR_H
#define CPP_PERSONAL_CLOUD_CLOUD_DIR_H
#include <string>

#include "cloud_file.h"

struct CloudDir {
    std::string name;
    std::string path;
    std::vector<CloudFile> files;
    std::vector<CloudDir> subdirs;
};

inline void to_json(json &j, const CloudDir &p) {
    j = json{
        {"name", p.name},
        {"path", p.path},
        {"files", p.files},
        {"subdirs", p.subdirs},
    };
}

inline void from_json(const json &j, CloudDir &p) {
    j.at("name").get_to(p.name);
    j.at("path").get_to(p.path);
    j.at("files").get_to(p.files);
    j.at("subdirs").get_to(p.subdirs);
}


#endif
