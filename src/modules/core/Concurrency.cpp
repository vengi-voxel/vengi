/**
 * @file
 */

#include "Concurrency.h"
#include <SDL.h>

#if defined(__LINUX__)
#include <dlfcn.h>
#ifndef RTLD_DEFAULT
#define RTLD_DEFAULT nullptr
#endif
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

namespace core {

void setThreadName(const char *name) {
#if defined(__LINUX__)
	int (*ppthread_setname_np)(pthread_t, const char*) = nullptr;
	void *fn = dlsym(RTLD_DEFAULT, "pthread_setname_np");
	ppthread_setname_np = (int(*)(pthread_t, const char*)) fn;
	if (ppthread_setname_np != nullptr) {
		ppthread_setname_np(pthread_self(), name);
	}
#endif
}

void setThreadPriority(ThreadPriority prio) {
#if defined(__LINUX__)
	int value;
	if (prio == ThreadPriority::Low) {
		value = 19;
	} else if (prio == ThreadPriority::High) {
		value = -20;
	} else {
		value = 0;
	}
	setpriority(PRIO_PROCESS, syscall(SYS_gettid), value);
#endif
}

}
