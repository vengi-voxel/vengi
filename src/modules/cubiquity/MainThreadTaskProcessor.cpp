#include "MainThreadTaskProcessor.h"

namespace Cubiquity {

MainThreadTaskProcessor gMainThreadTaskProcessor; //Our global instance

MainThreadTaskProcessor::MainThreadTaskProcessor() :
		TaskProcessor() {
}

MainThreadTaskProcessor::~MainThreadTaskProcessor() {
	mPendingTasks.clear();
}

void MainThreadTaskProcessor::addTask(Task* task) {
	mPendingTasks.push_back(task);
}

bool MainThreadTaskProcessor::hasTasks(void) {
	return !mPendingTasks.empty();
}

void MainThreadTaskProcessor::processOneTask(void) {
	if (!mPendingTasks.empty()) {
		Task* task = mPendingTasks.front();
		mPendingTasks.pop_front();
		task->process();
	}
}

void MainThreadTaskProcessor::processAllTasks(void) {
	while (!mPendingTasks.empty()) {
		Task* task = mPendingTasks.front();
		mPendingTasks.pop_front();
		task->process();
	}
}

}
