cmake_minimum_required(VERSION 3.21)
project(cpp_personal_cloud)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Slint" ${CMAKE_PREFIX_PATH})

find_package(Slint REQUIRED)
find_package(Threads REQUIRED)

# --- SERVER ---
add_executable(server_exec
        src/srv/server.cpp
        src/srv/sv_headers/client_worker.h
        src/srv/sv_headers/command_handlers.h
        src/srv/sv_headers/utility_functions.h
)
target_link_libraries(server_exec PRIVATE Threads::Threads)

# --- CLIENT ---
add_executable(client_exec
        src/cli/client.cpp
        src/cli/cli_headers/session_manager.h
)

slint_target_sources(client_exec src/cli/ui/main_window.slint)

target_link_libraries(client_exec PRIVATE Slint::Slint Threads::Threads)