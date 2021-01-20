/**
 * @file
 */

#include "core/String.h"

struct SDL_Thread;

namespace core {

typedef int (* ThreadFunction) (void *data);
typedef unsigned long ThreadId;

class Thread {
private:
	mutable SDL_Thread *_thread;

public:
	Thread(const core::String &name, ThreadFunction fn, void *data = nullptr);
	~Thread();

	void detach();
	int join();

	bool joinable() const;

	ThreadId id() const;
};

inline bool Thread::joinable() const {
	return _thread != nullptr;
}

}
