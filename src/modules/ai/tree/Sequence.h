/**
 * @file
 */
#pragma once

#include "tree/Selector.h"
#include "AI.h"

namespace ai {

/**
 * @brief The sequence continues to execute their children until one of the children
 * returned a state that is not equal to finished. On the next iteration the execution
 * is continued at the last running children or from the start again if no such
 * children exists.
 *
 * [AiGameDev](http://aigamedev.com/open/article/sequence/)
 */
class Sequence: public Selector {
public:
	SELECTOR_CLASS(Sequence)

	TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override {
		if (Selector::execute(entity, deltaMillis) == CANNOTEXECUTE)
			return CANNOTEXECUTE;

		TreeNodeStatus result = FINISHED;
		const int progress = core_max(0, getSelectorState(entity));

		const std::size_t size = _children.size();
		for (std::size_t i = static_cast<std::size_t>(progress); i < size; ++i) {
			TreeNodePtr& child = _children[i];

			result = child->execute(entity, deltaMillis);

			if (result == RUNNING) {
				setSelectorState(entity, static_cast<int>(i));
				break;
			} else if (result == CANNOTEXECUTE || result == FAILED) {
				resetState(entity);
				break;
			} else if (result == EXCEPTION) {
				break;
			}
		}

		if (result != RUNNING) {
			resetState(entity);
		}

		return state(entity, result);
	}

	void resetState(const AIPtr& entity) override {
		setSelectorState(entity, AI_NOTHING_SELECTED);
		TreeNode::resetState(entity);
	}
};

}
