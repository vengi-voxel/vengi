/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace core {

extern uint32_t cpus();
extern uint32_t halfcpus();

extern void setThreadName(const char *name);

enum class ThreadPriority {
	High, Normal, Low
};

extern void setThreadPriority(ThreadPriority prio);

}
