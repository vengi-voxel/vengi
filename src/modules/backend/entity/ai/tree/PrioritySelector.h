/**
 * @file
 */
#pragma once

#include "Selector.h"
#include "../AI.h"

namespace backend {

/**
 * @brief This node tries to execute all the attached children until one succeeds. This composite only
 * fails if all children failed, too.
 *
 * http://aigamedev.com/open/article/selector/
 */
class PrioritySelector: public Selector {
public:
	SELECTOR_CLASS(PrioritySelector)

	ai::TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override {
		if (Selector::execute(entity, deltaMillis) == ai::CANNOTEXECUTE)
			return ai::CANNOTEXECUTE;

		int index = getSelectorState(entity);
		if (index == AI_NOTHING_SELECTED) {
			index = 0;
		}
		const std::size_t size = _children.size();
		ai::TreeNodeStatus overallResult = ai::FINISHED;
		std::size_t i = index;
		for (std::size_t j = 0; j < i; ++j) {
			_children[j]->resetState(entity);
		}
		for (; i < size; ++i) {
			const TreeNodePtr& child = _children[i];
			const ai::TreeNodeStatus result = child->execute(entity, deltaMillis);
			if (result == ai::RUNNING) {
				setSelectorState(entity, static_cast<int>(i));
			} else if (result == ai::CANNOTEXECUTE || result == ai::FAILED) {
				child->resetState(entity);
				setSelectorState(entity, AI_NOTHING_SELECTED);
				continue;
			} else {
				setSelectorState(entity, AI_NOTHING_SELECTED);
			}
			child->resetState(entity);
			overallResult = result;
			break;
		}
		for (++i; i < size; ++i) {
			_children[i]->resetState(entity);
		}
		return state(entity, overallResult);
	}
};

}
