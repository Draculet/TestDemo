
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE		1024*1024 //buf size
#define LISTENQ		20 //listen queue

void setnonblocking(int sock)
{
	int opts;
	opts = fcntl(sock, F_GETFL);

	if(opts < 0) {
		perror("fcntl(sock, GETFL)");
		exit(1);
	}

	opts = opts | O_NONBLOCK;

	if(fcntl(sock, F_SETFL, opts) < 0) {
		perror("fcntl(sock, SETFL, opts)");
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("Usage: ./tcpserver [port]\n");
		exit(-1);
	} 
	int i, listenfd, connfd, sockfd, epfd, nfds;
	ssize_t n;
	char line[MAXLINE]; // buf
	socklen_t clilen;

	struct epoll_event ev, events[20];

	epfd = epoll_create(512);

	struct sockaddr_in serveraddr;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	int one = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	setnonblocking(listenfd);

	ev.data.fd = listenfd;
	ev.events = EPOLLIN;

	epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(atoi(argv[1]));

	bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));

	listen(listenfd, LISTENQ);

	for(; ;) {
		nfds = epoll_wait(epfd, events, 20, 500);

		for(i = 0; i < nfds; ++i) {
			if(events[i].data.fd == listenfd) {
				struct sockaddr_in clientaddr = {0};
				connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clilen);
				if(connfd < 0) {
					perror("connfd < 0");
					exit(1);
				}
				setnonblocking(connfd);

				printf("connect from %s: %d, connfd: %d\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port, connfd);

				ev.data.fd = connfd;
				ev.events = EPOLLIN;
				epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
			}
			else if(events[i].events & EPOLLIN) {
				if((sockfd = events[i].data.fd) < 0) continue;
				if((n = read(sockfd, line, MAXLINE)) < 0) {
					if(errno == ECONNRESET) {
						close(sockfd);
						events[i].data.fd = -1;
					} else {
						printf("readline error");
					}
				} else if(n == 0) {
					printf("sockfd %d disconnect\n", sockfd);
					close(sockfd);
					events[i].data.fd = -1;
				}
				else
					printf("received %ld byte(s) data from sockfd %d\n", n, sockfd);
			}
		}
	}
}
