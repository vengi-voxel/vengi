/**
 * @file
 */

#pragma once

#include "backend/entity/ai/AICommon.h"
#include "backend/entity/ai/AICharacter.h"
#include "core/String.h"

using namespace ai;

namespace backend {

class Task: public ITask {
public:
	TASK_CLASS(Task)

	TreeNodeStatus doAction(const AIPtr& entity, long deltaMillis) override {
		return doAction(ai::character_cast<AICharacter>(entity->getCharacter()), deltaMillis);
	}

	virtual TreeNodeStatus doAction(backend::AICharacter& chr, long deltaMillis) = 0;
};

#define AI_TASK(TaskName) \
struct TaskName: public Task { \
	TaskName(const std::string& name, const std::string& parameters, const ConditionPtr& condition) : \
			Task(name, parameters, condition) {} \
	virtual ~TaskName() {} \
	NODE_FACTORY(TaskName) \
	TreeNodeStatus doAction(backend::AICharacter& chr, long deltaMillis) override; \
}; \
inline TreeNodeStatus TaskName::doAction(backend::AICharacter& chr, long deltaMillis)

}

