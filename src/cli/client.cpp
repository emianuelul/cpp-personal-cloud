#include <iostream>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <string>
#include <cstring>
#include <filesystem>
#include <nlohmann/json.hpp>

#include "main_window.h"
#include "portable-file-dialogs.h"
#include "cloud_file.h"

#define PORT 8005
// #define IP "10.100.0.30"
#define IP "192.168.1.11"
#define BUFFER_SIZE 2048

using json = nlohmann::json;

int sock;
bool isConnected = false;

void sendToServer(std::string msg) {
    int len = msg.length();
    ssize_t sent = send(sock, &len, sizeof(int), 0);
    if (sent < 0) {
        std::cout << "Eroare la send\n";
    }
    sent = send(sock, msg.c_str(), msg.length(), 0);
    std::cout << "Trimis la server: " << msg << '\n';
}

bool commandStatus() {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int size;
    ssize_t received = recv(sock, &size, sizeof(int), 0);

    if (received < 0) {
        std::cout << "Eroare la primirea dimensiunii!\n";
        return false;
    }

    received = recv(sock, buffer, size, 0);

    if (received != size) {
        std::cout << "Eroare: primit " << received << " bytes si asteptam " << size << "\n";
        return false;
    }

    buffer[size] = '\0';
    std::cout << "De la server: " << buffer << '\n';

    std::string resp = buffer;
    return (resp == "SUCC");
}

void initConnection() {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cout << "Eroare la crearea socket-ului\n";
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(IP);

    if (connect(sock, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cout << "Eroare la connect\n";
        close(sock);
        return;
    }
    std::cout << "Client-ul s-a conectat!\n";
    isConnected = true;
}

bool sendLoginCommand(std::string user, std::string passwd) {
    if (sock < 0 || !isConnected) {
        std::cout << "You're not connected...\n";
        return false;
    }
    std::string cmd = "LOGIN " + user + " " + passwd;
    sendToServer(cmd);
    return commandStatus();
}

bool sendGetRequest(std::string file) {
    std::string cmd = "GET " + file;
    sendToServer(cmd);
    return commandStatus();
}

bool sendPostRequest(std::string file) {
    std::string cmd = "POST " + file;
    sendToServer(cmd);
    return commandStatus();
}

bool sendLogOutCommand() {
    std::string cmd = "LOGOUT";
    sendToServer(cmd);
    return commandStatus();
}

int main(int argc, char *argv[]) {
    initConnection();

    if (!isConnected) {
        std::cerr << "Failed to connect to server!\n";
        return 1;
    }

    auto ui = MainWindow::create();

    slint::ComponentHandle<MainWindow> ui_handle(ui);

    ui->on_get([](slint::SharedString file_path) {
        std::thread network_thread([file_path]() {
            bool response = sendGetRequest(file_path.data());

            if (!response) {
                std::cout << "Get esuat de la server.\n";
            }
        });

        network_thread.detach();
    });

    ui->on_post([](slint::SharedString file_data) {
        std::thread network_thread([file_data]() {
            bool response = sendPostRequest(file_data.data());

            if (!response) {
                std::cout << "Get esuat de la server.\n";
            }
        });

        network_thread.detach();
    });

    ui->on_logout([ui_handle]() {
        std::thread network_thread([ui_handle]() {
            bool response = sendLogOutCommand();

            slint::invoke_from_event_loop([ui_handle, response]() {
                if (auto *ui = ui_handle.operator->()) {
                    ui->set_is_logged(!response);

                    if (!response) {
                        std::cout << "Logout esuat de pe server.\n";
                    }
                }
            });
        });

        network_thread.detach();
    });

    ui->on_try_login([ui_handle](slint::SharedString name, slint::SharedString passwd) {
        std::thread network_thread([ui_handle, name, passwd]() {
            bool response = sendLoginCommand(name.data(), passwd.data());

            slint::invoke_from_event_loop([ui_handle, response]() {
                if (auto *ui = ui_handle.operator->()) {
                    ui->set_is_logged(response);

                    if (!response) {
                        std::cout << "Login esuat pe server.\n";
                    }
                }
            });
        });

        network_thread.detach();
    });

    ui->on_open_file_select([]() {
        auto selection = pfd::open_file("Select file to upload").result();

        if (!selection.empty()) {
            std::filesystem::path path = selection[0];

            CloudFile fileToSend = {
                std::filesystem::file_size(path),
                path.filename().string(),
            };

            json j = fileToSend;

            std::string serialized_cloud_file = j.dump();

            sendToServer(serialized_cloud_file);
        }
    });

    ui->run();

    if (sock >= 0) {
        close(sock);
    }

    return 0;
}
