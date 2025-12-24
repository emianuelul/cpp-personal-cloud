#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <arpa/inet.h>
#include <thread>
#include <nlohmann/json.hpp>


#include "utility_functions.h"
#include "sv_headers/client_worker.h"


#define PORT 8005
#define BACKLOG 30
#define BUFFER_SIZE 2048

int main() {
    const int serverFd = socket(AF_INET, SOCK_STREAM, 0);

    if (serverFd == 0) {
        std::cout << "Eroare la crearea socket-ului\n";
        return -1;
    }

    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "EROARE la setsockopt\n";
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    socklen_t serverLen = sizeof(serverAddr);

    if (bind(serverFd, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr))) {
        std::cout << "Eroare la bind\n";

        close(serverFd);
        return -1;
    }

    if (listen(serverFd, BACKLOG)) {
        std::cout << "Eroare la listen\n";

        close(serverFd);
        return -1;
    }


    std::cout << "Serverul asculta pe portul: " << PORT << "\n";

    while (true) {
        int new_sock = accept(serverFd, reinterpret_cast<sockaddr *>(&serverAddr), &serverLen);
        if (new_sock < 0) {
            std::cout << "Eroare la connect\n";

            close(serverFd);
            return -1;
        }

        std::cout << "Un client s-a conectat\n";

        ClientWorker worker(new_sock);

        std::thread client_thread(&ClientWorker::run, std::move(worker));

        client_thread.detach();
    }

    close(serverFd);
    return 0;
}

