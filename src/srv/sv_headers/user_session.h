// session.h
#ifndef CPP_PERSONAL_CLOUD_SESSION_H
#define CPP_PERSONAL_CLOUD_SESSION_H

#include <string>
#include <filesystem>
#include <iostream>

class UserSession {
private:
    std::string username;
    int userid;
    std::string password_hash;
    bool authenticated;
    std::filesystem::path user_directory;

public:
    UserSession() : authenticated(false) {
    }

    bool isAuthenticated() const {
        return authenticated;
    }

    std::string getUsername() const {
        return username;
    }

    std::filesystem::path getUserDirectory() const {
        return user_directory;
    }

    std::string getPasswordHash() const {
        return password_hash;
    }

    int getUserId() const {
        return userid;
    }

    bool login(const std::string &user, const std::string &password) {
        username = user;
        if (DBManager::try_login(username, password)) {
            authenticated = true;

            userid = DBManager::get_user_id(username);
            password_hash = DBManager::get_user_hash(userid);

            user_directory = std::filesystem::path("./storage") / username;
            std::filesystem::path primary = user_directory / "primary";
            std::filesystem::path backup = user_directory / "backup";

            try {
                std::filesystem::create_directories(primary);
                std::filesystem::create_directories(backup);
                std::cout << "Created directories for user: " << username << '\n';
                return true;
            } catch (const std::exception &e) {
                std::cerr << "Failed to create user directories: " << e.what() << '\n';
                authenticated = false;
                return false;
            }
        } else {
            return false;
        }
    }

    void logout() {
        username.clear();
        authenticated = false;
        user_directory.clear();
    }
};

#endif
