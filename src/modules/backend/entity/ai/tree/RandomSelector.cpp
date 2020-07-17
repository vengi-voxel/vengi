/**
 * @file
 */

#include "RandomSelector.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/common/Random.h"

namespace backend {
ai::TreeNodeStatus RandomSelector::execute(const AIPtr& entity, int64_t deltaMillis) {
	if (Selector::execute(entity, deltaMillis) == ai::CANNOTEXECUTE) {
		return ai::CANNOTEXECUTE;
	}

	TreeNodes childrenShuffled = _children;
	const std::size_t size = childrenShuffled.size();
	shuffle(childrenShuffled.begin(), childrenShuffled.end());
	ai::TreeNodeStatus overallResult = ai::FINISHED;
	std::size_t i;
	for (i = 0; i < size; ++i) {
		const TreeNodePtr& child = childrenShuffled[i];
		const ai::TreeNodeStatus result = child->execute(entity, deltaMillis);
		if (result == ai::RUNNING) {
			continue;
		} else if (result == ai::CANNOTEXECUTE || result == ai::FAILED) {
			overallResult = result;
		}
		child->resetState(entity);
	}
	for (++i; i < size; ++i) {
		childrenShuffled[i]->resetState(entity);
	}
	return state(entity, overallResult);
}

}
