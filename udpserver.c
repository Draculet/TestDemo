
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

#define MAXSIZE		1024*1024 //buf size

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
	char buf[MAXSIZE]; // buf
	socklen_t clilen;

	struct epoll_event ev, events[20];

	epfd = epoll_create(512);

	struct sockaddr_in serveraddr;

	listenfd = socket(AF_INET, SOCK_DGRAM, 0);
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

	for(; ;) {
		nfds = epoll_wait(epfd, events, 20, -1);

		for(i = 0; i < nfds; ++i) {
			if(events[i].events & EPOLLIN) {
				if(events[i].data.fd < 0) continue;
				struct sockaddr_in client_addr;
				socklen_t len = sizeof(client_addr);
				int n = recvfrom(events[i].data.fd, buf, MAXSIZE, 0, (struct sockaddr*)&client_addr, &len);
				if (n < 0) printf("recvfrom failed\n");
				else printf("received %d byte(s) data from %s: %d\n", n, inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
			}
		}
	}
}
