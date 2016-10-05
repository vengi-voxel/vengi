/**
 * @file
 */
#pragma once

#include "tree/Selector.h"
#include "AI.h"

namespace ai {

/**
 * @brief This node executes all the attached children in random order. This composite only
 * fails if all children failed, too. It doesn't continue a node in the state
 * @c TreeNodeStatus::RUNNING. It will always pick a new random node in each tick.
 *
 * http://aigamedev.com/open/article/selector/
 */
class RandomSelector: public Selector {
public:
	SELECTOR_CLASS(RandomSelector)

	TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override {
		if (Selector::execute(entity, deltaMillis) == CANNOTEXECUTE) {
			return CANNOTEXECUTE;
		}

		TreeNodes childrenShuffled = _children;
		const std::size_t size = childrenShuffled.size();
		ai::shuffle(childrenShuffled.begin(), childrenShuffled.end());
		TreeNodeStatus overallResult = FINISHED;
		std::size_t i;
		for (i = 0; i < size; ++i) {
			const TreeNodePtr& child = childrenShuffled[i];
			const TreeNodeStatus result = child->execute(entity, deltaMillis);
			if (result == RUNNING) {
				continue;
			} else if (result == CANNOTEXECUTE || result == FAILED) {
				overallResult = result;
			}
			child->resetState(entity);
		}
		for (++i; i < size; ++i) {
			childrenShuffled[i]->resetState(entity);
		}
		return state(entity, overallResult);
	}
};

}
