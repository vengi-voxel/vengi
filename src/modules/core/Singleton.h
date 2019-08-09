/**
 * @file
 */

#pragma once

namespace core {

/**
 * @brief A singleton is only a singleton for the current thread.
 */
template<class T>
class Singleton {
public:
	static T& getInstance() {
		static thread_local T theInstance;
		return theInstance;
	}
};

}
