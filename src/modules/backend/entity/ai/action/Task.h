/**
 * @file
 */

#pragma once

#include "backend/entity/ai/AICharacter.h"
#include "backend/entity/ai/tree/ITask.h"

namespace backend {

/**
 * @ingroup AI
 */
class Task: public ITask {
public:
	TASK_CLASS(Task)

	ai::TreeNodeStatus doAction(const AIPtr& entity, int64_t deltaMillis) override;

	virtual ai::TreeNodeStatus doAction(AICharacter& chr, int64_t deltaMillis) = 0;
};

#define AI_TASK(TaskName) \
/** \
 * @ingroup AI \
 */ \
struct TaskName: public Task { \
	TaskName(const core::String& name, const core::String& parameters, const ConditionPtr& condition) : \
			Task(name, parameters, condition) {} \
	virtual ~TaskName() {} \
	NODE_FACTORY(TaskName) \
	ai::TreeNodeStatus doAction(AICharacter& chr, int64_t deltaMillis) override; \
}; \
inline ai::TreeNodeStatus TaskName::doAction(AICharacter& chr, int64_t deltaMillis)

}

