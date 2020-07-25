/**
 * @file
 */

#include "Parallel.h"
#include "backend/entity/ai/AI.h"

namespace backend {

void Parallel::getRunningChildren(const AIPtr& entity, std::vector<bool>& active) const {
	for (const TreeNodePtr& child : _children) {
		active.push_back(child->getLastStatus(entity) != ai::RUNNING);
	}
}
/**
	* @brief If one of the children was executed, and is still running, the ::TreeNodeStatus::RUNNING
	* is returned, otherwise ::TreeNodeStatus::FINISHED is returned.
	*/
ai::TreeNodeStatus Parallel::execute(const AIPtr& entity, int64_t deltaMillis) {
	if (Selector::execute(entity, deltaMillis) == ai::CANNOTEXECUTE) {
		return ai::CANNOTEXECUTE;
	}

	bool totalStatus = false;
	for (const TreeNodePtr& child : _children) {
		const bool isActive = child->execute(entity, deltaMillis) == ai::RUNNING;
		if (!isActive) {
			child->resetState(entity);
		}
		totalStatus |= isActive;
	}

	if (!totalStatus) {
		resetState(entity);
	}
	return state(entity, totalStatus ? ai::RUNNING : ai::FINISHED);
}

}
