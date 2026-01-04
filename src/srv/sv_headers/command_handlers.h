#ifndef CPP_PERSONAL_CLOUD_COMMAND_HANDLERS_H
#define CPP_PERSONAL_CLOUD_COMMAND_HANDLERS_H

#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <utility>

#include "picosha2.h"
#include "cloud_dir.h"
#include "cloud_file.h"
#include "db_manager.h"
#include "encryption_manager.h"
#include "redundancy_manager.h"
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
            return ServerResponse{1, "Login Successful", ""};
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

        std::filesystem::path primary_path = session.getUserDirectory() / "primary" / file_path;
        int user_id = DBManager::get_user_id(session.getUsername());
        std::string db_hash = RedundancyManager::getStoredHash(user_id,
                                                               primary_path.string());

        std::cout << "Looking for hash of: " << primary_path.filename() << "\n";
        std::cout << "Found hash: " << db_hash << "\n";

        if (!db_hash.empty()) {
            if (!RedundancyManager::verifyFileIntegrity(primary_path.string(), db_hash)) {
                std::cout << "Hash mismatch! Repairing...\n";
                std::filesystem::path backup_path = session.getUserDirectory() / "backup" / file_path;
                RedundancyManager::repairFromBackup(primary_path.string(), backup_path.string());
            }
        } else {
            std::cout << "Warning: No hash found in DB for " << file_path << "\n";
        }

        file_path = primary_path;
        try {
            if (!std::filesystem::exists(file_path)) {
                std::string err = "File doesn't exist " + file_path;
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
            send(sock, &json_size, sizeof(int), 0);
            send(sock, metadata_json.c_str(), json_size, 0);

            int size = 0;
            if (recv(sock, &size, sizeof(int), 0) <= 0) {
                return {0, "Client disconnected", ""};
            }

            if (size <= 0 || size > 100) {
                return {0, "Protocol error: invalid READY size", ""};
            }

            char buffer[100];
            memset(buffer, 0, 100);
            int bytes_rec = recv(sock, buffer, size, 0);

            std::string response(buffer, bytes_rec);
            if (response != "READY") {
                return {0, "Sync error. Expected READY, got: " + response, ""};
            }

            std::ifstream file(file_path, std::ios::binary);
            if (!file.is_open()) {
                std::string err = "Can't open file for reading";
                return {0, err, ""};
            }

            char file_buffer[BUFFER_SIZE];
            size_t total_sent = 0;

            while (file.read(file_buffer, sizeof(file_buffer)) || file.gcount() > 0) {
                size_t bytes_to_send = file.gcount();

                std::string hash = session.getPasswordHash();
                EncryptionManager::decrypt((uint8_t *) file_buffer, bytes_to_send, hash);

                ssize_t sent = send(sock, file_buffer, bytes_to_send, 0);
                if (sent < 0) {
                    std::string err = "Error sending file data to client";
                    file.close();
                    return {0, err, ""};
                }

                total_sent += sent;
            }

            file.close();
            return ServerResponse{
                1, "Successfully downloaded " + std::filesystem::path(file_path).filename().string(), ""
            };
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
    std::string target_dir;

public:
    PostCommand(std::string ObjJson, std::string target_dir, int client_sock, UserSession &session)
        : ObjJson(std::move(ObjJson)), target_dir(std::move(target_dir)), client_sock(client_sock), session(session) {
    }

    ServerResponse execute() override {
        if (!session.isAuthenticated()) {
            return ServerResponse{0, "Not authenticated", ""};
        }

        try {
            json j = json::parse(this->ObjJson);
            CloudFile received_file = j.get<CloudFile>();

            std::string clean_name = received_file.name;
            clean_name.erase(std::remove(clean_name.begin(), clean_name.end(), '/'), clean_name.end());

            std::filesystem::path primary_dir = session.getUserDirectory() / "primary";
            std::filesystem::path backup_dir = session.getUserDirectory() / "backup";

            std::string relative_target = this->target_dir;
            if (!relative_target.empty() && relative_target[0] == '/') {
                relative_target.erase(0, 1);
            }

            std::filesystem::path primary_file = primary_dir / relative_target / clean_name;
            std::filesystem::path backup_file = backup_dir / relative_target / clean_name;

            if (std::filesystem::exists(primary_file)) {
                return ServerResponse{0, "File already exists", ""};
            }

            std::ofstream primary_stream(primary_file, std::ios::binary);
            std::ofstream backup_stream(backup_file, std::ios::binary);

            if (!primary_stream.is_open() || !backup_stream.is_open()) {
                return ServerResponse{0, "Failed to create file", ""};
            }

            std::string ack = "READY";
            size_t ack_size = ack.length();
            send(client_sock, &ack_size, sizeof(int), 0);
            send(client_sock, ack.c_str(), ack_size, 0);

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

                std::string hash = session.getPasswordHash();
                EncryptionManager::encrypt((uint8_t *) buffer, bytes_received, hash);

                primary_stream.write(buffer, bytes_received);
                backup_stream.write(buffer, bytes_received);

                total_received += bytes_received;
            }

            primary_stream.close();
            backup_stream.close();

            std::string hash = RedundancyManager::calculateHash(primary_file.string());
            int user_id = DBManager::get_user_id(session.getUsername());
            RedundancyManager::saveFileHash(user_id, primary_file.string(), hash);

            return ServerResponse{1, "Successfully uploaded file " + received_file.name, ""};
        } catch (const json::parse_error &e) {
            return ServerResponse{0, "JSON parse error", e.what()};
        } catch (const std::exception &e) {
            return ServerResponse{0, "Error uploading file", e.what()};
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

        if (rel_path == ".") {
            cloud_dir.path = "/";
        } else {
            std::string path_str = rel_path.string();
            std::replace(path_str.begin(), path_str.end(), '\\', '/');

            if (path_str[0] != '/') {
                cloud_dir.path = "/" + path_str;
            } else {
                cloud_dir.path = path_str;
            }
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
    DeleteCommand(const std::string &path, UserSession &session) : path(path), session(session) {
    }

    ServerResponse execute() override {
        try {
            std::filesystem::path primary_p = session.getUserDirectory() / "primary" / path;
            std::filesystem::path backup_p = session.getUserDirectory() / "backup" / path;

            bool deleted_primary = std::filesystem::remove(primary_p);
            std::filesystem::remove(backup_p);
            DBManager::removeFile(session.getUsername(), primary_p.filename());

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

class CreateDirCommand : public Command {
private:
    std::string name;
    std::string target_dir;
    UserSession &session;

public:
    CreateDirCommand(const std::string &name, const std::string &target_dir, UserSession &session) : name(name),
        target_dir(target_dir), session(session) {
    }

    ServerResponse execute() override {
        std::filesystem::path primary_path =
                session.getUserDirectory().string() + "/" + "primary" + "/" + target_dir + "/" + name;
        std::filesystem::path backup_path = session.getUserDirectory().string() + "/" + "backup" + "/" + target_dir
                                            + "/" + name;

        try {
            if (std::filesystem::exists(primary_path)) {
                return {0, "Directory already exists", ""};
            }
            std::filesystem::create_directory(primary_path);
            std::filesystem::create_directory(backup_path);
            return {1, "Successfully Created Directory", ""};
        } catch (std::exception e) {
            return {0, e.what(), ""};
        }
    }
};

class RegisterCommand : public Command {
private:
    std::string user;
    std::string pass;

public:
    RegisterCommand(const std::string &name, const std::string &pass)
        : user(name), pass(pass) {
    }

    ServerResponse execute() override {
        if (DBManager::try_register(user, pass)) {
            return {1, "Registered Successfully", ""};
        } else {
            return {0, "Username Taken", ""};
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
            return std::make_unique<PostCommand>(arguments[0], arguments[1], client_sock, session);
        } else if (command.find("LIST") == 0) {
            return std::make_unique<ListCommand>(session);
        } else if (command.find("DELETE") == 0) {
            return std::make_unique<DeleteCommand>(arguments[0], session);
        } else if (command.find("CREATEDIR") == 0) {
            return std::make_unique<CreateDirCommand>(arguments[0], arguments[1], session);
        } else if (command.find("REGISTER") == 0) {
            return std::make_unique<RegisterCommand>(arguments[0], arguments[1]);
        } else {
            throw std::runtime_error("Comanda Invalida");
        }
    }
};

#endif
