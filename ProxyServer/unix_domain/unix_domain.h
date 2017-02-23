#ifndef unix_domain_protocol
#define unix_domain_protocol

#define PROC_TYPE_PARENT 0
#define PROC_TYPE_CHILD  1

#define HAVE_MSGHDR_MSG_CONTROL

class unix_domain
{
public:
	unix_domain();
	~unix_domain();

public:
	int set_proc_type(int type = PROC_TYPE_PARENT);
	int print_errno();

	int get_unix_domain_fd();
	int send_fd(void* ptr, int nbytes, int to_send_fd);
	int read_fd(void* ptr, int nbytes, int* to_read_fd);
private:
	int fd_array[2];
	int unix_domain_fd;
};

#endif
