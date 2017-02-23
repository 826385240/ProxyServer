#include "buffer_manager.h"
#include <sys/socket.h>
#include "../common/common.h"


buffer_node::buffer_node(char* buffer, int size) :cur_pos(0)
{
	if ( NULL != buffer)
	{
		data_buf = new char[size+1];
		memcpy(data_buf, buffer, size);

		buf_size = size;
	}
}

buffer_node::~buffer_node()
{
	if (NULL != data_buf)
		delete data_buf;
}

send_state buffer_node::send_data(int fd)
{
	if (fd <= 0 || cur_pos >= buf_size) 
		return SEND_FALSE;

	int real_write = send( fd, data_buf+cur_pos, buf_size-cur_pos, 0);
	printf("**********************send buffer: fd:%d , addr:%d , cur_pos:%d , buf_size:%d\n", fd, data_buf, cur_pos, buf_size);
	if (real_write < 0)
	{
		print_errno();
		return SEND_FALSE;
	}

	cur_pos += real_write;
	if (cur_pos >= buf_size)
		return SEND_ALL_DONE;
	else
		return SEND_REMAIN;
}


buffer_manager::buffer_manager(int fd) :sock_fd(fd)
{
}

buffer_manager::~buffer_manager()
{
	for (list<buffer_node*>::iterator it = buf_list.begin(); it != buf_list.end();++it)
	{
		if ( NULL != *it)
			delete *it;
	}
	buf_list.clear();
}

int buffer_manager::set_fd(int fd)
{
	sock_fd = fd;
}

int buffer_manager::get_buf_list_size()
{
	return buf_list.size();
}

int buffer_manager::add_buffer(char* buffer, int buffer_size)
{
	if (NULL == buffer || buffer_size <= 0)
		return -1;

	printf("==================add buffer: fd:%d , addr:%d , size:%d\n", sock_fd , buffer, buffer_size);
	buffer_node* buf_node = new buffer_node(buffer, buffer_size);
	if (NULL == buf_node) return -1;

	buf_list.push_back(buf_node);
	return 0;

}

int buffer_manager::start_send()
{
	if ( buf_list.size() > 0 && sock_fd > 0)
	{
		list<buffer_node*>::iterator it = buf_list.begin();
		if ( NULL != *it)
		{
			send_state state = (*it)->send_data(sock_fd);
			if (SEND_ALL_DONE == state || SEND_FALSE == state)
			{
				delete *it;
				buf_list.erase(it);
			}
		}
	}

	return buf_list.size();
}
