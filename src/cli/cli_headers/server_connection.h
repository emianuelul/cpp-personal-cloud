#ifndef CPP_PERSONAL_CLOUD_COMMAND_HANDLER_H
#define CPP_PERSONAL_CLOUD_COMMAND_HANDLER_H
#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "cloud_file.h"
#include "server_response.h"

#define BUFFER_SIZE 8192
#define PORT 8005
// #define IP "10.100.0.30"
#define IP "192.168.1.10"

class ServerConnection {
private:
    int sock;
    bool isConnected;
    std::string user;
    std::string pass;

    ServerConnection() : sock(-1), isConnected(false) {
    }

    ServerConnection(const ServerConnection &) = delete;

    ServerConnection &operator=(const ServerConnection &) = delete;

    ServerResponse sendToServer(std::string msg) {
        int len = msg.length();
        ssize_t sent = send(sock, &len, sizeof(int), 0);
        if (sent < 0) {
            std::cout << "Eroare la send\n";
        }
        sent = send(sock, msg.c_str(), msg.length(), 0);
        std::cout << "Trimis la server: " << msg << '\n';

        return {sent == msg.length() ? 1 : 0, sent == msg.length() ? "Sent successfully\n" : "Failed to send\n", ""};
    }

    ServerResponse receiveStatus() {
        int size;
        if (recv(sock, &size, sizeof(int), 0) != sizeof(int)) {
            return {0, "Eroare la primirea dimensiunii status\n", ""};
        }

        if (size <= 0 || size >= BUFFER_SIZE) {
            std::stringstream s;
            s << "Invalid Status Size: " << size << "\n";
            std::string err = s.str();

            return {0, err, ""};
        }

        char buffer[BUFFER_SIZE];
        int received = recv(sock, buffer, size, 0);
        if (received != size) {
            return {0, "Eroare la primirea status-ului\n", ""};
        }

        buffer[size] = '\0';
        std::string status(buffer);

        return {(status == "SUCC" ? 1 : 0), status, ""};
    }

public:
    static ServerConnection &getInstance() {
        static ServerConnection instance;
        return instance;
    }

    ~ServerConnection() {
        disconnect();
    }

    std::string getUser() {
        return this->user;
    }

    ServerResponse connect() {
        if (isConnected) {
            std::string err = "Deja conectat\n";
            return {0, err, ""};
        }

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            std::string err = "Eroare la crearea socket-ului\n";
            return {0, err, ""};
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        serverAddr.sin_addr.s_addr = inet_addr(IP);

        if (::connect(sock, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) < 0) {
            std::string err = "Eroare la connect\n";
            close(sock);
            return {0, err, ""};
        }
        std::string status = "Client-ul s-a conectat!\n";
        isConnected = true;

        return {1, status, ""};
    }

    void disconnect() {
        if (sock >= 0) {
            close(sock);
            sock = -1;
            isConnected = false;
            std::cout << "Deconectat de la server\n";
        }
    }

    bool connected() const {
        return isConnected;
    }

    ServerResponse login(std::string user, std::string passwd) {
        if (sock < 0 || !isConnected) {
            std::string err = "You're not connected...\n";
            return {0, err, ""};
        }

        this->user = user;
        this->pass = passwd;

        std::string cmd = "LOGIN " + user + " " + passwd;
        sendToServer(cmd);

        return receiveStatus();
    }

    ServerResponse logout() {
        std::string cmd = "LOGOUT";
        sendToServer(cmd);
        return receiveStatus();
    }

    ServerResponse get(std::string file) {
        std::string cmd = "GET " + file;
        sendToServer(cmd);
        return receiveStatus();
    }

    ServerResponse post(std::string file_path) {
        if (sock < 0 || !isConnected) {
            std::string err = "You're not connected...\n";
            return {0, err, ""};
        }

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

            std::string cmd = "POST " + metadata_json;
            sendToServer(cmd);

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
                std::string err = "Server not ready to get file: " + response;
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
                    std::string err = "Error sending file data to server\n";
                    file.close();
                    return {0, err, ""};
                }

                total_sent += sent;
            }

            file.close();

            return receiveStatus();
        } catch (const std::exception &e) {
            std::string err = "Error: ";
            err += e.what();
            err += '\n';
            return {0, err, ""};
        }
    }

    ServerResponse delete_file(std::string path) {
        std::string cmd = "DELETE " + path;
        sendToServer(cmd);

        auto status = receiveStatus();
        if (!status.status_code) {
            std::string err = "DELETE command failed\n";
            return {0, err, ""};
        }

        return {1, "DELETED successfully", ""};
    }

    ServerResponse list() {
        std::string cmd = "LIST";
        sendToServer(cmd);

        auto status = receiveStatus();

        if (!status.status_code) {
            std::string err = "LIST command failed\n";
            return {0, err, ""};
        }

        int json_size;
        if (recv(sock, &json_size, sizeof(int), 0) != sizeof(int)) {
            std::string err = "Error JSON doesn't match received size\n";
            return {0, err, ""};
        }

        if (json_size <= 0 || json_size >= BUFFER_SIZE) {
            std::stringstream s;
            s << "Invalid JSON size: " << json_size << "\n";
            std::string err = s.str();

            return {0, err, ""};
        }

        std::cout << "Primesc JSON de " << json_size << " bytes\n";

        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);

        int total_received = 0;
        while (total_received < json_size) {
            int bytes_to_read = json_size - total_received;
            if (bytes_to_read > BUFFER_SIZE - total_received - 1) {
                bytes_to_read = BUFFER_SIZE - total_received - 1;
            }
            int received = recv(sock, buffer + total_received, bytes_to_read, 0);
            if (received <= 0) {
                std::string err = "Error getting JSON\n";
                return {0, err, ""};
            }
            total_received += received;
        }

        buffer[total_received] = '\0';
        std::string json_str(buffer);

        return {1, "Updated file tree successfully", json_str};
    }
};

#endif //CPP_PERSONAL_CLOUD_COMMAND_HANDLER_H
