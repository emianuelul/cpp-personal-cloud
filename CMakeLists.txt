cmake_minimum_required(VERSION 3.21)
project(cpp_personal_cloud)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Setează path-urile pentru Slint
set(Slint_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/Slint/lib/cmake/Slint")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include/Slint/lib/cmake/Slint" ${CMAKE_MODULE_PATH})

find_package(Slint REQUIRED)
find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# --- SERVER ---
add_executable(server_exec
        src/srv/server.cpp
        src/srv/sv_headers/client_worker.h
        src/srv/sv_headers/command_handlers.h
        include/utility_functions.h
)
target_link_libraries(server_exec PRIVATE Threads::Threads)

# --- CLIENT ---
add_executable(client_exec
        src/cli/client.cpp
        src/cli/cli_headers/session_manager.h
        include/cloud_file.h
)

slint_target_sources(client_exec src/cli/ui/main_window.slint)

target_include_directories(client_exec PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${GTK3_INCLUDE_DIRS}
)

target_link_libraries(client_exec PRIVATE
        Slint::Slint
        Threads::Threads
        ${GTK3_LIBRARIES}
)