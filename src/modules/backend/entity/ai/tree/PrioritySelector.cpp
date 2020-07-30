/**
 * @file
 */

#include "PrioritySelector.h"
#include "backend/entity/ai/AI.h"

namespace backend {

ai::TreeNodeStatus PrioritySelector::execute(const AIPtr& entity, int64_t deltaMillis) {
	if (Selector::execute(entity, deltaMillis) == ai::TreeNodeStatus::CANNOTEXECUTE)
		return ai::TreeNodeStatus::CANNOTEXECUTE;

	int index = getSelectorState(entity);
	if (index == AI_NOTHING_SELECTED) {
		index = 0;
	}
	const std::size_t size = _children.size();
	ai::TreeNodeStatus overallResult = ai::TreeNodeStatus::FINISHED;
	std::size_t i = index;
	for (std::size_t j = 0; j < i; ++j) {
		_children[j]->resetState(entity);
	}
	for (; i < size; ++i) {
		const TreeNodePtr& child = _children[i];
		const ai::TreeNodeStatus result = child->execute(entity, deltaMillis);
		if (result == ai::TreeNodeStatus::RUNNING) {
			setSelectorState(entity, static_cast<int>(i));
		} else if (result == ai::TreeNodeStatus::CANNOTEXECUTE || result == ai::TreeNodeStatus::FAILED) {
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

}
