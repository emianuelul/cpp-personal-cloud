#ifndef CPP_PERSONAL_CLOUD_CLIENT_WORKER_H
#define CPP_PERSONAL_CLOUD_CLIENT_WORKER_H

#include <thread>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>

#include "utility_functions.h"
#include "command_handlers.h"

#define BUFFER_SIZE 2048

class ClientWorker {
private:
    int fd;

    void processClientCommands() {
        while (true) {
            char buffer[BUFFER_SIZE] = {0};
            int received = recv(fd, buffer, BUFFER_SIZE-1, 0);

            if (received > 0) {
                buffer[BUFFER_SIZE] = '\0';
                std::string cmd = buffer;

                cmd = trimString(cmd);
                std::cout << "Received command: " << cmd << "\n";

                try {
                    std::unique_ptr<Command> command = CommandFactory::createCommand(cmd);
                    ServerResponse resp = command->execute();
                    std::cout << "SUCCESS: " << resp.status_message << '\n';
                } catch (const char * err) {
                    std::cerr << "COMANDA EROARE: " << err << "\n";
                } catch (const std::exception& e) {
                    std::cerr << "EROARE SISTEM: " << e.what() << "\n";
                } catch (...) {
                    std::cerr << "EROARE NECUNOSCUTĂ! Curatare si Oprire.\n";
                }

                std::string msgBack = "ACK";
                send(fd, msgBack.c_str(), msgBack.length(), 0);
            } else if (received == 0) {
                break;
            }
        }
    }
public:
    explicit ClientWorker(int socketFd) : fd(socketFd){};

    void run() {
        std::cout << "Thread [" << std::this_thread::get_id() << "] a preluat clientul FD: " << fd << '\n';

        processClientCommands();

        std::cout << "Sesiune incheiata pentru clientul FD: " << fd << std::endl;

        close(fd);
    }
};

#endif