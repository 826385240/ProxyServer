#ifndef common_h
#define common_h

#include <errno.h>
#include <string.h>
#include <stdio.h>

#define	SERVER_PORT 1080

#define CHECK_ERROR(retnum) \
if ( (retnum) < 0) return print_errno();

int print_errno();

#endif