#include "buffer_manager.h"
#ifndef sock4_h
#define sock4_h

struct sock4_require_header
{
	unsigned char version;
	unsigned char cmd;
	short dst_port;
	int dst_ip;
	char usr_id[16];
};


class sock4_protocol
{
public:
	sock4_protocol(int sockfd);
	~sock4_protocol();

	bool have_analysed_data();
	int  analyse_socket_fd();
	bool is_analyse_success();

	int  get_req_sock_fd();
	int  get_conn_sock_fd();

	int	 read_data(int fd);//if return -1,read postion has closed,but we need to wait write position done if we want to close connetion
	int  send_data(int fd); //return data list size which not send,if return -1 means connetion is closed

	int  get_buffer_list_size(int fd);
private:
	int print_errno();
	int set_fd_unblock(int fd);

	int  _add_data(buffer_manager& buf_mgr, char* data_buf, int buf_size);
private:
	buffer_manager req_buffer_mgr;
	buffer_manager conn_buffer_mgr;

	int req_sock_fd; //the fd from require host
	int conn_sock_fd; //the fd current server have connected destination host
	short dst_port; //the destination port need to connect
	int dst_ip; //the destination ip need to connect

	bool have_analysed;
	bool close_connection;
	char* temp_buf;
};


#endif
