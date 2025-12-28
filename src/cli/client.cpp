#include <iostream>
#include <unistd.h>
#include <thread>
#include <string>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "main_window.h"
#include "portable-file-dialogs.h"
#include "cli_headers/server_connection.h"


#define BUFFER_SIZE 8192

using json = nlohmann::json;


int main(int argc, char *argv[]) {
    auto &server = ServerConnection::getInstance();

    if (!server.connect()) {
        std::cerr << "Eroare la conectarea la server!\n";
        return 1;
    }

    auto ui = MainWindow::create();

    slint::ComponentHandle<MainWindow> ui_handle(ui);

    ui->on_get([](slint::SharedString file_path) {
        std::thread network_thread([file_path]() {
            bool response = ServerConnection::getInstance().get(file_path.data());

            if (!response) {
                std::cout << "Get esuat de la server.\n";
            }
        });

        network_thread.detach();
    });

    ui->on_post([]() {
        std::thread network_thread([]() {
            auto selection = pfd::open_file("Select file to upload").result();

            if (!selection.empty()) {
                std::filesystem::path path = selection[0];

                bool response = ServerConnection::getInstance().post(path.string());

                if (response) {
                    std::cout << "Fisier încarcat cu succes!\n";
                } else {
                    std::cout << "Post esuat de la server.\n";
                }
            }
        });

        network_thread.detach();
    });

    ui->on_logout([ui_handle]() {
        std::thread network_thread([ui_handle]() {
            bool response = ServerConnection::getInstance().logout();

            slint::invoke_from_event_loop([ui_handle, response]() {
                if (auto *ui = ui_handle.operator->()) {
                    ui->set_is_logged(false);

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
            bool response = ServerConnection::getInstance().login(name.data(), passwd.data());

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

    ui->run();

    return 0;
}
