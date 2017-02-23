#include "sock4.h"
#include "stdio.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "../epoll/epoll.h"
#include "../traceback/traceback.h"

#define	READ_MAX_SIZE 512000

sock4_protocol::sock4_protocol(int sockfd) :
req_sock_fd(-1), 
conn_sock_fd(-1), 
dst_port(-1), 
dst_ip(-1), 
have_analysed(false),
close_connection(false)
{
	req_sock_fd = sockfd;
	temp_buf = new char[READ_MAX_SIZE];
}

sock4_protocol::~sock4_protocol()
{
	if ( req_sock_fd > 0)
	{
		close(req_sock_fd);
	}

	if ( conn_sock_fd > 0)
	{
		close( conn_sock_fd );
	}
	if ( NULL != temp_buf)
	{
		delete temp_buf;
	}
}


int sock4_protocol::print_errno()
{
	if (errno != 0)
		printf("[%s %d function:%s] Error(%d): %s\n",__FILE__,__LINE__,__FUNCTION__, errno, strerror(errno));

	print_trace();
	return errno;
}

int sock4_protocol::set_fd_unblock(int fd)
{
	int flag=fcntl(fd,F_GETFL,0);
	int ret = fcntl(fd ,F_SETFL,flag | O_NONBLOCK);
	if (ret < 0) print_errno();

	return ret;
}

int sock4_protocol::_add_data(buffer_manager& buf_mgr, char* data_buf, int buf_size)
{
	return buf_mgr.add_buffer(data_buf, buf_size);
}

int sock4_protocol::analyse_socket_fd()
{
	have_analysed = true;

	char data_buf[1500] = { 0 };
	int real_size = read(req_sock_fd, data_buf, sizeof(data_buf));
	printf("analyse_socket_fd:read return value is %d!\n", real_size);
	if (real_size < 0) {
		print_errno();
		return real_size;
	}

	if ( real_size == 0 ) 
	{
		return 0;
	}
	else if (real_size > 0)
	{
		sock4_require_header* pheader = (sock4_require_header*)data_buf;
		printf("version:%d , cmd:%d , dst_port:%d , dst_ip:%d %d %d %d,uid:%d\n", pheader->version, pheader->cmd, ntohs(pheader->dst_port)
			, ((unsigned char*)&(pheader->dst_ip))[0] , ((unsigned char*)&(pheader->dst_ip))[1], ((unsigned char*)&(pheader->dst_ip))[2], ((unsigned char*)&(pheader->dst_ip))[3],pheader->usr_id);

		dst_port = ntohs(pheader->dst_port);
		dst_ip = pheader->dst_ip;

		//connect destination host
		int dst_host_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (dst_host_fd < 0){
			print_errno();
			return dst_host_fd;
		}

		sockaddr_in dst_sock_addr;
		dst_sock_addr.sin_family = AF_INET;
		dst_sock_addr.sin_port = htons(dst_port);
		dst_sock_addr.sin_addr.s_addr = dst_ip;
		int ret = connect(dst_host_fd, (sockaddr*)&dst_sock_addr, sizeof(sockaddr_in));
		if (ret < 0){
			close(dst_host_fd);
			print_errno();
			return ret;
		}

		data_buf[0] = 0;
		data_buf[1] = 0x5A;
		int real_write = send(req_sock_fd, data_buf, 9 ,0);
		if (real_write < 0) {
			close(dst_host_fd);
			print_errno();
			return real_write;
		}

		conn_sock_fd = dst_host_fd;

		set_fd_unblock(req_sock_fd);
		set_fd_unblock(conn_sock_fd);
		req_buffer_mgr.set_fd(req_sock_fd);
		conn_buffer_mgr.set_fd(conn_sock_fd);
		return 0;
	}
}

bool sock4_protocol::have_analysed_data()
{
	return have_analysed;
}

bool sock4_protocol::is_analyse_success()
{
	return conn_sock_fd > 0;
}

int sock4_protocol::get_req_sock_fd()
{
	return req_sock_fd;
}

int sock4_protocol::get_conn_sock_fd()
{
	return conn_sock_fd;
}

int sock4_protocol::read_data(int fd)
{
	buffer_manager& buf_mgr = fd == req_sock_fd ? conn_buffer_mgr : req_buffer_mgr;

	int cur_read_pos = 0;
	bool success = true;
	while (true)
	{
		int real_read = read(fd, temp_buf+cur_read_pos, READ_MAX_SIZE-cur_read_pos);
		printf("fd %d:read return value is %d!\n", fd, real_read);
		if (real_read < 0) //read error
		{
			if (errno == EAGAIN)
			{
				_add_data(buf_mgr, temp_buf, cur_read_pos);
			}
			else
				success = false;

			break;
		}
		else if(real_read == 0) //another has closed connection
		{
			_add_data(buf_mgr, temp_buf, cur_read_pos);
			success = false;

			break;
		}
		else if (real_read > 0)//read data successs
		{
			cur_read_pos += real_read;
		}
	}

	if ( false == success)
	{
		close_connection = true;
		return -1;
	}

	return 0;
}

int sock4_protocol::send_data(int fd)
{
	buffer_manager& buf_mgr = fd == req_sock_fd ? req_buffer_mgr : conn_buffer_mgr;

	int buf_list_size = buf_mgr.start_send();
	if ( close_connection && buf_list_size == 0)
	{
		return -1;
	}

	return buf_list_size;
}

int sock4_protocol::get_buffer_list_size(int fd)
{
	buffer_manager& buf_mgr = fd == req_sock_fd ? req_buffer_mgr : conn_buffer_mgr;

	return buf_mgr.get_buf_list_size();
}
