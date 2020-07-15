/**
 * @file
 */
#pragma once

#include "TreeNode.h"
#include "../common/Log.h"

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

	Limit(const core::String& name, const core::String& parameters, const ConditionPtr& condition) :
		TreeNode(name, parameters, condition) {
		_type = "Limit";
		if (!parameters.empty()) {
			_amount = parameters.toInt();
		} else {
			_amount = 1;
		}
	}

	ai::TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override {
		if (_children.size() != 1) {
			ai_log_error("Limit must have exactly one node");
			return ai::EXCEPTION;
		}

		if (TreeNode::execute(entity, deltaMillis) == ai::CANNOTEXECUTE) {
			return ai::CANNOTEXECUTE;
		}

		const int alreadyExecuted = getLimitState(entity);
		if (alreadyExecuted >= _amount) {
			return state(entity, ai::FINISHED);
		}

		const TreeNodePtr& treeNode = *_children.begin();
		const ai::TreeNodeStatus status = treeNode->execute(entity, deltaMillis);
		setLimitState(entity, alreadyExecuted + 1);
		if (status == ai::RUNNING) {
			return state(entity, ai::RUNNING);
		}
		return state(entity, ai::FAILED);
	}
};

}
