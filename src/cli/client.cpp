#include <iostream>
#include <thread>
#include <string>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <chrono>

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
            std::string path = manager->get_curr_path() == "/"
                                   ? manager->get_curr_path() + f.name
                                   : manager->get_curr_path() + "/" + f.name;

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

void show_toast(bool succ, const std::string &msg, slint::ComponentHandle<MainWindow> ui_handle) {
    if (succ) {
        ui_handle->set_toast_msg(slint::SharedString(msg));
        ui_handle->set_toast_type(slint::SharedString("SUCC"));
        ui_handle->set_showing_toast(true);
    } else {
        ui_handle->set_toast_msg(slint::SharedString(msg));
        ui_handle->set_toast_type(slint::SharedString("FAIL"));
        ui_handle->set_showing_toast(true);
    }

    slint::Timer::single_shot(std::chrono::seconds(2), [ui_handle] {
        ui_handle->set_showing_toast(false);
    });
}

int main(int argc, char *argv[]) {
    auto &server = ServerConnection::getInstance();

    if (server.connect().status_code == 0) {
        std::cerr << "Error connecting to server\n";
        return 1;
    }

    auto ui = MainWindow::create();
    slint::ComponentHandle<MainWindow> ui_handle(ui);

    auto explorer_manager = std::make_shared<FileExplorerManager>(
        CloudDir{"root", "/", {}, {}}
    );

    ui->on_get([ui_handle](slint::SharedString path) {
        std::thread network_thread([path, ui_handle]() {
            std::string process_path = path.data();
            process_path.erase(0, 1);

            ServerResponse response =
                    ServerConnection::getInstance().get(process_path);

            slint::invoke_from_event_loop([ui_handle, response] {
                if (response.status_code == 1) {
                    show_toast(true, response.status_message, ui_handle);
                } else {
                    show_toast(false, response.status_message, ui_handle);
                }
            });
        });

        network_thread.detach();
    });

    ui->on_post([ui_handle, explorer_manager]() {
        auto selection = pfd::open_file("Select file to upload").result();

        if (selection.empty())
            return;

        std::filesystem::path path = selection[0];
        std::string curr_dir = explorer_manager->get_curr_path();


        std::thread network_thread([path, curr_dir, ui_handle, explorer_manager]() {
            ServerResponse response =
                    ServerConnection::getInstance().post(path.string(), curr_dir);


            slint::invoke_from_event_loop([ui_handle, response, explorer_manager]() {
                if (response.status_code == 1) {
                    show_toast(true, response.status_message, ui_handle);
                    refresh_file_list(ui_handle, explorer_manager);
                } else {
                    show_toast(false, response.status_message, ui_handle);
                }
            });
        });

        network_thread.detach();
    });

    ui->on_navigate_back([ui_handle, explorer_manager]() {
        if (explorer_manager->navigate_back()) {
            refresh_explorer(ui_handle, explorer_manager);
        }
    });

    ui->on_navigate_to_dir([ui_handle, explorer_manager](slint::SharedString path) {
        if (explorer_manager->navigate_to(path.data())) {
            refresh_explorer(ui_handle, explorer_manager);
        }
    });

    ui->on_logout([ui_handle]() {
        std::thread network_thread([ui_handle]() {
            ServerResponse response = ServerConnection::getInstance().logout();

            slint::invoke_from_event_loop([ui_handle, response]() {
                if (auto *ui = ui_handle.operator->()) {
                    ui->set_is_logged(false);
                }
            });
        });

        network_thread.detach();
    });

    ui->on_try_login([ui_handle, explorer_manager](slint::SharedString name, slint::SharedString passwd) {
        std::thread network_thread([ui_handle, name, passwd, explorer_manager]() {
            ServerResponse response = ServerConnection::getInstance().login(name.data(), passwd.data());

            slint::invoke_from_event_loop([ui_handle, response, explorer_manager]() {
                auto *ui = ui_handle.operator->();
                if (!ui) return;

                if (response.status_code) {
                    show_toast(true, response.status_message, ui_handle);
                    ui->set_is_logged(true);
                    refresh_file_list(ui_handle, explorer_manager);
                } else {
                    ui->set_is_logged(false);
                    show_toast(false, response.status_message, ui_handle);
                }
            });
        });

        network_thread.detach();
    });

    ui->on_register([ui_handle, explorer_manager](slint::SharedString name, slint::SharedString passwd) {
        std::thread network_thread([ui_handle, name, passwd, explorer_manager]() {
            ServerResponse response = ServerConnection::getInstance().register_cmd(name.data(), passwd.data());


            slint::invoke_from_event_loop([ui_handle, response, explorer_manager]() {
                auto *ui = ui_handle.operator->();
                if (!ui) return;

                if (response.status_code == 1) {
                    show_toast(true, response.status_message, ui_handle);
                } else {
                    show_toast(false, response.status_message, ui_handle);
                }
            });
        });

        network_thread.detach();
    });

    ui->on_delete([ui_handle, explorer_manager](slint::SharedString path) {
        std::thread network_thread([path, ui_handle, explorer_manager]() {
            std::string process_path = path.data();
            process_path.erase(0, 1);

            ServerResponse response =
                    ServerConnection::getInstance().delete_file(process_path);

            slint::invoke_from_event_loop([ui_handle, response, explorer_manager]() {
                if (response.status_code == 1) {
                    refresh_file_list(ui_handle, explorer_manager);
                }
            });
        });

        network_thread.detach();
    });

    ui->on_create_dir([ui_handle, explorer_manager](slint::SharedString name) {
        auto response = ServerConnection::getInstance().create_dir(name.data(), explorer_manager->get_curr_path());

        slint::invoke_from_event_loop([ui_handle, explorer_manager, response] {
            if (response.status_code == 1) {
                show_toast(true, response.status_message, ui_handle);
                refresh_file_list(ui_handle, explorer_manager);
            } else {
                show_toast(false, response.status_message, ui_handle);
            }
        });
    });

    ui->run();

    return 0;
}
