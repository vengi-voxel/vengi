#include "tree/RandomSelector.h"
#include "AI.h"
#include <algorithm>

namespace ai {

TreeNodeStatus RandomSelector::execute(const AIPtr& entity, int64_t deltaMillis) {
	if (Selector::execute(entity, deltaMillis) == CANNOTEXECUTE)
		return CANNOTEXECUTE;

	TreeNodes childrenShuffled = _children;
	const std::size_t size = childrenShuffled.size();
	std::random_shuffle(childrenShuffled.begin(), childrenShuffled.end());
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

NODE_FACTORY_IMPL(RandomSelector)

}
