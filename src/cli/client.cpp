#include <iostream>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>

#include <QApplication>
#include <QMainWindow>
#include <ui_mainwindow.h>
#include <QStyleFactory>
#include <QPalette>


#define PORT 8005
#define IP "10.100.0.30"
#define BUFFER_SIZE 2048

int sock;
bool isConnected = false;

void sendToServer(std::string msg) {
    int len = msg.length();
    ssize_t sent = send(sock, &len, sizeof(int), 0);
    if (sent < 0) {
        std::cout << "Eroare la send\n";
    }
    sent = send(sock, msg.c_str(), msg.length(), 0);

    std::cout << "Trimis la server: " << msg << '\n';
}

bool commandStatus() {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int size;
    ssize_t received = recv(sock, &size, sizeof(int), 0);

    if (received < 0) {
        std::cout << "Eroare la primirea dimensiunii!\n";
        return false;
    }

    received = recv(sock, buffer, size, 0);

    if (received != size) {
        std::cout << "Eroare: primit " << received << " bytes si asteptam " << size << "\n";
        return false;
    }

    buffer[size] = '\0';

    std::cout << "De la server: " << buffer << '\n';

    std::string resp = buffer;
    return (resp == "SUCC");
}

void initConnection() {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cout << "Eroare la crearea socket-ului\n";
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(IP);

    if (connect(sock, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cout << "Eroare la connect\n";

        close(sock);
        return;
    }
    std::cout << "Client-ul s-a conectat!\n";

    isConnected = true;
}

bool sendLoginCommand(std::string user, std::string passwd) {
    if (sock < 0 || !isConnected) {
        std::cout << "You're not connected...\n";
        return false;
    }

    std::string cmd = "LOGIN " + user + " " + passwd;

    sendToServer(cmd);


    return commandStatus();
}

bool sendGetRequest(std::string file) {
    std::string cmd = "GET " + file;

    sendToServer(cmd);

    return commandStatus();
}

bool sendPostRequest(std::string file) {
    std::string cmd = "POST " + file;

    sendToServer(cmd);

    return commandStatus();
}

bool sendLogOutCommand() {
    std::string cmd = "LOGOUT";

    sendToServer(cmd);

    return commandStatus();
}

int main(int argc, char *argv[]) {
    initConnection();

    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Fusion"));
    QPalette lightPalette;


    a.setPalette(lightPalette);

    QMainWindow clientDialog;
    Ui::MainWindow ui;

    ui.setupUi(&clientDialog);

    ui.stackedWidget->setCurrentIndex(0);

    QObject::connect(ui.loginButton, &QPushButton::clicked, [&ui]() {
        std::string username = ui.userLineEdit->text().toStdString();
        std::string password = ui.passLineEdit->text().toStdString();

        std::thread([username, password, ui]() {
            bool resp = sendLoginCommand(username, password);

            if (resp) {
                QMetaObject::invokeMethod(ui.stackedWidget, [&ui]() {
                    ui.stackedWidget->setCurrentIndex(1);
                }, Qt::QueuedConnection);
            }
        }).detach();
    });

    QObject::connect(ui.getButton, &QPushButton::clicked, [&ui]() {
        std::thread([ui]() {
            bool resp = sendGetRequest(std::string("/home/repos/js"));

            if (resp) {
                std::cout << "Returned file!\n";
            }
        }).detach();
    });
    QObject::connect(ui.postButton, &QPushButton::clicked, [&ui]() {
        std::thread([ui]() {
            bool resp = sendPostRequest(std::string("/home/repos/js"));

            if (resp) {
                std::cout << "Posted file!\n";
            }
        }).detach();
    });

    QObject::connect(ui.logOutButton, &QPushButton::clicked, [&ui]() {
        sendLogOutCommand();
        QMetaObject::invokeMethod(ui.stackedWidget, [&ui]() {
            ui.stackedWidget->setCurrentIndex(0);
        }, Qt::QueuedConnection);
    });


    clientDialog.show();

    int result = QApplication::exec();

    if (sock >= 0) {
        close(sock);
    }

    return result;
}
