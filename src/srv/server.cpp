#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <arpa/inet.h>
#include <thread>

#include "sv_headers/client_worker.h"
#include "sv_headers/redundancy_manager.h"

#define PORT 8005
#define BACKLOG 30

int main() {
    const int serverFd = socket(AF_INET, SOCK_STREAM, 0);

    if (serverFd == 0) {
        std::cout << "Error creating socket\n";
        return -1;
    }

    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error on setsockopt\n";
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    socklen_t serverLen = sizeof(serverAddr);

    if (bind(serverFd, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr))) {
        std::cerr << "Error binding\n";

        close(serverFd);
        return -1;
    }

    if (listen(serverFd, BACKLOG)) {
        std::cerr << "Error listening\n";

        close(serverFd);
        return -1;
    }


    std::cout << "Server is listening on port: " << PORT << "\n";

    std::filesystem::create_directory("./storage");
    RedundancyManager::initDatabase();
    DBManager::initUsers();

    while (true) {
        int new_sock = accept(serverFd, reinterpret_cast<sockaddr *>(&serverAddr), &serverLen);
        if (new_sock < 0) {
            std::cout << "Error connecting\n";

            close(serverFd);
            return -1;
        }

        std::cout << "Client connected\n";

        ClientWorker worker(new_sock);

        std::thread client_thread(&ClientWorker::run, std::move(worker));

        client_thread.detach();
    }

    close(serverFd);
    return 0;
}

