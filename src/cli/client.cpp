#include <iostream>
#include <thread>
#include <string>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "cloud_dir.h"
#include "main_window.h"
#include "portable-file-dialogs.h"
#include "cli_headers/file_explorer_manager.h"
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

void refresh_explorer(slint::ComponentHandle<MainWindow> ui_handle, std::shared_ptr<FileExplorerManager> manager) {
    slint::invoke_from_event_loop([ui_handle, manager]() {
        auto *ui = ui_handle.operator->();
        if (!ui) return;

        ui->set_current_path(slint::SharedString(manager->get_curr_path()));

        auto subdirs = manager->get_subdirs();
        auto subdirs_model = std::make_shared<slint::VectorModel<DirEntry> >();

        for (const auto &entry: subdirs) {
            subdirs_model->push_back(DirEntry{
                slint::SharedString(entry.name),
                slint::SharedString(entry.path),
                entry.file_count,
            });
        }
        ui->set_subdirs(subdirs_model);

        auto files = manager->get_files();
        auto files_model = std::make_shared<slint::VectorModel<File> >();
        for (const auto &f: files) {
            std::string formatted = format_bits(f.size);
            std::string path = manager->get_curr_path() + "/" + f.name;

            files_model->push_back(File{
                slint::SharedString(f.name),
                slint::SharedString(path),
                slint::SharedString(formatted),
            });
        }

        ui->set_files(files_model);
    });
}

void refresh_file_list(slint::ComponentHandle<MainWindow> ui_handle, std::shared_ptr<FileExplorerManager> manager) {
    std::thread network_thread([ui_handle, manager]() {
        ServerResponse response = ServerConnection::getInstance().list();

        if (response.status_code != 1)
            return;

        try {
            json j = json::parse(response.response_data_json);
            CloudDir root = j.get<CloudDir>();

            manager->update_root(root);
            refresh_explorer(ui_handle, manager);
        } catch (...) {
            return;
        }
    });

    network_thread.detach();
}

void show_status(ServerResponse resp) {
    std::cout << "\n===== COMMAND STATUS =====\n";
    std::cout << "Code: " << resp.status_code << '\n';
    std::cout << "Msg: " << resp.status_message << '\n';
    std::cout << "Received JSON: " << resp.response_data_json << "\n\n";
}

int main(int argc, char *argv[]) {
    auto &server = ServerConnection::getInstance();

    if (server.connect().status_code == 0) {
        std::cerr << "Eroare la conectarea la server!\n";
        return 1;
    }

    auto ui = MainWindow::create();
    slint::ComponentHandle<MainWindow> ui_handle(ui);

    auto manager = std::make_shared<FileExplorerManager>(
        CloudDir{"root", "/", {}, {}}
    );

    ui->on_get([ui_handle](slint::SharedString path) {
        std::thread network_thread([path, ui_handle]() {
            std::string process_path = path.data();
            process_path.erase(0, 1);

            ServerResponse response =
                    ServerConnection::getInstance().get(process_path);

            show_status(response);

            slint::invoke_from_event_loop([ui_handle, response]() {
                if (response.status_code == 1) {
                    std::cout << "Successfully downloaded file!\n";
                } else {
                    std::cout << "Failed to get file from server\n";
                }
            });
        });

        network_thread.detach();
    });

    ui->on_post([ui_handle, manager]() {
        auto selection = pfd::open_file("Select file to upload").result();

        if (selection.empty())
            return;

        std::filesystem::path path = selection[0];

        std::thread network_thread([path, ui_handle, manager]() {
            ServerResponse response =
                    ServerConnection::getInstance().post(path.string());

            slint::invoke_from_event_loop([ui_handle, response, manager]() {
                if (response.status_code == 1) {
                    std::cout << "Fisier incarcat cu succes!\n";
                    refresh_file_list(ui_handle, manager);
                } else {
                    std::cout << "Post esuat de la server.\n";
                }
            });
        });

        network_thread.detach();
    });

    ui->on_navigate_back([ui_handle, manager]() {
        if (manager->navigate_back()) {
            refresh_explorer(ui_handle, manager);
        }
    });

    ui->on_navigate_to_dir([ui_handle, manager](slint::SharedString path) {
        if (manager->navigate_to(path.data())) {
            refresh_explorer(ui_handle, manager);
        } else {
            std::cerr << "Navigation failed for path: " << path.data() << std::endl;
        }
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

    ui->on_try_login([ui_handle, manager](slint::SharedString name, slint::SharedString passwd) {
        std::thread network_thread([ui_handle, name, passwd, manager]() {
            ServerResponse response = ServerConnection::getInstance().login(name.data(), passwd.data());

            slint::invoke_from_event_loop([ui_handle, response, manager]() {
                auto *ui = ui_handle.operator->();
                if (!ui) return;

                if (response.status_code) {
                    ui->set_is_logged(true);
                    refresh_file_list(ui_handle, manager);
                } else {
                    ui->set_is_logged(false);
                    std::cout << "Login esuat pe server.\n";
                }
            });
        });

        network_thread.detach();
    });

    ui->on_delete([ui_handle, manager](slint::SharedString path) {
        std::thread network_thread([path, ui_handle, manager]() {
            std::string process_path = path.data();
            process_path.erase(0, 1);

            ServerResponse response =
                    ServerConnection::getInstance().delete_file(process_path);

            slint::invoke_from_event_loop([ui_handle, response, manager]() {
                if (response.status_code == 1) {
                    std::cout << "Fisier sters cu succes!\n";
                    refresh_file_list(ui_handle, manager);
                } else {
                    std::cout << "DELETE esuat de la server.\n";
                }
            });
        });

        network_thread.detach();
    });

    ui->run();

    return 0;
}
