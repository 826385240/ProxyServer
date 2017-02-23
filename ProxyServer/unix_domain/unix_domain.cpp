#include "unix_domain.h"
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "../traceback/traceback.h"

unix_domain::unix_domain()
{
	int ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, fd_array);
	if (ret < 0) print_errno();
}

unix_domain::~unix_domain()
{
	int size = sizeof(fd_array) / sizeof(int);
	for (int i = 0; i < size; ++i)
	{
		close(fd_array[i]);
	}
}

int unix_domain::print_errno()
{
	if (errno != 0)
		printf("[%s %d function:%s] Error(%d): %s\n",__FILE__,__LINE__,__FUNCTION__, errno, strerror(errno));

	print_trace();

	return errno;
}

int unix_domain::set_proc_type(int type /*= PROC_TYPE_PARENT*/)
{
	if (type >= sizeof(fd_array) / sizeof(int))
		return -1;

	unix_domain_fd = fd_array[type];
	return 0;
}

int unix_domain::get_unix_domain_fd()
{
	return unix_domain_fd;
}

int unix_domain::send_fd(void* ptr, int nbytes, int to_send_fd)
{
	struct msghdr msg;
	struct iovec iov[1];

#ifdef HAVE_MSGHDR_MSG_CONTROL
	union {
		struct cmsghdr cm;
		char control[CMSG_SPACE(sizeof(int))];
	}control_un;
	struct cmsghdr *cmptr;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);

	cmptr = CMSG_FIRSTHDR(&msg);
	cmptr->cmsg_len = CMSG_LEN(sizeof(int));
	cmptr->cmsg_level = SOL_SOCKET;
	cmptr->cmsg_type = SCM_RIGHTS;
	*((int*)CMSG_DATA(cmptr)) = to_send_fd;
#else
	msg.msg_accrights = (caddr_t) &to_send_fd;
	msg.msg_accrightslen = sizeof(int);
#endif

	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	iov[0].iov_base = ptr;
	iov[0].iov_len = nbytes;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	int ret = sendmsg(unix_domain_fd, &msg , 0);
	if (ret < 0) print_errno();

	return ret;
}

int unix_domain::read_fd(void* ptr, int nbytes, int* to_read_fd)
{
	struct msghdr msg;
	struct iovec iov[1];

#ifdef HAVE_MSGHDR_MSG_CONTROL
	union {
		struct cmsghdr cm;
		char control[CMSG_SPACE(sizeof(int))];
	}control_un;
	struct cmsghdr *cmptr;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
#else
	int newfd;

	msg.msg_accrights = (caddr_t) &newfd;
	msg.msg_accrightslen = sizeof(int);
#endif

	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	iov[0].iov_base = ptr;
	iov[0].iov_len = nbytes;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	int ret = recvmsg(unix_domain_fd, &msg, 0);
	if (ret < 0){
		print_errno();
		return ret;
	}

#ifdef HAVE_MSGHDR_MSG_CONTROL
	if ((cmptr = CMSG_FIRSTHDR(&msg)) != NULL && cmptr->cmsg_len == CMSG_LEN(sizeof(int)))
	{
		if (cmptr->cmsg_level != SOL_SOCKET)
		{
			print_errno();
			return -1;
		}
		if (cmptr->cmsg_type != SCM_RIGHTS)
		{
			print_errno();
			return -1;
		}

		*to_read_fd = *((int*) CMSG_DATA(cmptr));
	}
	else
		*to_read_fd = -1;
#else
	if (msg.msg_accrightslen == sizeof(int))
		*to_read_fd = newfd;
	else
		*to_read_fd = -1;
#endif

	return 0;
}
