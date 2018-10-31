/**
 * @file
 */

#pragma once

#include "backend/entity/ai/AICommon.h"
#include "backend/entity/ai/AICharacter.h"

namespace backend {

/**
 * @ingroup AI
 */
class Task: public ai::ITask {
public:
	TASK_CLASS(Task)

	ai::TreeNodeStatus doAction(const ai::AIPtr& entity, int64_t deltaMillis) override {
		return doAction(ai::character_cast<AICharacter>(entity->getCharacter()), deltaMillis);
	}

	virtual ai::TreeNodeStatus doAction(backend::AICharacter& chr, int64_t deltaMillis) = 0;
};

#define AI_TASK(TaskName) \
/** \
 * @ingroup AI \
 */ \
struct TaskName: public Task { \
	TaskName(const std::string& name, const std::string& parameters, const ai::ConditionPtr& condition) : \
			Task(name, parameters, condition) {} \
	virtual ~TaskName() {} \
	NODE_FACTORY(TaskName) \
	ai::TreeNodeStatus doAction(backend::AICharacter& chr, int64_t deltaMillis) override; \
}; \
inline ai::TreeNodeStatus TaskName::doAction(backend::AICharacter& chr, int64_t deltaMillis)

}

