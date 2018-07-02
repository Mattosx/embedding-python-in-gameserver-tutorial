#ifndef _COMMON_HEADER
#define _COMMON_HEADER
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <ctime>

#include "format.h"

inline uint32_t getSystemTime()
{
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

#endif //_COMMON_HEADER