#ifndef CPP_PERSONAL_CLOUD_COMMAND_HANDLERS_H
#define CPP_PERSONAL_CLOUD_COMMAND_HANDLERS_H
#include <memory>
#include <string>
#include <utility>
#include <stdexcept>

struct ServerResponse {
    int status_code;
    std::string status_message;
    std::string response_data_json;
};

class Command {
public:
    virtual ServerResponse execute() = 0;
    virtual ~Command() {};
};

class LogInCommand : public Command {
private:
    std::string username;
    std::string passwd;
public:
    LogInCommand(std::string username, std::string passwd) : username(std::move(username)), passwd(std::move(passwd)){}
    ServerResponse execute() override {
        return ServerResponse{0, "Login Successful", ""};
    }
};

class LogOutCommand : public Command {
public:
    LogOutCommand(){}
    ServerResponse execute() override {
        return ServerResponse{0, "Logout Successful", ""};    }
};

class GetCommand : public Command {
public:
    GetCommand(){}
    ServerResponse execute() override {
        return ServerResponse{0, "Get Successful", ""};    }
};

class PostCommand : public Command {
private:
    std::string ObjJson;
public:
    PostCommand(std::string ObjJson) : ObjJson(ObjJson) {}
    ServerResponse execute() override {
        return ServerResponse{0, "Post Successful", ""};    }
};

class CommandFactory {
public:
    static std::unique_ptr<Command> createCommand(const std::string& command) {
        if (command.find("login") == 0) {
            std::string username = command.substr(6);
            std::string passwd = command.substr(6 + username.length());

            return std::make_unique<LogInCommand>(username, passwd);
        } else if (command.find("logout") == 0) {
            return std::make_unique<LogOutCommand>();
        } else if (command.find("get") == 0 ) {
            return std::make_unique<GetCommand>();
        } else if (command.find("post") == 0) {

            // TODO GET OBJECT METADATA AND RETURN IT THROUGH JSON TO CONSTRUCTOR;

            std::string json;
            return std::make_unique<PostCommand>(json);
        } else {
            throw std::runtime_error("Comanda Invalida");
        }
    }
};

#endif