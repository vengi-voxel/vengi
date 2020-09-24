/**
 * @file
 */
#pragma once

#include "TreeNode.h"

namespace backend {

/**
 * @brief A node for your real actions in the behaviour tree
 * @note This node doesn't support children
 */
class ITask: public TreeNode {
protected:
	ai::TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override {
		if (TreeNode::execute(entity, deltaMillis) == ai::TreeNodeStatus::CANNOTEXECUTE) {
			return ai::TreeNodeStatus::CANNOTEXECUTE;
		}

		return state(entity, doAction(entity, deltaMillis));
	}
public:
	ITask(const core::String& name, const core::String& parameters, const ConditionPtr& condition) :
			TreeNode(name, parameters, condition) {
	}

	virtual ~ITask() {
	}

	/**
	 * @note The returned @c TreeNodeStatus is automatically recorded. This method is only
	 * called when the attached @c ICondition evaluated to @c true
	 */
	virtual ai::TreeNodeStatus doAction(const AIPtr& entity, int64_t deltaMillis) = 0;

	bool addChild(const TreeNodePtr& /*child*/) override {
		return false;
	}
};

#define AI_TASK_DEFINITION(TaskName) \
/** \
 * @ingroup AI \
 */ \
struct TaskName: public ::backend::ITask { \
	TaskName(const core::String& name, const core::String& parameters, const ConditionPtr& condition) : \
			::backend::ITask(name, parameters, condition) {_type = #TaskName; } \
	virtual ~TaskName() {} \
	NODE_FACTORY(TaskName) \
	ai::TreeNodeStatus doAction(const AIPtr& entity, int64_t deltaMillis) override; \
};

#define AI_TASK_IMPL(TaskName) \
ai::TreeNodeStatus TaskName::doAction(const AIPtr& entity, int64_t deltaMillis)

#define AI_TASK(TaskName) \
	AI_TASK_DEFINITION(TaskName) \
	AI_TASK_IMPL(TaskName)

}
