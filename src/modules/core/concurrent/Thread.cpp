/**
 * @file
 */

#include "Thread.h"
#include "core/Common.h"
#include "core/StandardLib.h"
#include <SDL_thread.h>
#include <SDL_version.h>
#include <new>

namespace core {

ThreadId getCurrentThreadId() {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	return SDL_GetCurrentThreadID();
#else
	return SDL_ThreadID();
#endif
}

static int threadTrampoline(void *data) {
	core::Function<void()> *func = (core::Function<void()> *)data;
	(*func)();
	func->~Function();
	core_free(func);
	return 0;
}

Thread::Thread(core::Function<void()> &&func, const char *name) {
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
