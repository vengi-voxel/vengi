/**
 * @file
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

namespace core {

extern uint32_t cpus();
extern uint32_t halfcpus();

extern bool setThreadName(const char *name);

enum class ThreadPriority {
	High, Normal, Low
};

extern size_t getThreadId();
extern void setThreadPriority(ThreadPriority prio);

}
