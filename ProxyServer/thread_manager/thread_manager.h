#ifndef thread_manager_h
#define thread_manager_h

#include <vector>
#include <queue>
#include <bits/pthreadtypes.h>

using namespace std;

#define	THREAD_COUNT  4


class thread_manger
{
public:
	thread_manger(int thread_count=THREAD_COUNT);
	~thread_manger();

	int get_thread_num();
	int add_fd(int fd);
private:
	static void* thread_fun(void* arg);
	int print_errno();
	void stop();
	unsigned int bkdrHash(const char * c_src);

	int add_fd_to_queue(int tid, int fd);
	int get_queue_size(int tid);
private:
	volatile bool is_run;
	typedef queue<int> fd_queue;
	vector<fd_queue*> fd_queue_cache;
	vector<pthread_t*> pthread_cache;
	vector<pthread_rwlock_t*> rw_lock_cache;

	int thread_num;
};

#endif