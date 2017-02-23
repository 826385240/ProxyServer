#include "epoll.h"
#include "stdio.h"
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include "../traceback/traceback.h"

epoll_manager::epoll_manager() :epoll_fd(-1)
{
	epoll_fd = epoll_create(1);
}

epoll_manager::~epoll_manager()
{
	if ( epoll_fd > 0){
		close(epoll_fd);
	}
}

int epoll_manager::epoll_ctl_add(int fd, int events)
{
	epoll_event ee;
	ee.data.fd = fd;
	ee.events = events;

	int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ee);
	if (ret < 0) print_errno();

	return ret;
}

int epoll_manager::epoll_ctl_mod(int fd, int events)
{
	epoll_event ee;
	ee.data.fd = fd;
	ee.events = events;

	int ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ee);
	if (ret < 0) print_errno();

	return ret;
}

int epoll_manager::epoll_ctl_del(int fd, int events)
{
	epoll_event ee;
	ee.data.fd = fd;
	ee.events = events;

	int ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &ee);
	if (ret < 0)
	{
		printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@  fd:%d @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n",fd);
		print_errno();
	}
	
	return ret;
}

int epoll_manager::do_epoll_ctl(int op, int fd, int events)
{
	epoll_event ee;
	ee.data.fd = fd;
	ee.events = events;

	int ret = epoll_ctl(epoll_fd, op, fd, &ee);
	if (ret < 0) print_errno();

	return ret;
}

int epoll_manager::do_epoll_wait(struct epoll_event * events, int maxevents, int timeout)
{
	int ret = epoll_wait(epoll_fd, events, maxevents, timeout);
	if (ret < 0) print_errno();

	return ret;
}

int epoll_manager::print_errno()
{
	if (errno != 0)
		printf("[%s %d function:%s] Error(%d): %s\n",__FILE__,__LINE__,__FUNCTION__, errno, strerror(errno));

	print_trace();

	return errno;
}

