#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <bits/stdc++.h>
#include <thread>
#include <mutex>
#include "account.pb.h"

using namespace std;
using namespace account::protobuf;

map<int, string> clients; // <socket, username>
mutex clients_mutex;

void start_requist(sockaddr_in &addr_s, int sock, const char *ip, int port) {
    memset(&addr_s, 0, sizeof(addr_s));      
    addr_s.sin_family = AF_INET;             
    addr_s.sin_addr.s_addr = inet_addr(ip);  
    addr_s.sin_port = htons(port);           
    bind(sock, (struct sockaddr *)&addr_s, sizeof(addr_s));
}

void read_requist(account_struct &res, int sock_c) {
    char buff[1000];
    memset(buff, 0, sizeof buff);
    read(sock_c, buff, sizeof(buff) - 1);
    if (strlen(buff) != 0) {
        res.ParseFromArray(buff, sizeof(buff));
    }
}

void handle_client(int client_socket, string username) {
    char client_message[256];
    while (1) {
        memset(client_message, 0, sizeof(client_message));
        ssize_t bytes_read = read(client_socket, client_message, sizeof(client_message) - 1);

        if (bytes_read <= 0 || strcmp(client_message, "logout") == 0) {
            break;
        }

        // Display on the server
        printf("%s: %s\n", username.c_str(), client_message);

        // Forward the message to all connected clients
        string full_message = username + ": " + client_message;
        clients_mutex.lock();
        for (auto &client : clients) {
            if (client.first != client_socket) {
                write(client.first, full_message.c_str(), full_message.size() + 1);
            }
        }
        clients_mutex.unlock();
    }

    clients_mutex.lock();
    clients.erase(client_socket);
    clients_mutex.unlock();

    close(client_socket);
}

int main() {
    int sock_s;
    if ((sock_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        perror("Create Socket Error\n");
        exit(1);
    } else {
        printf("Create SocketID: %d\n", sock_s);
    }

    sockaddr_in addr_s;
    start_requist(addr_s, sock_s, "127.0.0.1", 8888);
    listen(sock_s, 20);

    while (true) {
        printf("----------------------------------------------------begin\n");
        
        struct sockaddr_in addr_c;
        socklen_t addr_c_size = sizeof(addr_c);
        int sock_c = accept(sock_s, (struct sockaddr *)&addr_c, &addr_c_size);
        if (sock_c == -1) {
            perror("Accept Error");
            exit(1);
        }

        account_struct res;
        read_requist(res, sock_c);

        cout << "是否是注册请求：" << res.is_register() << endl;
        cout << "账号：" << res.account() << endl;
        cout << "密码：" << res.password() << endl;
        cout << "IP：" << res.hostip() << endl;
        cout << "操作时间：" << res.time() << endl;

        string path = "Accounts/" + res.account();
        const char *account = path.c_str();
        const char *password = res.password().c_str();

        char rebuff[100];
        memset(rebuff, 0, sizeof(rebuff));

        if (res.is_register()) {
            if (access(account, F_OK) == 0) {
                rebuff[0] = 'N';
                printf("账号已存在\n");
            } else {
                rebuff[0] = 'Y';
                FILE *file = fopen(account, "a+");
                if (file == NULL) {
                    rebuff[0] = 'N';
                    printf("生成文件失败\n");
                } else {
                    fprintf(file, "%s", password);
                    fflush(file);
                    fclose(file);
                    printf("注册成功\n");
                }
            }
        } else {
            if (access(account, F_OK) != 0) {
                rebuff[0] = 'N';
                printf("账号不存在\n");
            } else {
                FILE *file = fopen(account, "r");
                char pwd[100];
                memset(pwd, 0, sizeof pwd);
                fscanf(file, "%s", pwd);
                if (strcmp(pwd, password) == 0) {
                    rebuff[0] = 'Y';
                    printf("登陆成功\n");
                } else {
                    rebuff[0] = 'N';
                    printf("密码不正确\n");
                }
            }
        }
        write(sock_c, rebuff, sizeof(rebuff));

        if (rebuff[0] == 'Y') {
            clients_mutex.lock();
            clients[sock_c] = res.account();
            clients_mutex.unlock();

            thread client_thread(handle_client, sock_c, res.account());
            client_thread.detach();
        } else {
            close(sock_c);
        }
    }

    printf("--------------------------end\n");
    close(sock_s);
    return 0;
}
