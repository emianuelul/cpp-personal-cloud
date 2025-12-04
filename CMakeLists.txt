cmake_minimum_required(VERSION 4.0)
project(cpp_personal_cloud)

set(CMAKE_CXX_STANDARD 14)

add_executable(cpp_personal_cloud
        src/cli/client.cpp
        src/srv/server.cpp)
