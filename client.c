
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

struct timeval now(){
	struct timeval tval;
    gettimeofday(&tval, NULL);
	return tval;
}

int64_t getUsFromEpoch(struct timeval *tval)
{
    return tval->tv_sec * 1000000 + tval->tv_usec;
}

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
		printf("connect failed\n");
		exit(-1);
	}
	epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
	printf("connect success\n");
	//设置计时器
	int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	int endfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	struct itimerspec lt;
	struct itimerspec lt2;
	struct itimerspec et;
	struct itimerspec et2;
	memset(&lt, 0, sizeof(lt));
	memset(&et, 0, sizeof(et));
	int64_t lastsec = atol(argv[3]);
	int64_t eachsec = atol(argv[4]);
	struct timespec l;
	l.tv_sec = lastsec;
	l.tv_nsec = 0;
	lt.it_value = l;
	struct timespec e;
	e.tv_sec = eachsec;
	e.tv_nsec = 0;
	et.it_value = e;
	//设置 epoll监听
	struct epoll_event timeev = {0}, lastev = {0};
	timeev.data.fd = timerfd;
	timeev.events = EPOLLIN;
	lastev.data.fd = endfd;
	lastev.events = EPOLLIN;

	printf("Enter Any Key to Continue\n");
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
				printf("%ld 秒时间到\n", et.it_value.tv_sec);
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
