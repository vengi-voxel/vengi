/**
 * @file
 */
#pragma once

#include "TreeNode.h"
#include "backend/entity/ai/common/Log.h"

namespace backend {

/**
 * @brief A decorator node with only one child attached. The result of the attached child is only
 * taken into account if it returned @c TreeNodeStatus::RUNNING - in every other case this decorator
 * will return @c TreeNodeStatus::FAILED.
 */
class Fail: public TreeNode {
public:
	NODE_CLASS(Fail)

	ai::TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override {
		if (_children.size() != 1) {
			ai_log_error("Fail must have exactly one child");
			return ai::EXCEPTION;
		}

		if (TreeNode::execute(entity, deltaMillis) == ai::CANNOTEXECUTE) {
			return ai::CANNOTEXECUTE;
		}

		const TreeNodePtr& treeNode = *_children.begin();
		const ai::TreeNodeStatus status = treeNode->execute(entity, deltaMillis);
		if (status == ai::RUNNING) {
			return state(entity, ai::RUNNING);
		}
		return state(entity, ai::FAILED);
	}
};

}
