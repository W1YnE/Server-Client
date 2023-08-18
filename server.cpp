#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <pthread.h>
#include <sys/select.h>
#include <algorithm>
#include <sys/time.h>
#include <sys/types.h>

using namespace std;

// 客户端处理线程函数
void* clientHandler(void* arg)
{
    int clientfd = *(int*)arg; // 获取客户端套接字描述符
    char buf[100];

    while (true)
    {
        // 从客户端接收数据
        int n = read(clientfd, buf, sizeof(buf) - 1);
        if (n == 0)
        {
            cout << "Client disconnected: " << clientfd << endl;
            break;
        }
        else if (n > 0)
        {
            buf[n] = '\0';
            cout << "Received data from client " << clientfd << ": " << buf << endl;

            // 发送响应给客户端
            fgets(buf, sizeof(buf), stdin);
            send(clientfd, buf, strlen(buf), 0);
        }
    }

    close(clientfd); // 关闭客户端套接字
    delete (int*)arg; // 释放动态分配的内存
    pthread_exit(NULL); // 退出线程
}

int main()
{
    int sockfd, sfd;
    struct sockaddr_in sock_addr;
    int i, op;
    int maxfd, maxi, n;

    fd_set rset, allset;
    int client[1024]; // 存储客户端套接字描述符的数组

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // 创建服务器套接字
    if (sockfd < 0)
    {
        printf("Failed to create socket\n");
        return -1;
    }

    bzero(&sock_addr, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(8000);
    sock_addr.sin_addr.s_addr = INADDR_ANY; // 服务器绑定的 IP 地址为 INADDR_ANY，表示可以接受任意 IP 地址的连接

    op = bind(sockfd, (struct sockaddr*)&sock_addr, sizeof(sock_addr)); // 将服务器套接字绑定到指定的 IP 地址和端口上
    if (op < 0)
    {
        printf("Failed to bind socket\n");
        close(sockfd);
        return -1;
    }

    op = listen(sockfd, 4); // 开始监听连接请求，同时维护的最大连接数为 4
    if (op < 0)
    {
        printf("Listen error\n");
        close(sockfd);
        return -1;
    }

    int clientfd;
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(struct sockaddr);

    maxfd = sockfd;
    maxi = -1;
    for (i = 0; i < 1024; i++)
        client[i] = -1; // 初始化客户端套接字描述符数组

    FD_ZERO(&allset);
    FD_SET(sockfd, &allset); // 将监听套接字添加到文件描述符集合中

    while (true)
    {
        rset = allset;
        op = select(maxfd + 1, &rset, NULL, NULL, NULL); // 使用 select 监听套接字集合中的事件
        if (op < 0)
        {
            cout << "Select error\n";
            return -1;
        }

        if (FD_ISSET(sockfd, &rset)) // 如果监听套接字有事件发生，表示有新的连接请求
        {
            clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &len); // 接受新的连接请求

            if (clientfd < 0)
            {
                printf("Accept error\n");
                return -1;
            }

            char clientip[32] = "";
            inet_ntop(AF_INET, &client_addr.sin_addr, clientip, 16);
            printf("Client connected: IP %s, Port %d\n", clientip, ntohs(client_addr.sin_port));

            for (i = 0; i < 1024; i++)
            {
                if (client[i] < 0) // 找到一个空闲的位置存储客户端套接字描述符
                {
                    client[i] = clientfd;
                    break;
                }
            }

            if (i == 1024) // 如果客户端连接数超过了数组的大小，打印错误信息并退出
            {
                cout << "Too many clients\n";
                return -1;
            }

            FD_SET(clientfd, &allset); // 将客户端套接字添加到文件描述符集合中
            if (clientfd > maxfd)
                maxfd = clientfd; // 更新最大文件描述符值

            maxi = max(maxi, i); // 更新最大索引值

            if (--op == 0)
                continue;
        }

        for (i = 0; i <= maxi; i++)
        {
            sfd = client[i];
            if (sfd < 0)
                continue;
            if (FD_ISSET(sfd, &rset)) // 如果客户端套接字有事件发生，表示有数据可读
            {
                pthread_t tid;
                int* arg = new int;
                *arg = sfd;
                pthread_create(&tid, NULL, clientHandler, arg); // 创建一个新的线程来处理客户端请求
                pthread_detach(tid); // 分离线程，使其自行释放资源

                if (--op == 0)
                    break;
            }
        }
    }

    close(sockfd); // 关闭服务器套接字
    return 0;
}
