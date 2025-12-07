cmake_minimum_required(VERSION 3.14)
project(cpp_personal_cloud)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

add_executable(server_exec
        src/srv/server.cpp
        src/srv/sv_headers/client_worker.h
        src/srv/sv_headers/command_handlers.h
        src/srv/sv_headers/utility_functions.h
)

set(CLIENT_SOURCES
        src/cli/client.cpp
        src/cli/cli_headers/session_manager.h
)

add_executable(client_exec
        ${CLIENT_SOURCES}
        src/cli/mainwindow.ui
        src/cli/mainwindow.h src/cli/mainwindow.cpp src/cli/mainwindow.ui
)

target_link_libraries(client_exec
        PRIVATE
        Qt6::Core
        Qt6::Widgets
)
