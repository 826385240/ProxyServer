#ifndef epoll_h
#define epoll_h

#define  EPOLL_TIME_OUT 10000000

class epoll_manager{
public:
	epoll_manager();
	~epoll_manager();

public:
	int epoll_ctl_add(int fd, int events);
	int epoll_ctl_mod(int fd, int events);
	int epoll_ctl_del(int fd, int events);

	int do_epoll_ctl(int op, int fd, int events);
	int do_epoll_wait(struct epoll_event * events, int maxevents, int timeout=EPOLL_TIME_OUT);
private:
	int print_errno();
private:
	int epoll_fd;
};

#endif
