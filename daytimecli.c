#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#define MAXLINE 100

int
main(int argc, char **argv)
{
	int					sockfd, n;
	char				recvline[MAXLINE + 1];
	struct sockaddr_in	servaddr;

	if(argc != 2)
    {
		printf("usage: daytimecli <IPaddress>\n");
		return -1;
	}
	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		perror("create socket");

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port   = htons(13);	/* daytime server */
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
	{
		printf("inet_pton error for %s", argv[1]);
		return -1;
	}

	if (connect(sockfd, (struct sockaddr_in *) &servaddr, sizeof(servaddr)) < 0)
    {
		perror("connect.");
		return -1;
	}

	while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
		recvline[n] = 0;	/* null terminate */
		if (fputs(recvline, stdout) == EOF)
		{
			printf("fputs error.\n");
			return -1;
		}
	}
	if (n < 0)
	{
		printf("read error.\n")
		return -1;
	}

	return 0;
}
