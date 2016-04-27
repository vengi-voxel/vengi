#include "tree/ITask.h"
#include "common/Types.h"

namespace ai {

ITask::ITask(const std::string& name, const std::string& parameters, const ConditionPtr& condition) :
		TreeNode(name, parameters, condition) {
}

ITask::~ITask() {
}

TreeNodeStatus ITask::execute(const AIPtr& entity, int64_t deltaMillis) {
	if (TreeNode::execute(entity, deltaMillis) == CANNOTEXECUTE)
		return CANNOTEXECUTE;

#if AI_EXCEPTIONS
	try {
#endif
		return state(entity, doAction(entity, deltaMillis));
#if AI_EXCEPTIONS
	} catch (...) {
		return state(entity, EXCEPTION);
	}
#endif
}

bool ITask::addChild(const TreeNodePtr& /*child*/) {
	return false;
}

}
