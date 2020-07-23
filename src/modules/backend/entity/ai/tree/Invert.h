/**
 * @file
 */
#pragma once

#include "TreeNode.h"

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

	ai::TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override;
};

}
