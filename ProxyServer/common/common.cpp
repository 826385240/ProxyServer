#include "common.h"
#include "../traceback/traceback.h"

int print_errno()
{
	if (errno != 0)
		printf("[%s %d function:%s] Error(%d): %s\n",__FILE__,__LINE__,__FUNCTION__, errno, strerror(errno));

	print_trace();

	return errno;
}