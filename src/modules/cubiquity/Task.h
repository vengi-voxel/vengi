#pragma once

#include "PolyVox/ErrorHandling.h"
#include <cstdint>

namespace Cubiquity {
class Task {
public:
	virtual ~Task() {}

	virtual void process(void) = 0;

	uint32_t priority = 0;
};

class TaskSortCriterion {
public:
	bool operator()(const Task* task1, const Task* task2) const {
		return task1->priority < task2->priority;
	}
};

}
