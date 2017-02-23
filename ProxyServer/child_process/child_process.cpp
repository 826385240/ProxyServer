#include <locale.h>
#include "child_process.h"
#include "../thread_manager/thread_manager.h"
#include <stdio.h>

int do_child(unix_domain& ud)
{
	ud.set_proc_type(PROC_TYPE_CHILD);

	int fd_to_read = 0;
	thread_manger thread_mgr(6);
	char temp_buf[1500] = { 0 };
	while (ud.read_fd((void*)temp_buf, 1500, &fd_to_read) >= 0)
	{
		thread_mgr.add_fd(fd_to_read);
	}

	return 0;
}

