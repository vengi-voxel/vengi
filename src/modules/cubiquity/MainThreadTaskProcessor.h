#pragma once

#include "TaskProcessor.h"

#include <list>

namespace Cubiquity {

class MainThreadTaskProcessor: public TaskProcessor {
public:
	MainThreadTaskProcessor();
	virtual ~MainThreadTaskProcessor();

	void addTask(Task* task);
	bool hasTasks(void);

	virtual void processOneTask(void)/* = 0*/;
	virtual void processAllTasks(void)/* = 0*/;

	std::list<Task*> mPendingTasks;
};

extern MainThreadTaskProcessor gMainThreadTaskProcessor;

}
