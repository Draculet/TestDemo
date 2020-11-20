
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
#include <sys/timerfd.h>
#include <sys/time.h>
#include <time.h>
#include <stdbool.h>

#define MAXLINE		1024*1024 //buf size

int64_t  nowUsFromEpoch(){
	struct timeval tval;
    gettimeofday(&tval, NULL);
	return tval.tv_sec * 1000000 + tval.tv_usec;
}

int main(int argc, char *argv[])
{
	if (argc != 6)
	{
		printf("Usage: ./%s [ip] [port] [持续时间] [每多少秒] [发送数据量]\n", argv[0]);
		exit(-1);
	} 
	int i, connfd, sockfd, epfd, nfds;
	ssize_t n;
	char *buf = malloc(atoi(argv[5])); // buf
	memset(buf, (int)'a', atoi(argv[5]));
	socklen_t clilen;

	struct epoll_event ev = {0}, events[20];
	connfd = socket(AF_INET, SOCK_STREAM, 0);
	epfd = epoll_create(512);
	struct sockaddr_in serveraddr = {0};
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
	serveraddr.sin_port = htons(atoi(argv[2]));

	
	//setnonblocking(listenfd);

	ev.data.fd = connfd;
	ev.events = EPOLLIN;
	
	int res = connect(connfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (res == -1) {
		printf("连接失败\n");
		exit(-1);
	}
	epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
	printf("连接成功\n");
	//设置计时器
	int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	struct itimerspec et;
	struct itimerspec et2;
	memset(&et, 0, sizeof(et));
	double eachsec = atof(argv[4]);

	struct timespec e;
	e.tv_sec = (long)(eachsec * 1000000) / 1000000;;
	e.tv_nsec = (long)(eachsec * 1000000000) % 1000000000;;
	et.it_value = e;

	//发包数
	int times = atof(argv[3]) / atof(argv[4]);
	printf("total %d package\n", times);
	//设置 epoll监听
	struct epoll_event timeev = {0};
	timeev.data.fd = timerfd;
	timeev.events = EPOLLIN;


	printf("键入任意内容开始计时\n");
	getchar();
	printf("计时开始\n");
	//计时开始
	timerfd_settime(timerfd, 0, &et, &et2);
	//加入监听
	epoll_ctl(epfd, EPOLL_CTL_ADD, timerfd, &timeev);

	for(; ;) {
		nfds = epoll_wait(epfd, events, 20, -1);
		for(i = 0; i < nfds; ++i) {
			if(events[i].data.fd == timerfd) {
				printf("%s秒时间到\n", argv[4]);
				uint64_t count;
    			size_t n = read(timerfd, &count, sizeof(count));
				if (times > 0){
					timerfd_settime(timerfd, 0, &et, &et2);
					write(connfd, buf, atoi(argv[5]));
					times--;
				}
			}
			else if(events[i].events & EPOLLIN) {
				printf("fd %d readable\n", events[i].data.fd);
				//...do nothing
			}
		}
	}
}
