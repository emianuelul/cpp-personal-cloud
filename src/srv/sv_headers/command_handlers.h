#ifndef CPP_PERSONAL_CLOUD_COMMAND_HANDLERS_H
#define CPP_PERSONAL_CLOUD_COMMAND_HANDLERS_H

#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

#include "cloud_dir.h"
#include "cloud_file.h"
#include "user_session.h"

using json = nlohmann::json;

struct ServerResponse {
    int status_code;
    std::string status_message;
    std::string response_data_json;
};

class Command {
public:
    virtual ServerResponse execute() = 0;

    virtual ~Command() {
    }
};

class LogInCommand : public Command {
private:
    std::string username;
    std::string passwd;
    UserSession &session;

public:
    LogInCommand(std::string username, std::string passwd, UserSession &session)
        : username(std::move(username)), passwd(std::move(passwd)), session(session) {
    }

    ServerResponse execute() override {
        if (session.login(username, passwd)) {
            return ServerResponse{0, "Login Successful for " + username, ""};
        }
        return ServerResponse{1, "Login Failed", ""};
    }
};

class LogOutCommand : public Command {
private:
    UserSession &session;

public:
    LogOutCommand(UserSession &session) : session(session) {
    }

    ServerResponse execute() override {
        session.logout();
        return ServerResponse{0, "Logout Successful", ""};
    }
};

class GetCommand : public Command {
private:
    UserSession &session;
    std::string filename;

public:
    GetCommand(std::string filename, UserSession &session)
        : filename(std::move(filename)), session(session) {
    }

    ServerResponse execute() override {
        if (!session.isAuthenticated()) {
            return ServerResponse{1, "Not authenticated", ""};
        }

        // TODO: Implementeaza GET
        return ServerResponse{0, "Get Successful", ""};
    }
};

class PostCommand : public Command {
private:
    std::string ObjJson;
    int client_sock;
    UserSession &session;

public:
    PostCommand(std::string ObjJson, int client_sock, UserSession &session)
        : ObjJson(ObjJson), client_sock(client_sock), session(session) {
    }

    ServerResponse execute() override {
        if (!session.isAuthenticated()) {
            return ServerResponse{1, "Not authenticated", ""};
        }

        try {
            json j = json::parse(this->ObjJson);
            CloudFile received_file = j.get<CloudFile>();

            std::filesystem::path primary_dir =
                    session.getUserDirectory() / "primary";
            std::filesystem::path backup_dir =
                    session.getUserDirectory() / "backup";

            std::filesystem::path primary_file = primary_dir / received_file.name;
            std::filesystem::path backup_file = backup_dir / received_file.name;

            if (std::filesystem::exists(primary_file)) {
                return ServerResponse{1, "File already exists", ""};
            }

            std::ofstream primary_stream(primary_file, std::ios::binary);
            std::ofstream backup_stream(backup_file, std::ios::binary);

            if (!primary_stream.is_open() || !backup_stream.is_open()) {
                return ServerResponse{1, "Failed to create files", ""};
            }

            std::string ack = "READY";
            size_t ack_size = ack.length();
            send(client_sock, &ack_size, sizeof(int), 0);
            send(client_sock, ack.c_str(), ack_size, 0);

            std::cout << "Uploading file for user: " << session.getUsername() << '\n';

            char buffer[8192];
            size_t total_received = 0;
            size_t file_size = received_file.size;

            while (total_received < file_size) {
                size_t remaining = file_size - total_received;
                size_t to_receive = (sizeof(buffer) < remaining) ? sizeof(buffer) : remaining;
                ssize_t bytes_received = recv(client_sock, buffer, to_receive, 0);

                if (bytes_received <= 0) {
                    primary_stream.close();
                    backup_stream.close();
                    std::filesystem::remove(primary_file);
                    std::filesystem::remove(backup_file);
                    return ServerResponse{1, "Transfer interrupted", ""};
                }

                primary_stream.write(buffer, bytes_received);
                backup_stream.write(buffer, bytes_received);

                total_received += bytes_received;
            }

            primary_stream.close();
            backup_stream.close();

            std::cout << "File saved with redundancy: " << received_file.name
                    << " (" << total_received << " bytes)\n";

            return ServerResponse{0, "Post Successful", ""};
        } catch (const json::parse_error &e) {
            return ServerResponse{1, "JSON parse error", e.what()};
        } catch (const std::exception &e) {
            return ServerResponse{1, "Error", e.what()};
        }
    }
};

class ListCommand : public Command {
private:
    UserSession &session;

    CloudDir buildDir(const std::filesystem::path &dir_path, const std::filesystem::path &base_path) {
        CloudDir cloud_dir;
        cloud_dir.name = dir_path.filename().string();

        std::filesystem::path rel_path = std::filesystem::relative(dir_path, base_path);
        cloud_dir.path = rel_path.string();
        if (cloud_dir.path == ".") {
            cloud_dir.path = "";
        }

        try {
            for (const auto &entry: std::filesystem::directory_iterator(dir_path)) {
                if (entry.is_regular_file()) {
                    CloudFile file;
                    file.name = entry.path().filename().string();
                    file.size = std::filesystem::file_size(entry.path());
                    cloud_dir.files.push_back(file);
                } else if (entry.is_directory()) {
                    CloudDir subdir = buildDir(entry.path(), base_path);
                    cloud_dir.subdirs.push_back(subdir);
                }
            }
        } catch (const std::filesystem::filesystem_error &e) {
            std::cerr << "Error reading directory: " << e.what() << '\n';
        }

        return cloud_dir;
    }

public:
    ListCommand(UserSession &session) : session(session) {
    }

    ServerResponse execute() override {
        if (!session.isAuthenticated()) {
            return ServerResponse{1, "Not authenticated", ""};
        }

        try {
            std::filesystem::path primary_dir = session.getUserDirectory() / "primary";

            if (!std::filesystem::exists(primary_dir)) {
                return ServerResponse{1, "User directory not found", ""};
            }

            CloudDir root = buildDir(primary_dir, primary_dir);

            json j = root;
            std::string json_str = j.dump();

            return ServerResponse{0, "List Successful", json_str};
        } catch (const std::exception &e) {
            return ServerResponse{1, "Error listing files", e.what()};
        }
    }
};


class CommandFactory {
public:
    static std::unique_ptr<Command> createCommand(
        const std::string &command, int client_sock,
        UserSession &session) {
        std::vector<std::string> arguments = getArguments(command);

        if (command.find("LOGIN") == 0) {
            return std::make_unique<LogInCommand>(arguments[0], arguments[1], session);
        } else if (command.find("LOGOUT") == 0) {
            return std::make_unique<LogOutCommand>(session);
        } else if (command.find("GET") == 0) {
            return std::make_unique<GetCommand>(arguments[0], session);
        } else if (command.find("POST") == 0) {
            return std::make_unique<PostCommand>(arguments[0], client_sock, session);
        } else if (command.find("LIST" == 0)) {
            return std::make_unique<ListCommand>(session);
        } else {
            throw std::runtime_error("Comanda Invalida");
        }
    }
};

#endif
