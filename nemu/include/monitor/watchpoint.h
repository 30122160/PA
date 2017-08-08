#ifndef ___H__
#define ___H__

#include "common.h"

typedef struct watchpoint {
	int NO;//编号
	struct watchpoint *next;
	char args[32];
	int num;//表达式初始值
	/* TODO: Add more members if necessary */

} WP;

#endif
