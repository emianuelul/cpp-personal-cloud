cmake_minimum_required(VERSION 3.21)
project(cpp_personal_cloud)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "/opt/homebrew;${CMAKE_PREFIX_PATH}")

set(SLINT_STYLE "native" CACHE STRING "Use native style" FORCE)

include(FetchContent)
FetchContent_Declare(
        Slint
        GIT_REPOSITORY https://github.com/slint-ui/slint.git
        GIT_TAG v1.14.1
        SOURCE_SUBDIR api/cpp
)
FetchContent_MakeAvailable(Slint)

find_package(Threads REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(aes_lib
        include/aes.c
)

# --- SERVER ---
add_executable(server_exec
        src/srv/server.cpp
        src/srv/sv_headers/client_worker.h
        src/srv/sv_headers/command_handlers.h
        include/utility_functions.h
        include/cloud_file.h
        include/cloud_dir.h
        include/server_response.h
)

target_include_directories(server_exec PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(server_exec PRIVATE
        Threads::Threads
        SQLite::SQLite3
        nlohmann_json::nlohmann_json
        aes_lib
)

# --- CLIENT ---
add_executable(client_exec
        src/cli/client.cpp
        include/cloud_file.h
        src/cli/cli_headers/server_connection.h
        src/srv/sv_headers/user_session.h
        include/cloud_dir.h
        include/server_response.h
        src/srv/sv_headers/redundancy_manager.h
        src/srv/sv_headers/db_manager.h
        src/cli/cli_headers/file_explorer_manager.h
)

slint_target_sources(client_exec src/cli/ui/slint_files/main_window.slint)

target_include_directories(client_exec PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

if(APPLE)
    find_library(APPKIT_LIBRARY AppKit)
    find_library(FOUNDATION_LIBRARY Foundation)
    find_library(METAL_LIBRARY Metal)
    find_library(QUARTZCORE_LIBRARY QuartzCore)
endif()

target_link_libraries(client_exec PRIVATE
        Slint::Slint
        Threads::Threads
        nlohmann_json::nlohmann_json
)

if(APPLE)
    target_link_libraries(client_exec PRIVATE
            ${APPKIT_LIBRARY}
            ${FOUNDATION_LIBRARY}
            ${METAL_LIBRARY}
            ${QUARTZCORE_LIBRARY}
    )
endif()