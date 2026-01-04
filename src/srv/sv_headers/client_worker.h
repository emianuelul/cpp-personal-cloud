#ifndef CPP_PERSONAL_CLOUD_CLIENT_WORKER_H
#define CPP_PERSONAL_CLOUD_CLIENT_WORKER_H

#include <thread>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>

#include "utility_functions.h"
#include "command_handlers.h"
#include "user_session.h"


class ClientWorker {
private:
    int fd;
    UserSession session;

    void processClientCommands() {
        ServerResponse response;

        while (true) {
            char buffer[BUFFER_SIZE] = {0};
            int size;
            int received = recv(fd, &size, sizeof(int), 0);

            if (received == sizeof(int)) {
                if (size <= 0 || size >= BUFFER_SIZE) {
                    std::cerr << "Dimensiune invalida primita: " << size << "\n";
                    std::string msgBack = "FAIL";

                    int msgSize = msgBack.length();
                    send(fd, &msgSize, sizeof(int), 0);
                    send(fd, msgBack.c_str(), msgBack.length(), 0);
                } else {
                    received = recv(fd, buffer, size, 0);
                    if (received == size) {
                        buffer[size] = '\0';
                        std::string cmd = buffer;

                        cmd = trimString(cmd);
                        std::cout << "Received command: " << cmd << "\n";

                        try {
                            std::unique_ptr<Command> command = CommandFactory::createCommand(cmd, this->fd, session);
                            response = command->execute();

                            if (response.status_code) {
                                std::cout << "SUCCESS: " << response.status_message << '\n';
                            } else {
                                std::cout << "FAILED: " << response.status_message << '\n';
                            }
                        } catch (const char *err) {
                            std::cerr << "COMANDA EROARE: " << err << "\n";
                        } catch (const std::exception &e) {
                            std::cerr << "EROARE SISTEM: " << e.what() << "\n";
                        } catch (...) {
                            std::cerr << "EROARE NECUNOSCUTĂ! Curatare si Oprire.\n";
                        }

                        nlohmann::json response_j = response;
                        std::string response_str = response_j.dump();

                        int msgSize = response_str.length();
                        send(fd, &msgSize, sizeof(int), 0);
                        send(fd, response_str.c_str(), msgSize, 0);

                        std::cout << "Sent " << response_str << "\n";
                    } else {
                        std::cerr << "Eroare la primire mesaj: asteptat " << size << ", primit " << received << "\n";
                    }
                }
            } else if (received == 0) {
                std::cout << "Client deconectat\n";
                break;
            } else {
                std::cerr << "Eroare la primire dimensiune\n";
                break;
            }
        }
    }

public:
    explicit ClientWorker(int socketFd) : fd(socketFd) {
    };

    void run() {
        std::cout << "Thread [" << std::this_thread::get_id() << "] a preluat clientul FD: " << fd << '\n';

        processClientCommands();

        std::cout << "Sesiune incheiata pentru clientul FD: " << fd << std::endl;

        close(fd);
    }
};

#endif
