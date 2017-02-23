#include <unistd.h>
#include "common/common.h"
#include "unix_domain/unix_domain.h"
#include "parent_process/parent_process.h"
#include "child_process/child_process.h"
#include <stdio.h>

int main()
{
	//��ʼ��UNIX���׽���
	unix_domain ud;

	//�����ӽ���
	int pid = fork();
	//CHECK_ERROR(pid)

	if ( 0 == pid) //child
	{
		do_child(ud);
	} 
	else if (pid > 0) //parent
	{
		do_parent(ud);
	}

	return 0;
}
