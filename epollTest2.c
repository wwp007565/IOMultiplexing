#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#define MAXLINE 10
#define OPEN_MAX 100
#define LISTENQ 20
#define SERV_PORT 5555
#define INFTIM 1000
#define LOCAL_HOST "192.168.30.55" // 改成你电脑的IP地址
void setnonblocking(int sock)
{
    int opts;
    opts=fcntl(sock,F_GETFL); // 把sock的属性查询出来，存到opts中
    if(opts<0)
    {
        perror("fcntl(sock,GETFL)");
        exit(1);
    }
    opts = opts | O_NONBLOCK; // 把opts中的某一位O_NONBLOCK置为生效
    if(fcntl(sock,F_SETFL,opts)<0) // 再设回去，搞定
    {
        perror("fcntl(sock,SETFL,opts)");
        exit(1);
    }
}
int main()

{
    int i, maxi, listenfd, connfd, sockfd, epfd, nfds;
    ssize_t n;
    char line[MAXLINE];
    socklen_t clilen;
    struct epoll_event ev,events[20]; //声明epoll_event结构体的变量, ev用于注册事件, events数组用于回传要处理的事件
    epfd=epoll_create(256); //生成用于处理accept的epoll专用的文件描述符, 指定生成描述符的最大范围为256
    struct sockaddr_in clientaddr;
    struct sockaddr_in serveraddr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    setnonblocking(listenfd); //把用于监听的socket设置为非阻塞方式
    ev.data.fd=listenfd; //设置与要处理的事件相关的文件描述符
    ev.events=EPOLLIN | EPOLLET; //设置要处理的事件类型和使用的触发器的机制
    epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev); //把listenfd注册到epfd队列中，监听EPOLLIN事件，使用EPOLLET机制的触发器
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    char *local_addr = LOCAL_HOST;
    inet_aton(local_addr,&(serveraddr.sin_addr));
    serveraddr.sin_port=htons(SERV_PORT);
    bind(listenfd,(struct sockaddr *)&serveraddr, sizeof(serveraddr));
    listen(listenfd, LISTENQ);
    maxi = 0;
    for( ; ; )
    {
        static int count = 0;
        nfds=epoll_wait(epfd,events,20,-1); //等待epoll事件的发生，第四个参数设置为-1，永久阻塞
        printf("[%d] nfds = [%d]\n", ++count, nfds); // 现实当前处于第几次epoll_wait内
        for(i=0; i<nfds; ++i) //处理所发生的所有事件
        {
            if(events[i].data.fd==listenfd)    /**当产生事件的fd和开始监听的fd相同，那么肯定是新客户端请求来了**/
            {
                connfd = accept(listenfd,(struct sockaddr *)&clientaddr, &clilen);
                if(connfd<0)
                {
                    perror("connfd<0");
                    exit(1);
                }
                setnonblocking(connfd); //把客户端的socket设置为非阻塞方式
                char *str = inet_ntoa(clientaddr.sin_addr);
                printf("connect from %s \n", str);
                ev.data.fd=connfd; //设置用于读操作的文件描述符
                ev.events=EPOLLIN | EPOLLET; //设置用于注测的读操作事件
                epoll_ctl(epfd,EPOLL_CTL_ADD,connfd,&ev); //注册ev事件
            }
            else if(events[i].events&EPOLLIN)     /**当这个fd上有读事件发生时**/
            {
                if ( (sockfd = events[i].data.fd) < 0)
                    continue; //因为下面设置了，所以这里考虑了
                if ( (n = read(sockfd, line, MAXLINE)) < 0) // 如果read返回值小于零，表示系统调用被中断，要分开考虑错误情况
                {
                    if (errno == ECONNRESET) // 表示连接被重置了，已经无效了，关闭它，删除它
                    {
                        printf("ECONNRESET: %s\n", strerror(ECONNRESET));
                        epoll_ctl(epfd,EPOLL_CTL_DEL,sockfd,NULL); // 可以把这个sockfd从epfd队列中删除了
                        close(sockfd);
                    }
                    else // 其他错误
                    {
                        printf("readline error\n");
                    }
                }
                else if (n == 0) // 有读事件触发，但是read返回0，所以是对面已经关闭socketfd了
                {
                    epoll_ctl(epfd,EPOLL_CTL_DEL,sockfd,NULL); // 可以把这个sockfd从epfd队列中删除了
                    close(sockfd);
                }
                else
                {
                    ev.data.fd=sockfd;
                    ev.events=EPOLLOUT | EPOLLET;
                    epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev); // 下次监听写事件
                }
            }
            else if(events[i].events&EPOLLOUT)    /**当这个fd上有写事件发生时**/

            {
                sockfd = events[i].data.fd;
                write(sockfd, line, n);
                ev.data.fd=sockfd;
                ev.events=EPOLLIN | EPOLLET;
                epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev); //修改sockfd上要处理的事件为EPOLIN，下次监听读事件
            }
        }
    }
} 
