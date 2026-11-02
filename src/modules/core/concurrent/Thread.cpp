/**
 * @file
 */

#include "Thread.h"
#include "core/Common.h"
#include "core/StandardLib.h"
#include <SDL3/SDL_thread.h>
#include <SDL3/SDL_version.h>
#if defined(__ANDROID__)
#include "engine-config.h"
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_system.h>
#endif
#include <new>

namespace core {

ThreadId getCurrentThreadId() {
	return SDL_GetCurrentThreadID();
}

static int threadTrampoline(void *data) {
	core::Function<void()> *func = (core::Function<void()> *)data;
	(*func)();
	func->~Function();
	core_free(func);
	return 0;
}

Thread::Thread(core::Function<void()> &&func, const char *name) {
#if defined(__ANDROID__)
	if (SDL_GetAndroidJNIEnv() == nullptr) {
		SDL_SetHint(SDL_HINT_APP_NAME, PROJECT_NAME);
	}
#endif
	void *mem = core_malloc(sizeof(core::Function<void()>));
	core::Function<void()> *stored = new (mem) core::Function<void()>(core::move(func));
	_thread = SDL_CreateThread(threadTrampoline, name, stored);
	if (_thread == nullptr) {
		stored->~Function();
		core_free(stored);
	}
}

Thread::~Thread() {
	join();
}

Thread::Thread(Thread &&other) : _thread(other._thread) {
	other._thread = nullptr;
}

Thread &Thread::operator=(Thread &&other) {
	if (this != &other) {
		join();
		_thread = other._thread;
		other._thread = nullptr;
	}
	return *this;
}

bool Thread::joinable() const {
	return _thread != nullptr;
}

void Thread::join() {
	if (_thread != nullptr) {
		SDL_WaitThread(_thread, nullptr);
		_thread = nullptr;
	}
}

}
