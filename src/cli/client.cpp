#include <iostream>
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


std::string format_bits(unsigned long long bits) {
    static const char *units[] = {"b", "KB", "MB", "GB"};

    double size = static_cast<double>(bits);
    int unit_index = 0;

    while (size >= 1024.0 && unit_index < 5) {
        size /= 1024.0;
        ++unit_index;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return oss.str();
}

void refresh_file_list(slint::ComponentHandle<MainWindow> ui_handle) {
    std::thread network_thread([ui_handle]() {
        ServerResponse response =
                ServerConnection::getInstance().list();

        if (response.status_code != 1)
            return;

        std::vector<CloudFile> cloud_files;

        try {
            auto j = json::parse(response.response_data_json);
            cloud_files = j.at("files").get<std::vector<CloudFile> >();
        } catch (...) {
            return;
        }

        slint::invoke_from_event_loop([ui_handle, cloud_files]() {
            auto *ui = ui_handle.operator->();
            if (!ui) return;

            auto files_model =
                    std::make_shared<slint::VectorModel<File> >();

            for (const auto &f: cloud_files) {
                std::string formatted = format_bits(f.size);
                files_model->push_back(File{
                    slint::SharedString(f.name),
                    slint::SharedString(formatted),
                });
            }

            ui->set_files(files_model);
        });
    });

    network_thread.detach();
}

int main(int argc, char *argv[]) {
    auto &server = ServerConnection::getInstance();

    if (server.connect().status_code == 0) {
        std::cerr << "Eroare la conectarea la server!\n";
        return 1;
    }

    auto ui = MainWindow::create();

    slint::ComponentHandle<MainWindow> ui_handle(ui);

    ui->on_get([](slint::SharedString file_path) {
        std::thread network_thread([file_path]() {
            ServerResponse response = ServerConnection::getInstance().get(file_path.data());

            if (response.status_code == 0) {
                std::cout << "Get esuat de la server.\n";
            }
        });

        network_thread.detach();
    });

    ui->on_post([ui_handle]() {
        auto selection = pfd::open_file("Select file to upload").result();

        if (selection.empty())
            return;

        std::filesystem::path path = selection[0];

        std::thread network_thread([path, ui_handle]() {
            ServerResponse response =
                    ServerConnection::getInstance().post(path.string());

            slint::invoke_from_event_loop([ui_handle, response]() {
                if (response.status_code == 1) {
                    std::cout << "Fisier incarcat cu succes!\n";
                    refresh_file_list(ui_handle);
                } else {
                    std::cout << "Post esuat de la server.\n";
                }
            });
        });

        network_thread.detach();
    });

    ui->on_logout([ui_handle]() {
        std::thread network_thread([ui_handle]() {
            ServerResponse response = ServerConnection::getInstance().logout();

            slint::invoke_from_event_loop([ui_handle, response]() {
                if (auto *ui = ui_handle.operator->()) {
                    ui->set_is_logged(false);

                    if (response.status_code == 0) {
                        std::cout << "Logout esuat de pe server.\n";
                    }
                }
            });
        });

        network_thread.detach();
    });

    ui->on_try_login([ui_handle](slint::SharedString name, slint::SharedString passwd) {
        std::thread network_thread([ui_handle, name, passwd]() {
            ServerResponse response = ServerConnection::getInstance().login(name.data(), passwd.data());

            slint::invoke_from_event_loop([ui_handle, response]() {
                auto *ui = ui_handle.operator->();
                if (!ui) return;

                if (response.status_code) {
                    ui->set_is_logged(true);
                    refresh_file_list(ui_handle);
                } else {
                    ui->set_is_logged(false);
                    std::cout << "Login esuat pe server.\n";
                }
            });
        });

        network_thread.detach();
    });

    ui->on_update_files([ui_handle]() {
        refresh_file_list(ui_handle);
    });

    ui->run();

    return 0;
}
