#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8005
#define BACKLOG 30
#define BUFFER_SIZE 2048

int main(){
    const int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    int new_sock, client_sockets[BACKLOG] = {0};
    fd_set master_set, read_set, write_set;

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

    FD_ZERO(&master_set);
    FD_SET(serverFd, &master_set);
    int maxfd = serverFd;

    std::cout << "Serverul asculta pe portul: " << PORT << "\n";

    while(1) {
        read_set = master_set;
        write_set = master_set;
        int activity = select(maxfd + 1, &read_set, &write_set, NULL, NULL);

        if (activity < 0) {
            std::cout<< "Select error\n";

            close(serverFd);
            return -1;
        }

        if (FD_ISSET(serverFd, &read_set)) {
            if ((new_sock = accept(serverFd, reinterpret_cast<sockaddr*>(&serverAddr), &serverLen)) < 0) {
                std::cout << "Eroare la accept\n";

                close(serverFd);
                return -1;
            }

            std::cout << "S-a conectat un client cu ID-ul: " << new_sock << " si IP-ul: " << inet_ntoa(serverAddr.sin_addr) << '\n';

            for (int & client_socket : client_sockets) {
                if (client_socket == 0) {
                    client_socket = new_sock;
                    break;
                }
            }

            if (new_sock > maxfd) {
                maxfd = new_sock;
            }

            FD_SET(new_sock, &master_set);
        }

        for (int & client_socket : client_sockets) {
            int fd = client_socket;

            if (fd > 0 && FD_ISSET(fd, &read_set)) {
                char buffer[BUFFER_SIZE] = {};
                int val_read = recv(fd, buffer, (ssize_t)(BUFFER_SIZE- 1), 0);

                if (val_read > 0) {
                    buffer[BUFFER_SIZE] = '\0';
                    std::cout << "From client " << fd << ": " << buffer << '\n';
                } else if (val_read == 0) {
                    std::cout << "Client " << fd << " disconnected...\n";
                    close(fd);
                    FD_CLR(fd, &master_set);
                }
            }
        }
    }

    close(serverFd);
    return 0;
}

