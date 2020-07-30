/**
 * @file
 */

#include "Succeed.h"
#include "core/Log.h"

namespace backend {

ai::TreeNodeStatus Succeed::execute(const AIPtr& entity, int64_t deltaMillis) {
	if (_children.size() != 1) {
		Log::error("Succeed must have exactly one child");
		return ai::TreeNodeStatus::EXCEPTION;
	}

	if (TreeNode::execute(entity, deltaMillis) == ai::TreeNodeStatus::CANNOTEXECUTE) {
		return ai::TreeNodeStatus::CANNOTEXECUTE;
	}

	const TreeNodePtr& treeNode = *_children.begin();
	const ai::TreeNodeStatus status = treeNode->execute(entity, deltaMillis);
	if (status == ai::TreeNodeStatus::RUNNING) {
		return state(entity, ai::TreeNodeStatus::RUNNING);
	}
	return state(entity, ai::TreeNodeStatus::FINISHED);
}

}
