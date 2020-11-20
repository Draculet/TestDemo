
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
		printf("Usage: ./tcpclient [ip] [port] [持续时间] [每多少秒] [发送数据量]\n");
		exit(-1);
	} 
	int i, connfd, sockfd, epfd, nfds;
	ssize_t n;
	char *buf = malloc(atoi(argv[5])); // buf
	memset(buf, (int)'a', atoi(argv[5]));
	socklen_t clilen;

	struct epoll_event ev = {0}, events[20];
	connfd = socket(AF_INET, SOCK_DGRAM, 0);
	epfd = epoll_create(512);
	
	//setnonblocking(listenfd);

	ev.data.fd = connfd;
	ev.events = EPOLLIN;
	
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

	bool stopped = false;
	int64_t endTime = 0;
	for(; ;) {
		nfds = epoll_wait(epfd, events, 20, -1);
		for(i = 0; i < nfds; ++i) {
			if(events[i].data.fd == timerfd) {
				printf("%s秒时间到\n", argv[4]);
				uint64_t count;
    			size_t n = read(timerfd, &count, sizeof(count));
				if (!stopped){
					timerfd_settime(timerfd, 0, &et, &et2);
					struct sockaddr_in dst = {0};
					dst.sin_family = AF_INET;
					dst.sin_addr.s_addr = inet_addr(argv[1]);
					dst.sin_port = htons(atoi(argv[2]));
					socklen_t len = sizeof(dst);
					sendto(connfd, buf, atoi(argv[5]), 0, (struct sockaddr *)&dst, len);
				} else {
					int64_t now = nowUsFromEpoch();
					if (now - endTime < 10000){ // 在计时误差内仍发送
						struct sockaddr_in dst = {0};
						dst.sin_family = AF_INET;
						dst.sin_addr.s_addr = inet_addr(argv[1]);
						dst.sin_port = htons(atoi(argv[2]));
						socklen_t len = sizeof(dst);
						sendto(connfd, buf, atoi(argv[5]), 0, (struct sockaddr *)&dst, len);
					}
					epoll_ctl(epfd, EPOLL_CTL_DEL,timerfd, &ev);
				}
			}
			else if (events[i].data.fd == endfd){
				printf("计时结束，共持续%s秒\n", argv[3]);
				struct epoll_event ev;
				uint64_t count;
    			size_t n = read(endfd, &count, sizeof(count));
				epoll_ctl(epfd, EPOLL_CTL_DEL, endfd, &ev);
				stopped = true;
				endTime = nowUsFromEpoch();
			}
			else if(events[i].events & EPOLLIN) {
				printf("fd %d readable\n", events[i].data.fd);
				//...do nothing
			}
		}
	}
}
