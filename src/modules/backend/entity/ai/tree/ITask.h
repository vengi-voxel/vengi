/**
 * @file
 */
#pragma once

#include "TreeNode.h"

namespace backend {

/**
 * @brief Macro for the constructor of a task. Just give the class name as parameter.
 */
#define TASK_CLASS_CTOR(TaskName) \
	TaskName(const core::String& name, const core::String& parameters, const ConditionPtr& condition) : \
			ITask(name, parameters, condition)
/**
 * @brief Macro for the destructor of a task. Just give the class name as parameter.
 */
#define TASK_CLASS_DTOR(TaskName) virtual ~TaskName()

/**
 * @brief Macro to simplify the task creation. Just give the class name as parameter.
 */
#define TASK_CLASS(TaskName) \
	TASK_CLASS_CTOR(TaskName) {}\
	TASK_CLASS_DTOR(TaskName) {}

/**
 * @brief A node for your real actions in the behaviour tree
 * @note This node doesn't support children
 */
class ITask: public TreeNode {
protected:
	ai::TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override {
		if (TreeNode::execute(entity, deltaMillis) == ai::CANNOTEXECUTE) {
			return ai::CANNOTEXECUTE;
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

}
