/**
 * @file
 */

#pragma once

#include "backend/entity/ai/AICommon.h"
#include "backend/entity/ai/AICharacter.h"

using namespace ai;

namespace backend {

/**
 * @ingroup AI
 */
class Task: public ITask {
public:
	TASK_CLASS(Task)

	TreeNodeStatus doAction(const AIPtr& entity, int64_t deltaMillis) override {
		return doAction(ai::character_cast<AICharacter>(entity->getCharacter()), deltaMillis);
	}

	virtual TreeNodeStatus doAction(backend::AICharacter& chr, int64_t deltaMillis) = 0;
};

#define AI_TASK(TaskName) \
/** \
 * @ingroup AI \
 */ \
struct TaskName: public Task { \
	TaskName(const std::string& name, const std::string& parameters, const ConditionPtr& condition) : \
			Task(name, parameters, condition) {} \
	virtual ~TaskName() {} \
	NODE_FACTORY(TaskName) \
	TreeNodeStatus doAction(backend::AICharacter& chr, int64_t deltaMillis) override; \
}; \
inline TreeNodeStatus TaskName::doAction(backend::AICharacter& chr, int64_t deltaMillis)

}

