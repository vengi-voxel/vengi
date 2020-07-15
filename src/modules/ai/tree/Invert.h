/**
 * @file
 */
#pragma once

#include "tree/TreeNode.h"
#include "ai-shared/common/Log.h"

namespace ai {

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

	TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override {
		if (_children.size() != 1) {
			ai_log_error("Invert must have exactly one child");
			return EXCEPTION;
		}

		if (TreeNode::execute(entity, deltaMillis) == CANNOTEXECUTE) {
			return CANNOTEXECUTE;
		}

		const TreeNodePtr& treeNode = *_children.begin();
		const TreeNodeStatus status = treeNode->execute(entity, deltaMillis);
		if (status == FINISHED) {
			return state(entity, FAILED);
		} else if (status == FAILED) {
			return state(entity, FINISHED);
		} else if (status == EXCEPTION) {
			return state(entity, EXCEPTION);
		} else if (status == CANNOTEXECUTE) {
			return state(entity, FINISHED);
		}
		return state(entity, RUNNING);
	}
};

}
