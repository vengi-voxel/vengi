/**
 * @file
 */
#pragma once

#include "tree/TreeNode.h"
#include "common/Types.h"

namespace ai {

/**
 * @brief Macro for the constructor of a task. Just give the class name as parameter.
 */
#define TASK_CLASS_CTOR(TaskName) \
	TaskName(const std::string& name, const std::string& parameters, const ::ai::ConditionPtr& condition) : \
			::ai::ITask(name, parameters, condition)
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
	TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override {
		if (TreeNode::execute(entity, deltaMillis) == CANNOTEXECUTE) {
			return CANNOTEXECUTE;
		}

#if AI_EXCEPTIONS
		try {
#endif
			return state(entity, doAction(entity, deltaMillis));
#if AI_EXCEPTIONS
		} catch (...) {
			ai_log_error("Exception while running task %s of type %s", _name.c_str(), _type.c_str());
		}
		return state(entity, EXCEPTION);
#endif
	}
public:
	ITask(const std::string& name, const std::string& parameters, const ConditionPtr& condition) :
			TreeNode(name, parameters, condition) {
	}

	virtual ~ITask() {
	}

	/**
	 * @note The returned @c TreeNodeStatus is automatically recorded. This method is only
	 * called when the attached @c ICondition evaluated to @c true
	 */
	virtual TreeNodeStatus doAction(const AIPtr& entity, int64_t deltaMillis) = 0;

	bool addChild(const TreeNodePtr& /*child*/) override {
		return false;
	}
};

}
