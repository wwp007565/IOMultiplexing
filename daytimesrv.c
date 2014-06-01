#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include	<time.h>
#define MAXEPOLLSIZE 100
#define LISTENQ 100
/*
 *    setnonblocking – 设置句柄为非阻塞方式      
 */
int setnonblocking(int sockfd)
{
	if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK) == -1) 
	{   
		return -1; 
	}   
	return 0;
}

int
main(int argc, char **argv)
{
	int					listenfd, connfd, nfds;
	struct sockaddr_in	servaddr;
	char				buff[MAXLINE];
	time_t				ticks;
    struct epoll_event ev;
	struct epoll_event events[MAXEPOLLSIZE];

	if((listenfd = Socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("create socket.");
		exit(1);
	}
    printf("socket 创建成功.\n");

	SetNonBlocking(listenfd);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(1025);	/* daytime server */

	if(bind(listenfd, (struct sockaddr_in *) &servaddr, sizeof(servaddr)) == -1)
	{
	   perror("bind");
	   exit(1);
	}
    printf("ip地址和端口绑定成功.\n");
   
	epfd = epoll_create(MAXEPOLLSIZE);
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
	ev.data.fd = listenfd;
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev) < 0)
	{
		fprintf(stderr, "epoll set insertion error: fd=%d\n", listenfd);
		return -1;
	}
    printf("监听socket加入epoll成功.\n");

	listen(listenfd, LISTENQ);

	while(1)
	{
		nfds = epoll_wait(epfd, events, 10000, -1);
		if(nfds<0 && errno != EINTR)
		{
			perror("epoll_wait");
			break;
		}
		else if(nfds > 0)
		{
			/*处理所有事件*/
			for(i=0; i < nfds; i++)
			{
				if(events[i].data.fd == listenfd)
				{
					while(1)
					{
						if((connfd = accept(listenfd, (sockaddr_in *) NULL, NULL)) == -1)
						{
							perror("accept");
							break;
						}
						ticks = time(NULL);
						snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
					   if(write(connfd, buff, strlen(buff)) == -1)
					   {
						   perror("write");
						   break;
					   }	  
					}
				}
			}
		}

		close(connfd);
	}
}
