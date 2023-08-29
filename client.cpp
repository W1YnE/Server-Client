#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "account.pb.h"
#include <iostream>
#include <thread>  // 引入线程库

using namespace std;
using namespace account::protobuf;

void connect_to_server(sockaddr_in &addr_s, int &sock, const char *ip, int port) {
    // 创建套接字
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Create Socket Error");
        exit(1);
    }

    // 设置要连接的服务器地址
    addr_s.sin_family = AF_INET;
    addr_s.sin_addr.s_addr = inet_addr(ip);
    addr_s.sin_port = htons(port);

    // 连接服务器
    if (connect(sock, (struct sockaddr*)&addr_s, sizeof(addr_s)) == -1) {
        perror("Connect Server Error");
        exit(1);
    }
}

// 新增函数：接收服务器消息
void receive_messages(int sock) {
    char server_message[256];
    while (true) {
        memset(server_message, 0, sizeof(server_message));
        ssize_t bytes_read = read(sock, server_message, sizeof(server_message) - 1);
        if (bytes_read <= 0) {
            printf("\n与服务器断开连接...\n");
            exit(0);
        }
        printf("\n%s\n发送消息 (输入 'logout' 退出): ", server_message);
        fflush(stdout); // 确保消息立即显示
    }
}

int main() {
    int sock;
    sockaddr_in addr_s;

    connect_to_server(addr_s, sock, "127.0.0.1", 8888);

    account_struct req;
    char buff[1000];

    // 用户输入
    int choice;
    printf("1. 注册\n2. 登录\n请选择: ");
    scanf("%d", &choice);
    getchar(); // 吸收换行符

    string account, password;

    printf("请输入账号: ");
    getline(cin, account);
    printf("请输入密码: ");
    getline(cin, password);

    req.set_account(account);
    req.set_password(password);
    if (choice == 1) {
        req.set_is_register(true);
    } else {
        req.set_is_register(false);
    }

    // 序列化并发送给服务器
    int serializedSize = req.ByteSizeLong();
    req.SerializeToArray(buff, serializedSize);
    write(sock, buff, serializedSize); // 发送实际序列化长度

    // 获取服务器回应
    char response[100];
    memset(response, 0, sizeof(response));
    read(sock, response, sizeof(response));

    if (response[0] == 'Y') {
        printf("操作成功\n");

        // 开启新线程来读取服务器消息
        thread receive_thread(receive_messages, sock);
        receive_thread.detach();

        // 在此处，实现消息发送功能
        while (1) {
            string message;
            printf("发送消息 (输入 'logout' 退出): ");
            getline(cin, message);

            if (message == "logout") {
                write(sock, message.c_str(), message.size() + 1); // 也发送null终止符
                break;
            } else {
                write(sock, message.c_str(), message.size() + 1); // 也发送null终止符
            }
        }

    } else {
        printf("操作失败\n");
    }

    close(sock);
    return 0;
}
