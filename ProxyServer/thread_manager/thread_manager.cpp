#include "thread_manager.h"
#include <stddef.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include "../sock4/sock4.h"
#include "../epoll/epoll.h"
#include <sys/epoll.h>
#include <stdlib.h>
#include "../traceback/traceback.h"

using namespace std;

#define EPOLL_SIZE 1024
#define EPOLL_WAIT_TIME 1000

#define CHECK_THREAD_RET(ret) if (ret != 0) { \
	print_errno(); exit(-1); }

struct thread_args
{
	int tid;
	thread_manger* pthread_mgr;
};


thread_manger::thread_manger(int thread_count) : thread_num(thread_count), is_run(true)
{
	int ret = 0;
	for (int i = 0; i < thread_num; ++i)
	{
		pthread_rwlock_t* pth_rw_lock = new pthread_rwlock_t();
		ret = pthread_rwlock_init(pth_rw_lock, NULL);
		CHECK_THREAD_RET(ret)
		rw_lock_cache.push_back(pth_rw_lock);

		pthread_t* pthread_id = new pthread_t();
		thread_args* ta = new thread_args();
		ta->tid = i;
		ta->pthread_mgr = this;
		ret = pthread_create(pthread_id, NULL, thread_fun, (void*)ta);
		CHECK_THREAD_RET(ret)
		pthread_cache.push_back(pthread_id);

		fd_queue_cache.push_back(new fd_queue());
	}
}

thread_manger::~thread_manger()
{
	stop();
	for (int i = 0; i < thread_num; ++i)
	{
		delete fd_queue_cache[i];
		pthread_rwlock_destroy(rw_lock_cache[i]);
		delete rw_lock_cache[i];
		delete pthread_cache[i];
	}
}

int thread_manger::get_thread_num()
{
	return thread_num;
}

void* thread_manger::thread_fun(void* arg)
{
	thread_args* pta = (thread_args*)arg;
	int tid = pta->tid;
	thread_manger* ptm = pta->pthread_mgr;
	delete pta;

	map<int, sock4_protocol*> req_fd_to_sock4;
	map<int, int> conn_fd_to_req_fd;
	epoll_manager epoll_mgr;
	int temp_fd = 0, queue_size = 0;
	epoll_event ee[EPOLL_SIZE] = { 0 };

	int epoll_ready_event = EPOLLIN, epoll_ready_fd = 0;
	while (ptm->is_run)
	{
		fd_queue temp;

		pthread_rwlock_t& pth_rw = *(ptm->rw_lock_cache[tid]);
		pthread_rwlock_wrlock(&pth_rw);
		fd_queue& fq = *(ptm->fd_queue_cache[tid]);
		if ( fq.size() > 0)
		{
			fq.swap(temp);
		}
		pthread_rwlock_unlock(&pth_rw);

		//add new connection to epoll
		queue_size = temp.size();
		for (int i = 0; i < queue_size; ++i)
		{
			temp_fd = temp.front();
			temp.pop();

			if ( temp_fd > 0)
			{
				req_fd_to_sock4.insert(make_pair(temp_fd, new sock4_protocol(temp_fd)));
				epoll_mgr.epoll_ctl_add(temp_fd, EPOLLIN | EPOLLERR);
			}
		}

		//wait events occur
		int ready_fds = epoll_mgr.do_epoll_wait(ee, EPOLL_SIZE, EPOLL_WAIT_TIME);


		for (int i = 0; i < ready_fds; ++i)
		{
			epoll_ready_event = ee[i].events;
			epoll_ready_fd = ee[i].data.fd;

			bool close_sock4_connetion = false;
			//find sock4_protocal object
			map<int, sock4_protocol*>::iterator it_sock4 = req_fd_to_sock4.find(epoll_ready_fd);
			map<int, int>::iterator it_fd = conn_fd_to_req_fd.find(epoll_ready_fd);
			if ( it_sock4 == req_fd_to_sock4.end())
			{
				if ( it_fd != conn_fd_to_req_fd.end() )
					it_sock4 = req_fd_to_sock4.find((*it_fd).second);
			}

			if ( it_sock4 == req_fd_to_sock4.end())
			{
				if (it_fd != conn_fd_to_req_fd.end())
					conn_fd_to_req_fd.erase(it_fd);

				epoll_mgr.epoll_ctl_del(epoll_ready_fd, EPOLLIN | EPOLLOUT | EPOLLERR);
				continue;
			}

			//deal with epoll events
			sock4_protocol& p = *((*it_sock4).second);
			if ( ee[i].events & EPOLLOUT) //deal with write event
			{
				printf("***deal out event\n");
				int ret = p.send_data(epoll_ready_fd);
				if ( ret == 0 )//no data need to send
				{
					epoll_mgr.epoll_ctl_mod(epoll_ready_fd, EPOLLIN | EPOLLERR);
				}
				else if (ret < 0)//connection should be closed
				{
					close_sock4_connetion = true;
				}
			}

			if ( ee[i].events & EPOLLIN) //deal with read event
			{
				printf("***deal in event\n");
				if ( false == p.have_analysed_data()) //build sock4 connection
				{
					p.analyse_socket_fd();
					if ( false == p.is_analyse_success())
					{
						req_fd_to_sock4.erase(it_sock4);
						epoll_mgr.epoll_ctl_del(epoll_ready_fd, EPOLLIN | EPOLLOUT | EPOLLERR);
					}
					else
					{
						conn_fd_to_req_fd.insert(make_pair(p.get_conn_sock_fd(), p.get_req_sock_fd()));
						epoll_mgr.epoll_ctl_add(p.get_conn_sock_fd(), EPOLLIN | EPOLLERR);
					}
				}
				else //read connetion data
				{
					if (p.read_data(epoll_ready_fd) < 0)
					{
						int another_sock_fd = epoll_ready_fd == p.get_req_sock_fd() ? p.get_conn_sock_fd() : p.get_req_sock_fd();

						if (p.get_buffer_list_size(another_sock_fd) > 0)
							epoll_mgr.epoll_ctl_del(epoll_ready_fd, EPOLLIN | EPOLLOUT | EPOLLERR);
						else
							close_sock4_connetion = true;
					}
					else
					{
						int need_write_fd = epoll_ready_fd == p.get_req_sock_fd() ? p.get_conn_sock_fd() : p.get_req_sock_fd();
						printf("==================  %d %d  ++++++++++++++++++++\n", p.get_conn_sock_fd(), p.get_req_sock_fd());
						epoll_mgr.epoll_ctl_mod(need_write_fd, EPOLLIN | EPOLLOUT | EPOLLERR);
					}
				}

			}

			if ( ee[i].events & EPOLLERR) //deal with error event
			{
				int another_sock_fd = epoll_ready_fd == p.get_req_sock_fd() ? p.get_conn_sock_fd() : p.get_req_sock_fd();
				if (p.get_buffer_list_size(another_sock_fd) > 0)
				{
					p.read_data(epoll_ready_fd); //set sock4_protocol's close_sock4_connetion be true 
					epoll_mgr.epoll_ctl_del(epoll_ready_fd, EPOLLIN | EPOLLOUT | EPOLLERR);
				}
				else
					close_sock4_connetion = true;
			}


			if ( true == close_sock4_connetion)
			{
				epoll_mgr.epoll_ctl_del(p.get_conn_sock_fd(), EPOLLIN | EPOLLOUT | EPOLLERR);
				epoll_mgr.epoll_ctl_del(p.get_req_sock_fd(), EPOLLIN | EPOLLOUT | EPOLLERR);

				map<int, int>::iterator it_temp = conn_fd_to_req_fd.find(p.get_conn_sock_fd());
				if ( it_temp != conn_fd_to_req_fd.end())
					conn_fd_to_req_fd.erase(it_temp);

				if (it_sock4 != req_fd_to_sock4.end())
				{
					delete (*it_sock4).second;
					req_fd_to_sock4.erase(it_sock4);
				}
			}
		}
	}

	return 0;
}

int thread_manger::print_errno()
{
	if (errno != 0)
		printf("[%s %d function:%s] Error(%d): %s\n",__FILE__,__LINE__,__FUNCTION__, errno, strerror(errno));

	print_trace();
	return errno;
}

void thread_manger::stop()
{
	is_run = false;
	void* ret_code;
	int ret = 0;

	for (int i = 0; i < thread_num; ++i)
	{
		ret = pthread_join(*pthread_cache[i], &ret_code);
		if (ret < 0) print_errno();
	}
}

unsigned int thread_manger::bkdrHash(const char * c_src)
{
	if (NULL == c_src)
	{
		return 0;
	}

	unsigned int seed = 1313131; // 31 131 1313 13131 131313 etc..
	unsigned int hash = 0;

	const unsigned char* p = (const unsigned char *)c_src;
	while (*p)
	{
		hash = (hash * seed) + (*p);
		++p;
	}
	return hash;
}

int thread_manger::add_fd(int fd)
{
	char convert[16] = {0};
	sprintf(convert, "%d", fd);

	int tid = 0;
	if ( strlen(convert) > 0)
	{
		tid = bkdrHash(convert) % thread_num;
	}

	return add_fd_to_queue(tid, fd);
}

int thread_manger::add_fd_to_queue(int tid, int fd)
{
	pthread_rwlock_wrlock(rw_lock_cache[tid]);
	fd_queue_cache[tid]->push(fd);
	pthread_rwlock_unlock(rw_lock_cache[tid]);

	return 0;
}

int thread_manger::get_queue_size(int tid)
{
	int size = 0;
	pthread_rwlock_rdlock(rw_lock_cache[tid]);
	size = fd_queue_cache[tid]->size();
	pthread_rwlock_unlock(rw_lock_cache[tid]);

	return size;
}
