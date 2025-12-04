#include <iostream>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8005
#define IP 192.168.0.104
#define BACKLOG 30

int main(){
    const int serverFd = socket(AF_INET, SOCK_STREAM, 0);

    if (serverFd  == 0) {
        std::cout << "Eroare la crearea socket-ului\n";
        return -1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    socklen_t serverLen = sizeof(serverAddr);

    if (bind(serverFd, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr))) {
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

    int clinentFd = accept(serverFd, reinterpret_cast<sockaddr*>(&serverAddr), &serverLen);

    if (clinentFd < 0) {
        std::cout << "Eroare la connect\n";

        close(serverFd);
        return -1;
    }

    std::cout << "Client-ul s-a conectat!\n";

    close(serverFd);
    return 0;
}

