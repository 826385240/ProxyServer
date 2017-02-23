#ifndef buffer_manager_h
#define buffer_manager_h

#include<list>
using namespace std;

enum send_state
{
	SEND_FALSE,
	SEND_ALL_DONE,
	SEND_REMAIN,
};

class buffer_node
{
public:
	buffer_node(char* buffer ,int size);
	~buffer_node();

	send_state send_data(int fd);
private:
	int buf_size;
	int cur_pos;
	char* data_buf;
};


class buffer_manager
{
public:
	buffer_manager(int fd);
	buffer_manager(){};
	~buffer_manager();

	int set_fd(int fd);
	int get_buf_list_size();
	int add_buffer(char* buffer,int buffer_size);
	int start_send(); //return buf_list size
private:
	int sock_fd;

	list<buffer_node*> buf_list;
};

#endif
