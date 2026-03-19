/**
 * @file
 */

#pragma once

#include "core/Function.h"

struct SDL_Thread;

namespace core {

typedef unsigned long ThreadId;

ThreadId getCurrentThreadId();

/**
 * @brief RAII wrapper around an SDL thread. Move-only, non-copyable.
 */
class Thread {
private:
	SDL_Thread *_thread = nullptr;

public:
	Thread() = default;
	Thread(core::Function<void()> &&func, const char *name);
	~Thread();

	Thread(const Thread &) = delete;
	Thread &operator=(const Thread &) = delete;

	Thread(Thread &&other);
	Thread &operator=(Thread &&other);

	bool joinable() const;
	void join();
};

} // namespace core
