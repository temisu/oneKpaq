/* Copyright (c) Teemu Suutari */

#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>

template<typename F,typename... Args>
double Timer(F func,Args&&... args)
{
	struct timeval beforeTime;
	// should use something better that measures real CPU-time
	gettimeofday(&beforeTime,nullptr);
	func(args...);
	struct timeval afterTime;
	gettimeofday(&afterTime,nullptr);
	return afterTime.tv_sec-beforeTime.tv_sec+double(afterTime.tv_usec-beforeTime.tv_usec)/1000000.0;
}

#endif
