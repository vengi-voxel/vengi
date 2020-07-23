/**
 * @file
 */
#pragma once

#include "TreeNode.h"

namespace backend {

/**
 * @brief A decorator node with only one child attached. The result of the attached child is only
 * taken into account if it returned @c TreeNodeStatus::RUNNING - in every other case this decorator
 * will return TreeNodeStatus::FINISHED.
 */
class Succeed: public TreeNode {
public:
	NODE_CLASS(Succeed)

	ai::TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override;
};

}
