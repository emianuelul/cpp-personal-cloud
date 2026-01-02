#ifndef CPP_PERSONAL_CLOUD_COMMAND_HANDLERS_H
#define CPP_PERSONAL_CLOUD_COMMAND_HANDLERS_H

#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

#include "cloud_dir.h"
#include "cloud_file.h"
#include "server_response.h"
#include "user_session.h"

#define BUFFER_SIZE 8192

using json = nlohmann::json;

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
            return ServerResponse{1, "Login Successful for " + username, ""};
        }
        return ServerResponse{0, "Login Failed", ""};
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
        return ServerResponse{1, "Logout Successful", ""};
    }
};

class GetCommand : public Command {
private:
    UserSession &session;
    std::string file_path;
    int sock;

public:
    GetCommand(std::string file_path, UserSession &session, int sock)
        : file_path(std::move(file_path)), session(session), sock(sock) {
    }

    ServerResponse execute() override {
        if (!session.isAuthenticated()) {
            return ServerResponse{0, "Not authenticated", ""};
        }

        // TODO: CORRUPTION CHECK HERE
        this->file_path = session.getUserDirectory() / "primary" / file_path;
        try {
            if (!std::filesystem::exists(file_path)) {
                std::string err = "File doesn't exist" + file_path;
                err += '\n';
                return {0, err, ""};
            }
            std::filesystem::path path(file_path);
            CloudFile fileToSend = {
                std::filesystem::file_size(path),
                path.filename().string(),
            };

            json j = fileToSend;
            std::string metadata_json = j.dump();

            int json_size = metadata_json.length();
            if (send(sock, &json_size, sizeof(int), 0) < 0) {
                throw std::runtime_error("Error sending payload size to client");
            }
            if (send(sock, metadata_json.c_str(), json_size, 0) < 0) {
                throw std::runtime_error("Error sending payload to client");
            }

            char buffer[BUFFER_SIZE];
            memset(buffer, 0, BUFFER_SIZE);

            int size = 0;
            ssize_t received = recv(sock, &size, sizeof(int), 0);
            if (received < 0) {
                std::string err = "Error getting payload size!\n";
                return {0, err, ""};
            }

            memset(buffer, 0, BUFFER_SIZE);
            received = recv(sock, buffer, size, 0);
            if (received != size) {
                std::string err = "Error getting payload!\n";
                return {0, err, ""};
            }

            std::string response(buffer);
            if (response != "READY") {
                std::string err = "Client not ready to get file: " + response;
                err += '\n';
                return {0, err, ""};
            }

            std::ifstream file(file_path, std::ios::binary);
            if (!file.is_open()) {
                std::string err = "Can't open file for reading\n";
                return {0, err, ""};
            }

            char file_buffer[BUFFER_SIZE];
            size_t total_sent = 0;

            while (file.read(file_buffer, sizeof(file_buffer)) || file.gcount() > 0) {
                size_t bytes_to_send = file.gcount();
                ssize_t sent = send(sock, file_buffer, bytes_to_send, 0);
                if (sent < 0) {
                    std::string err = "Error sending file data to client\n";
                    file.close();
                    return {0, err, ""};
                }

                total_sent += sent;
            }

            file.close();
            return ServerResponse{1, "Get Successful", ""};
        } catch (const std::exception &e) {
            std::string err = "Error: ";
            err += e.what();
            err += '\n';
            return {0, err, ""};
        }
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
            return ServerResponse{0, "Not authenticated", ""};
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
                return ServerResponse{0, "File already exists", ""};
            }

            std::ofstream primary_stream(primary_file, std::ios::binary);
            std::ofstream backup_stream(backup_file, std::ios::binary);

            if (!primary_stream.is_open() || !backup_stream.is_open()) {
                return ServerResponse{0, "Failed to create files", ""};
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
                    return ServerResponse{0, "Transfer interrupted", ""};
                }

                primary_stream.write(buffer, bytes_received);
                backup_stream.write(buffer, bytes_received);

                total_received += bytes_received;
            }

            primary_stream.close();
            backup_stream.close();

            std::cout << "File saved with redundancy: " << received_file.name
                    << " (" << total_received << " bytes)\n";

            return ServerResponse{1, "Post Successful", ""};
        } catch (const json::parse_error &e) {
            return ServerResponse{0, "JSON parse error", e.what()};
        } catch (const std::exception &e) {
            return ServerResponse{0, "Error", e.what()};
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
            return ServerResponse{0, "Not authenticated", ""};
        }

        try {
            std::filesystem::path primary_dir = session.getUserDirectory() / "primary";

            if (!std::filesystem::exists(primary_dir)) {
                return ServerResponse{0, "User directory not found", ""};
            }

            CloudDir root = buildDir(primary_dir, primary_dir);

            json j = root;
            std::string json_str = j.dump();

            std::cout << json_str << '\n';

            return ServerResponse{1, "List Successful", json_str};
        } catch (const std::exception &e) {
            return ServerResponse{0, "Error listing files", e.what()};
        }
    }
};

class DeleteCommand : public Command {
private:
    std::string path;
    UserSession &session;

public:
    DeleteCommand(std::string path, UserSession &session) : path(path), session(session) {
    }

    ServerResponse execute() override {
        try {
            std::filesystem::path primary_p = session.getUserDirectory() / "primary" / path;
            std::filesystem::path backup_p = session.getUserDirectory() / "backup" / path;

            bool deleted_primary = std::filesystem::remove(primary_p);
            std::filesystem::remove(backup_p);

            if (deleted_primary) {
                return ServerResponse{1, "Deleted successfully", ""};
            } else {
                return ServerResponse{0, "File not found in primary storage", ""};
            }
        } catch (const std::filesystem::filesystem_error &e) {
            return ServerResponse{0, e.what(), ""};
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
            return std::make_unique<GetCommand>(arguments[0], session, client_sock);
        } else if (command.find("POST") == 0) {
            return std::make_unique<PostCommand>(arguments[0], client_sock, session);
        } else if (command.find("LIST") == 0) {
            return std::make_unique<ListCommand>(session);
        } else if (command.find("DELETE") == 0) {
            return std::make_unique<DeleteCommand>(arguments[0], session);
        } else {
            throw std::runtime_error("Comanda Invalida");
        }
    }
};

#endif
