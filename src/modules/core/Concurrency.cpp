/**
 * @file
 */

#include <SDL.h>

#if defined(__LINUX__)
#include <dlfcn.h>
#ifndef RTLD_DEFAULT
#define RTLD_DEFAULT nullptr
#endif
#include <pthread.h>
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

}
