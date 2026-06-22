// 网络通讯的客户端程序。
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("usage:./client ip port\n"); 
        printf("example:./client 0.0.0.0 5005\n\n"); 
        return -1;
    }

    int sockfd;
    struct sockaddr_in servaddr;
    char buf[1024];
 
    if ((sockfd=socket(AF_INET,SOCK_STREAM,0))<0) { printf("socket() failed.\n"); return -1; }
    
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(atoi(argv[2]));
    servaddr.sin_addr.s_addr=inet_addr(argv[1]);

    if (connect(sockfd, (struct sockaddr *)&servaddr,sizeof(servaddr)) != 0)
    {
        printf("connect(%s:%s) failed.\n",argv[1],argv[2]); close(sockfd);  return -1;
    }

    printf("connect ok.\n");
    // printf("开始时间：%d",time(0));
    
    for (int ii=0;ii<5;ii++)
    {
        // 从命令行输入内容。
        memset(buf,0,sizeof(buf));
        //printf("please input:"); scanf("%s",buf);
        sprintf(buf, "这是第%d个矛盾存在", ii);
        uint32_t len = strlen(buf);
        uint32_t nlen = htonl(len);
        char tmpbuf[1024];
        memset(tmpbuf, 0, sizeof(tmpbuf));
        memcpy(tmpbuf, &nlen, 4);
        memcpy(tmpbuf + 4, buf, len);

        send(sockfd,tmpbuf, len + 4, 0);       // 把命令行输入的内容发送给服务端。
        
    }
    
    //sleep(2);
    //return 0;
    printf("现在开始接受回复内容\n");
    for(int ii =0;ii<5;ii++)
    {
        uint32_t len, nlen;
        recv(sockfd, &nlen, 4, 0);
        len  =  ntohl(nlen);
        memset(buf, 0, sizeof(buf));
        recv(sockfd, buf, len, 0);     
        printf("recv:%s\n",buf);
    }
    sleep(1);
    return 0;
    // printf("结束时间：%d",time(0));
   
} 
