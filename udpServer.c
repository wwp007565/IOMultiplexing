#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
//#include <openssl/ssl.h>
//#include <openssl/err.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
#include <assert.h>

//#include "oci_api.h"

#define MAXBUF 1024
#define MAXEPOLLSIZE 100

/*
   setnonblocking – 设置句柄为非阻塞方式
   */
int setnonblocking(int sockfd)
{
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK) == -1)
    {
        return -1;
    }
    return 0;
}


/*
   pthread_handle_message – 线程处理 socket 上的消息收发
   */
void* pthread_handle_message(int* sock_fd)
{
    char recvbuf[MAXBUF + 1];
    char sendbuf[MAXBUF+1];
    int  ret;
    int  new_fd;
    struct sockaddr_in client_addr;
    socklen_t cli_len=sizeof(client_addr);

    new_fd=*sock_fd; 
    /* 开始处理每个新连接上的数据收发 */
    bzero(recvbuf, MAXBUF + 1);
    bzero(sendbuf, MAXBUF + 1);
    /* 接收客户端的消息 */
    ret = recvfrom(new_fd, recvbuf, MAXBUF, 0, (struct sockaddr *)&client_addr, &cli_len);
    if (ret > 0)
    {
        printf("socket %d 接收到来自:%s:%d的消息成功:’%s’，共%d个字节的数据\n",
             new_fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), recvbuf, ret);
    }
    else
    {
            printf("消息接收失败！错误代码是%d，错误信息是’%s’/n",
                 errno, strerror(errno));
        
    }
    /* 处理每个新连接上的数据收发结束 */ 
    //printf("pthread exit!");
    fflush(stdout); 
    pthread_exit(NULL);
}

 

int main(int argc, char **argv)
{
    int listener, kdpfd, nfds, n, curfds;
    socklen_t len;
    struct sockaddr_in my_addr, their_addr;
    unsigned int myport;
    struct epoll_event ev;
    struct epoll_event events[MAXEPOLLSIZE];
    struct rlimit rt;

    myport = 1234;

    pthread_t thread;
    pthread_attr_t attr;


    /* 设置每个进程允许打开的最大文件数 */
    rt.rlim_max = rt.rlim_cur = MAXEPOLLSIZE;
    if (setrlimit(RLIMIT_NOFILE, &rt) == -1) 
    {
        perror("setrlimit");
        exit(1);
    }
    else 
    {
        printf("设置系统资源参数成功！\n");
    }
    /* 开启 socket 监听 */
    if ((listener = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket 创建失败！");
        exit(1);
    }
    else
    {
        printf("socket 创建成功！\n");
    }

    /*设置socket属性，端口可以重用*/
    int opt=SO_REUSEADDR;
    setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    setnonblocking(listener);
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = PF_INET;
    my_addr.sin_port = htons(myport);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listener, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) == -1) 
    {
        perror("bind");
        exit(1);
    } 
    else
    {
        printf("IP 地址和端口绑定成功\n");
    }
   

    /* 创建 epoll 句柄，把监听 socket 加入到 epoll 集合里 */
    kdpfd = epoll_create(MAXEPOLLSIZE);
    len = sizeof(struct sockaddr_in);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listener;
    if (epoll_ctl(kdpfd, EPOLL_CTL_ADD, listener, &ev) < 0) 
    {
        fprintf(stderr, "epoll set insertion error: fd=%d\n", listener);
        return -1;
    }
    else
    {
        printf("监听 socket 加入 epoll 成功！\n");
    }
   
    while (1) 
    {
        /* 等待有事件发生 */
        nfds = epoll_wait(kdpfd, events, 10000, -1);
        if (nfds == -1)
        {
            perror("epoll_wait");
            break;
        }
        /* 处理所有事件 */
        for (n = 0; n < nfds; ++n)
        {
            if (events[n].data.fd == listener) 
            {
                /*初始化属性值，均设为默认值*/
                pthread_attr_init(&attr);
                pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
                /*  设置线程为分离属性*/ 
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

                if(pthread_create(&thread,&attr,(void*)pthread_handle_message,(void*)&(events[n].data.fd)))
                {
                        perror("pthread_creat error!");
                        exit(-1);
                } 
            } 
        }
    }
    close(listener);
    return 0;
}
