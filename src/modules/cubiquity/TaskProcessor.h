#pragma once

#include "Task.h"

namespace Cubiquity {

class TaskProcessor {
public:
	virtual ~TaskProcessor() {
	}

	virtual void addTask(Task* task) = 0;
};

}
