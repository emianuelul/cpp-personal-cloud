#ifndef CPP_PERSONAL_CLOUD_COMMAND_HANDLER_H
#define CPP_PERSONAL_CLOUD_COMMAND_HANDLER_H
#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "cloud_dir.h"
#include "cloud_file.h"

#define BUFFER_SIZE 8192
#define PORT 8005
// #define IP "10.100.0.30"
#define IP "192.168.1.3"

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

    bool sendToServer(std::string msg) {
        int len = msg.length();
        ssize_t sent = send(sock, &len, sizeof(int), 0);
        if (sent < 0) {
            std::cout << "Eroare la send\n";
        }
        sent = send(sock, msg.c_str(), msg.length(), 0);
        std::cout << "Trimis la server: " << msg << '\n';
        return (sent == msg.length());
    }

    bool receiveStatus() {
        int size;
        if (recv(sock, &size, sizeof(int), 0) != sizeof(int)) {
            std::cerr << "Eroare la primirea dimensiunii status\n";
            return false;
        }

        if (size <= 0 || size >= BUFFER_SIZE) {
            std::cerr << "Dimensiune status invalidă: " << size << "\n";
            return false;
        }

        char buffer[BUFFER_SIZE];
        int received = recv(sock, buffer, size, 0);
        if (received != size) {
            std::cerr << "Eroare la primirea status-ului\n";
            return false;
        }

        buffer[size] = '\0';
        std::string status(buffer);

        std::cout << "Status primit: " << status << "\n";

        return status == "SUCC";
    }

public:
    static ServerConnection &getInstance() {
        static ServerConnection instance;
        return instance;
    }

    ~ServerConnection() {
        disconnect();
    }

    bool connect() {
        if (isConnected) {
            std::cout << "Deja conectat\n";
            return true;
        }

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            std::cout << "Eroare la crearea socket-ului\n";
            return false;
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        serverAddr.sin_addr.s_addr = inet_addr(IP);

        if (::connect(sock, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) < 0) {
            std::cout << "Eroare la connect\n";
            close(sock);
            return false;
        }
        std::cout << "Client-ul s-a conectat!\n";
        isConnected = true;

        return true;
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

    bool login(std::string user, std::string passwd) {
        if (sock < 0 || !isConnected) {
            std::cout << "You're not connected...\n";
            return false;
        }

        this->user = user;
        this->pass = passwd;

        std::string cmd = "LOGIN " + user + " " + passwd;
        sendToServer(cmd);

        return receiveStatus();
    }

    bool logout() {
        std::string cmd = "LOGOUT";
        sendToServer(cmd);
        return receiveStatus();
    }

    bool get(std::string file) {
        std::string cmd = "GET " + file;
        sendToServer(cmd);
        return receiveStatus();
    }

    bool post(std::string file_path) {
        if (sock < 0 || !isConnected) {
            std::cout << "Nu esti conectat la server\n";
            return false;
        }

        try {
            if (!std::filesystem::exists(file_path)) {
                std::cerr << "Fisierul nu exista: " << file_path << '\n';
                return false;
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
                std::cerr << "Eroare la primirea confirmarii!\n";
                return false;
            }

            memset(buffer, 0, BUFFER_SIZE);
            received = recv(sock, buffer, size, 0);
            if (received != size) {
                std::cerr << "Eroare la primirea confirmarii complete\n";
                return false;
            }

            std::string response(buffer);
            if (response != "READY") {
                std::cerr << "Serverul nu este gata sa primeasca fisierul: " << response << '\n';
                return false;
            }

            std::ifstream file(file_path, std::ios::binary);
            if (!file.is_open()) {
                std::cerr << "Nu pot deschide fisierul pentru citire\n";
                return false;
            }

            char file_buffer[BUFFER_SIZE];
            size_t total_sent = 0;
            size_t file_size = fileToSend.size;

            while (file.read(file_buffer, sizeof(file_buffer)) || file.gcount() > 0) {
                size_t bytes_to_send = file.gcount();
                ssize_t sent = send(sock, file_buffer, bytes_to_send, 0);

                if (sent < 0) {
                    std::cerr << "Eroare la trimiterea datelor fisierului\n";
                    file.close();
                    return false;
                }

                total_sent += sent;
            }

            file.close();

            return receiveStatus();
        } catch (const std::exception &e) {
            std::cerr << "Eroare: " << e.what() << '\n';
            return false;
        }
    }

    bool list() {
        std::string cmd = "LIST";
        sendToServer(cmd);

        bool status = receiveStatus();

        if (!status) {
            std::cerr << "Comanda LIST a eșuat\n";
            return false;
        }

        int json_size;
        if (recv(sock, &json_size, sizeof(int), 0) != sizeof(int)) {
            std::cerr << "Eroare la primirea dimensiunii JSON\n";
            return false;
        }

        if (json_size <= 0 || json_size >= BUFFER_SIZE) {
            std::cerr << "Dimensiune JSON invalidă: " << json_size << "\n";
            return false;
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
                std::cerr << "Eroare la primirea JSON-ului\n";
                return false;
            }
            total_received += received;
        }

        buffer[total_received] = '\0';
        std::string json_str(buffer);

        std::cout << "JSON primit: " << json_str.substr(0, 100) << "...\n";

        try {
            json j = json::parse(json_str);
            CloudDir root = j.get<CloudDir>();

            std::cout << "\n=== Structura Cloud Storage ===\n";
            std::cout << json_str << '\n';

            return true;
        } catch (const json::parse_error &e) {
            std::cerr << "Eroare la parsarea JSON: " << e.what() << "\n";
            std::cerr << "JSON primit: " << json_str << "\n";
            return false;
        } catch (const std::exception &e) {
            std::cerr << "Eroare: " << e.what() << "\n";
            return false;
        }
    }
};

#endif //CPP_PERSONAL_CLOUD_COMMAND_HANDLER_H
