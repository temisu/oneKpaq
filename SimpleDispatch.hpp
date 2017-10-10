/* Copyright (c) Teemu Suutari */

#ifndef SIMPLEDISPATCH_H
#define SIMPLEDISPATCH_H

#ifdef __APPLE__

#include <dispatch/dispatch.h>

template<typename F,typename... Args>
void DispatchLoop(size_t iterations,size_t stride,F func,Args&&... args)
{
	if (!iterations) return;
	if (stride<1) stride=1;
	dispatch_apply((iterations+stride-1)/stride,dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT,0),^(size_t majorIt) {
		majorIt*=stride;
		for (size_t minorIt=0;minorIt<stride;minorIt++) {
			size_t iter=majorIt+minorIt;
			if (iter>=iterations) break;
			func(iter,args...);
		}
	});
}

#else
#warning "Parallelization disabled!!!"

template<typename F,typename... Args>
void DispatchLoop(size_t iterations,size_t stride,F func,Args&&... args)
{
	if (!iterations) return;
	// Take a coffee, come back, wait, go to lunch, come back, wait, take a coffee...
	for (size_t i=0;i<iterations;i++) func(i,args...);
}

#endif

#endif
