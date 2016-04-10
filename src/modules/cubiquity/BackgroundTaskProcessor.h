#pragma once

#include "ConcurrentQueue.h"
#include "TaskProcessor.h"

#include <list>

namespace Cubiquity {

class BackgroundTaskProcessor: public TaskProcessor {
public:
	BackgroundTaskProcessor();
	virtual ~BackgroundTaskProcessor();

	void addTask(Task* task);
	bool hasTasks(void);

	virtual void processOneTask(void)/* = 0*/;
	virtual void processAllTasks(void)/* = 0*/;

	std::list<Task*> _pendingTasks;
};

}
