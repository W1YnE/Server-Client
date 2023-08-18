#include<unistd.h>
#include<cstdio>
#include<cstring>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<string.h>
using namespace std;
int main()
{
    int sockfd;
    struct sockaddr_in sock_addr;
    int i,j;
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd<0)
    {
        printf("套接字创建失败\n");
        return -1;
    }
    bzero(&sock_addr,sizeof(sock_addr));
    sock_addr.sin_family=AF_INET;
    sock_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    sock_addr.sin_port=htons(8000);
    
    int result = connect(sockfd,(struct sockaddr*)&sock_addr,sizeof(sock_addr));
    if(result<0)
    {
        perror("connect error");
        close(sockfd);
        return -1;
    }

    while(1)
    {
        char buf[1010]="";

        fgets(buf,sizeof(buf),stdin);
        send(sockfd,buf,strlen(buf),0);
        //printf("send:%s\n",buf);
        result=recv(sockfd,buf,strlen(buf),0);
        if(result > 0)
        {
            buf[result]='\0';
            printf("%s\n",buf);
        }
    }
    close(sockfd);
    return 0;
}