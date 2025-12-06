#ifndef CPP_PERSONAL_CLOUD_SESSION_MANAGER_H
#define CPP_PERSONAL_CLOUD_SESSION_MANAGER_H

class SessionManager {
private:
    std::string currUser;
    bool isLogged = false;

    SessionManager() {}

    bool validate(std::string user) {
        // TODO look through sqlite db

        return true;
    }

public:
    static SessionManager& getInstance() {
        static SessionManager instance;
        return instance;
    }

    bool login(std::string user) {
        if (validate(user)) {
            isLogged = true;
            currUser = user;
            return true;
        }
        return false;
    }

    bool logout() {
        if (isLogged) {
            currUser.clear();
            isLogged = false;
            return true;
        }

        return false;
    }

    bool isLoggedIn() { return isLogged; }

    std::string getCurrentUser() { return currUser; }
};
#endif