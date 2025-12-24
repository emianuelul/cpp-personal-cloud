#ifndef CPP_PERSONAL_CLOUD_COMMAND_HANDLERS_H
#define CPP_PERSONAL_CLOUD_COMMAND_HANDLERS_H

#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <stdexcept>

#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

#include "cloud_file.h"

using json = nlohmann::json;
namespace fs = std::filesystem;


struct ServerResponse {
    int status_code;
    std::string status_message;
    std::string response_data_json;
};

class Command {
public:
    virtual ServerResponse execute() = 0;

    virtual ~Command() {
    };
};

class LogInCommand : public Command {
private:
    std::string username;
    std::string passwd;

public:
    LogInCommand(std::string username, std::string passwd) : username(std::move(username)), passwd(std::move(passwd)) {
    }

    ServerResponse execute() override {
        std::string status = "Login Successful. User: " + username + " Pass: " + passwd;
        return ServerResponse{0, status, ""};
    }
};

class LogOutCommand : public Command {
public:
    LogOutCommand() {
    }

    ServerResponse execute() override {
        return ServerResponse{0, "Logout Successful", ""};
    }
};

class GetCommand : public Command {
public:
    GetCommand() {
    }

    ServerResponse execute() override {
        return ServerResponse{0, "Get Successful", ""};
    }
};

class PostCommand : public Command {
private:
    std::string ObjJson;

public:
    PostCommand(std::string ObjJson) : ObjJson(ObjJson) {
    }

    ServerResponse execute() override {
        try {
            json j = json::parse(this->ObjJson);
            CloudFile received_file = j.get<CloudFile>();

            std::filesystem::create_directories("./client_files");

            std::filesystem::path file_path = "./client_files/" + received_file.name;

            if (std::filesystem::exists(file_path)) {
                return ServerResponse{1, "File already exists", ""};
            }

            // Creează și scrie în fișier
            std::ofstream file(file_path, std::ios::binary);
            if (!file.is_open()) {
                return ServerResponse{1, "Failed to create file", ""};
            }
            file.close();

            return ServerResponse{0, "Post Successful", ""};
        } catch (const json::parse_error &e) {
            return ServerResponse{1, "JSON parse error", e.what()};
        } catch (const std::exception &e) {
            return ServerResponse{1, "Error", e.what()};
        }
    }
};

class CommandFactory {
public:
    static std::unique_ptr<Command> createCommand(const std::string &command) {
        std::vector<std::string> arguments = getArguments(command);

        if (command.find("LOGIN") == 0) {
            return std::make_unique<LogInCommand>(arguments[0], arguments[1]);
        } else if (command.find("LOGOUT") == 0) {
            return std::make_unique<LogOutCommand>();
        } else if (command.find("GET") == 0) {
            return std::make_unique<GetCommand>();
        } else if (command.find("POST") == 0) {
            return std::make_unique<PostCommand>(arguments[0]);
        } else {
            throw std::runtime_error("Comanda Invalida");
        }
    }
};

#endif
