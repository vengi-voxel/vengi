/**
 * @file
 */
#pragma once

#include "TreeNode.h"
#include "../common/Log.h"

namespace backend {

/**
 * @brief A node with only one child attached. The result of the attached child is inverted.
 *
 * - If the child returns a TreeNodeStatus::FINISHED, this node will return TreeNodeStatus::FAILED
 * - If the child returns a TreeNodeStatus::FAILED, this node will return TreeNodeStatus::FINISHED
 * - otherwise this node will return a TreeNodeStatus::RUNNING
 */
class Invert: public TreeNode {
public:
	NODE_CLASS(Invert)

	ai::TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override {
		if (_children.size() != 1) {
			ai_log_error("Invert must have exactly one child");
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
};

}
