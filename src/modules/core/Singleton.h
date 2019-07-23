/**
 * @file
 */

#pragma once

#include <SDL.h>

namespace core {

/**
 * @brief A singleton is only a singleton for the current thread.
 */
template<class T>
class Singleton {
public:
	static T& getInstance() {
		// XCode8 clang will support thread_local, but XCode7's doesn't
#ifdef __MACOSX__
		static T theInstance;
#else
		static thread_local T theInstance;
#endif
		return theInstance;
	}
};

}
