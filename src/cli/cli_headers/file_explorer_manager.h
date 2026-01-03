#ifndef CPP_PERSONAL_CLOUD_FILE_EXPLORER_MANAGER_H
#define CPP_PERSONAL_CLOUD_FILE_EXPLORER_MANAGER_H
#include <string>
#include <utility>

#include "cloud_dir.h"

struct SimpleDirEntry {
    std::string name;
    std::string path;
    int file_count;
};

class FileExplorerManager {
private:
    CloudDir root;
    std::vector<std::string> path_stack;
    CloudDir *curr_dir;

    CloudDir *find_dir_by_path(CloudDir &dir, const std::string &target_path) {
        if (dir.path == target_path) {
            return &dir;
        }

        for (auto &subdir: dir.subdirs) {
            CloudDir *found = find_dir_by_path(subdir, target_path);
            if (found) return found;
        }
        return nullptr;
    }

    int count_files(const CloudDir &dir) {
        int count = dir.files.size();
        count += dir.subdirs.size();

        for (const auto &subdir: dir.subdirs) {
            count += subdir.files.size();
            count += subdir.subdirs.size();
        }

        return count;
    }

public:
    FileExplorerManager(CloudDir root) : root(std::move(root)), curr_dir(&this->root) {
    }

    std::vector<SimpleDirEntry> get_subdirs() {
        std::vector<SimpleDirEntry> subdirs;

        for (auto &subdir: curr_dir->subdirs) {
            SimpleDirEntry entry;
            entry.name = subdir.name;
            entry.file_count = count_files(subdir);
            entry.path = subdir.path;

            subdirs.push_back(entry);
        }

        return subdirs;
    }

    std::vector<CloudFile> get_files() {
        return this->curr_dir->files;
    }

    bool navigate_to(const std::string &path) {
        CloudDir *target = find_dir_by_path(this->root, path);

        if (target) {
            path_stack.push_back(curr_dir->path);
            this->curr_dir = target;
            return true;
        }

        return false;
    }

    bool navigate_back() {
        if (path_stack.empty()) {
            return false;
        }

        std::string prev_path = path_stack.back();
        path_stack.pop_back();

        CloudDir *target = find_dir_by_path(this->root, prev_path);
        if (target) {
            this->curr_dir = target;
            return true;
        }

        return false;
    }

    std::string get_curr_path() const {
        return this->curr_dir->path;
    }

    void update_root(CloudDir new_root) {
        this->root = std::move(new_root);
        this->curr_dir = &this->root;
        this->path_stack.clear();
        std::cout << "Root updated. Path is: " << this->root.path << std::endl;
    }
};

#endif //CPP_PERSONAL_CLOUD_FILE_EXPLORER_MANAGER_H
