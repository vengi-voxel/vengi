/**
 * @file
 */

#include "Invert.h"
#include "core/Log.h"

namespace backend {

ai::TreeNodeStatus Invert::execute(const AIPtr& entity, int64_t deltaMillis) {
	if (_children.size() != 1) {
		Log::error("Invert must have exactly one child");
		return ai::TreeNodeStatus::EXCEPTION;
	}

	if (TreeNode::execute(entity, deltaMillis) == ai::TreeNodeStatus::CANNOTEXECUTE) {
		return ai::TreeNodeStatus::CANNOTEXECUTE;
	}

	const TreeNodePtr& treeNode = *_children.begin();
	const ai::TreeNodeStatus status = treeNode->execute(entity, deltaMillis);
	if (status == ai::TreeNodeStatus::FINISHED) {
		return state(entity, ai::TreeNodeStatus::FAILED);
	} else if (status == ai::TreeNodeStatus::FAILED) {
		return state(entity, ai::TreeNodeStatus::FINISHED);
	} else if (status == ai::TreeNodeStatus::EXCEPTION) {
		return state(entity, ai::TreeNodeStatus::EXCEPTION);
	} else if (status == ai::TreeNodeStatus::CANNOTEXECUTE) {
		return state(entity, ai::TreeNodeStatus::FINISHED);
	}
	return state(entity, ai::TreeNodeStatus::RUNNING);
}

}
