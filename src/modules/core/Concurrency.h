/**
 * @file
 */

#pragma once

#include <thread>
#include <algorithm>

namespace core {

inline uint32_t cpus() {
	return (std::max)(1u, std::thread::hardware_concurrency());
}

inline uint32_t halfcpus() {
	return (std::max)(1u, std::thread::hardware_concurrency() / 2u);
}

extern void setThreadName(const char *name);

enum class ThreadPriority {
	High, Normal, Low
};

extern void setThreadPriority(ThreadPriority prio);

}
