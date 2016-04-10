#include "BackgroundTaskProcessor.h"

namespace Cubiquity {

BackgroundTaskProcessor gBackgroundTaskProcessor; //Our global instance

BackgroundTaskProcessor::BackgroundTaskProcessor() :
		TaskProcessor() {
}

BackgroundTaskProcessor::~BackgroundTaskProcessor() {
	_pendingTasks.clear();
}

void BackgroundTaskProcessor::addTask(Task* task) {
	_pendingTasks.push_back(task);
}

bool BackgroundTaskProcessor::hasTasks(void) {
	return _pendingTasks.size() > 0;
}

void BackgroundTaskProcessor::processOneTask(void) {
	if (_pendingTasks.size() > 0) {
		Task* task = _pendingTasks.front();
		_pendingTasks.pop_front();
		task->process();
	}
}

void BackgroundTaskProcessor::processAllTasks(void) {
	while (_pendingTasks.size() > 0) {
		Task* task = _pendingTasks.front();
		_pendingTasks.pop_front();
		task->process();
	}
}

}
