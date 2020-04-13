/**
 * @file
 */

#include "Concurrency.h"
#include "core/Common.h"
#include "core/Log.h"
#include <SDL_platform.h>
#include <thread>

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
#elif defined(__WINDOWS__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace core {

bool setThreadName(const char *name) {
#if defined(__LINUX__)
	int (*ppthread_setname_np)(pthread_t, const char*) = nullptr;
	void *fn = dlsym(RTLD_DEFAULT, "pthread_setname_np");
	ppthread_setname_np = (int(*)(pthread_t, const char*)) fn;
	if (ppthread_setname_np != nullptr) {
		const int err = ppthread_setname_np(pthread_self(), name);
		if (err == ERANGE) {
			Log::error("Thread name is too long - max 15 chars, got %s", name);
		} else if (err != 0) {
			Log::error("Can't set thread name: %i", err);
		}
		return err == 0;
	}
#elif defined(__WINDOWS__)
	typedef HRESULT (WINAPI *pfnSetThreadDescription)(HANDLE, PCWSTR);
	static pfnSetThreadDescription pSetThreadDescription = nullptr;
	static HMODULE kernel32 = nullptr;

	if (kernel32 == nullptr) {
		kernel32 = LoadLibraryW(L"kernel32.dll");
		if (kernel32 != nullptr) {
			pSetThreadDescription = (pfnSetThreadDescription)GetProcAddress(kernel32, "SetThreadDescription");
		}
	}

	if (pSetThreadDescription != nullptr) {
		wchar_t wname[512];
		mbstowcs(wname, name, 512);
		pSetThreadDescription(GetCurrentThread(), wname);
		return true;
	}
#endif
	return false;
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
#elif defined(__WINDOWS__)
	int value;

	if (prio == ThreadPriority::Low) {
		value = THREAD_PRIORITY_LOWEST;
	} else if (prio == ThreadPriority::High) {
		value = THREAD_PRIORITY_TIME_CRITICAL;
	} else {
		value = THREAD_PRIORITY_NORMAL;
	}
	SetThreadPriority(GetCurrentThread(), value);
#endif
}

uint32_t cpus() {
	return core_max(1u, std::thread::hardware_concurrency());
}

uint32_t halfcpus() {
	return core_max(1u, std::thread::hardware_concurrency() / 2u);
}

size_t getThreadId() {
	return std::hash<std::thread::id>{}(std::this_thread::get_id());
}

}
