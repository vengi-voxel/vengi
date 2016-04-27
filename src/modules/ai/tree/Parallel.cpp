#include "tree/Parallel.h"
#include "AI.h"

namespace ai {

void Parallel::getRunningChildren(const AIPtr& entity, std::vector<bool>& active) const {
	for (TreeNodes::const_iterator i = _children.begin(); i != _children.end(); ++i) {
		active.push_back((*i)->getLastStatus(entity) != RUNNING);
	}
}

TreeNodeStatus Parallel::execute(const AIPtr& entity, int64_t deltaMillis) {
	if (Selector::execute(entity, deltaMillis) == CANNOTEXECUTE)
		return CANNOTEXECUTE;

	bool totalStatus = false;
	for (const TreeNodePtr& child : _children) {
		const bool isActive = child->execute(entity, deltaMillis) == RUNNING;
		if (!isActive) {
			child->resetState(entity);
		}
		totalStatus |= isActive;
	}

	if (!totalStatus) {
		resetState(entity);
	}
	return state(entity, totalStatus ? RUNNING : FINISHED);
}

NODE_FACTORY_IMPL(Parallel)

}
