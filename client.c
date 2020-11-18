
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

#define MAXLINE		1024*1024 //buf size

int main(int argc, char *argv[])
{
	if (argc != 6)
	{
		printf("Usage: ./tcpclient [ip] [port] [持续时间] [每多少秒] [发送数据量]\n");
		exit(-1);
	} 
	int i, connfd, sockfd, epfd, nfds;
	ssize_t n;
	char *buf = malloc(atoi(argv[5])); // buf
	memset(buf, (int)'a', sizeof(buf));
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
	int endfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	struct itimerspec lt;
	struct itimerspec lt2;
	struct itimerspec et;
	struct itimerspec et2;
	memset(&lt, 0, sizeof(lt));
	memset(&et, 0, sizeof(et));
	double lastsec = atof(argv[3]);
	double eachsec = atof(argv[4]);
	struct timespec l;
	l.tv_sec = (long)(lastsec * 1000000) / 1000000;
	l.tv_nsec = (long)(lastsec * 1000000000) % 1000000000;
	lt.it_value = l;
	struct timespec e;
	e.tv_sec = (long)(eachsec * 1000000) / 1000000;;
	e.tv_nsec = (long)(eachsec * 1000000000) % 1000000000;;
	et.it_value = e;
	//设置 epoll监听
	struct epoll_event timeev = {0}, lastev = {0};
	timeev.data.fd = timerfd;
	timeev.events = EPOLLIN;
	lastev.data.fd = endfd;
	lastev.events = EPOLLIN;

	printf("键入任意内容开始计时\n");
	getchar();
	printf("计时开始\n");
	//计时开始
	timerfd_settime(endfd, 0, &lt, &lt2);
	timerfd_settime(timerfd, 0, &et, &et2);
	//加入监听
	epoll_ctl(epfd, EPOLL_CTL_ADD, timerfd, &timeev);
	epoll_ctl(epfd, EPOLL_CTL_ADD, endfd, &lastev);
	for(; ;) {
		nfds = epoll_wait(epfd, events, 20, -1);
		for(i = 0; i < nfds; ++i) {
			if(events[i].data.fd == timerfd) {
				printf("%s秒时间到\n", argv[4]);
				uint64_t count;
    			size_t n = read(timerfd, &count, sizeof(count));
				timerfd_settime(timerfd, 0, &et, &et2);
				write(connfd, buf, atoi(argv[5]));
			}
			else if (events[i].data.fd == endfd){
				printf("计时结束，共持续%s秒\n", argv[3]);
				struct epoll_event ev;
				uint64_t count;
    			size_t n = read(endfd, &count, sizeof(count));
				epoll_ctl(epfd, EPOLL_CTL_DEL,timerfd, &ev);
				epoll_ctl(epfd, EPOLL_CTL_DEL, endfd, &ev);
			}
			else if(events[i].events & EPOLLIN) {
				printf("fd %d readable\n", events[i].data.fd);
				//...do nothing
			}
		}
	}
}
