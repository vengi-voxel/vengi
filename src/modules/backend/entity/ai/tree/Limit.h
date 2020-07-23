/**
 * @file
 */
#pragma once

#include "TreeNode.h"

namespace backend {

/**
 * @brief A decorator node which limits the execution of the attached child to a
 * specified amount of runs.
 *
 * @note If the configured amount is reached, the return @c TreeNodeStatus is
 * @c TreeNodeStatus::FINISHED
 */
class Limit: public TreeNode {
private:
	int _amount;
public:
	NODE_FACTORY(Limit)

	Limit(const core::String& name, const core::String& parameters, const ConditionPtr& condition);

	ai::TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override;
};

}
