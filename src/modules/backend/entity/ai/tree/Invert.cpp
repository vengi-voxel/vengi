/**
 * @file
 */

#include "Invert.h"
#include "core/Log.h"

namespace backend {

ai::TreeNodeStatus Invert::execute(const AIPtr& entity, int64_t deltaMillis) {
	if (_children.size() != 1) {
		Log::error("Invert must have exactly one child");
		return ai::EXCEPTION;
	}

	if (TreeNode::execute(entity, deltaMillis) == ai::CANNOTEXECUTE) {
		return ai::CANNOTEXECUTE;
	}

	const TreeNodePtr& treeNode = *_children.begin();
	const ai::TreeNodeStatus status = treeNode->execute(entity, deltaMillis);
	if (status == ai::FINISHED) {
		return state(entity, ai::FAILED);
	} else if (status == ai::FAILED) {
		return state(entity, ai::FINISHED);
	} else if (status == ai::EXCEPTION) {
		return state(entity, ai::EXCEPTION);
	} else if (status == ai::CANNOTEXECUTE) {
		return state(entity, ai::FINISHED);
	}
	return state(entity, ai::RUNNING);
}

}
