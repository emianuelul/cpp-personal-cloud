#include <iostream>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8005
#define IP "192.168.1.11"
#define BUFFER_SIZE 2048

int main(){
    const int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock  < 0) {
        std::cout << "Eroare la crearea socket-ului\n";
        return -1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(IP);

    if (connect(sock, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cout << "Eroare la connect\n";

        close(sock);
        return -1;
    }

    std::cout << "Client-ul s-a conectat!\n";

    while (true) {
        std::cout << "Input: ";
        std::string cmd;
        std::getline(std::cin, cmd);

        send(sock, cmd.c_str(), cmd.length(), 0);

        char buffer[1024];
        const ssize_t from_sv = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (from_sv == 0) {
            std::cout << "Server shut down. Exitting client...\n";

            close(sock);
            break;
        }
    }

    close(sock);
    return 0;
}