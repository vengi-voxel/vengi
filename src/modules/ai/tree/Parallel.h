/**
 * @file
 */
#pragma once

#include "tree/Selector.h"
#include "AI.h"

namespace ai {

/**
 * @brief Executes all the connected children in the order they were added (no matter what
 * the TreeNodeStatus of the previous child was).
 *
 * http://aigamedev.com/open/article/parallel/
 */
class Parallel: public Selector {
public:
	SELECTOR_CLASS(Parallel)

	void getRunningChildren(const AIPtr& entity, std::vector<bool>& active) const override {
		for (TreeNodes::const_iterator i = _children.begin(); i != _children.end(); ++i) {
			active.push_back((*i)->getLastStatus(entity) != RUNNING);
		}
	}
	/**
	 * @brief If one of the children was executed, and is still running, the ::TreeNodeStatus::RUNNING
	 * is returned, otherwise ::TreeNodeStatus::FINISHED is returned.
	 */
	TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override {
		if (Selector::execute(entity, deltaMillis) == CANNOTEXECUTE) {
			return CANNOTEXECUTE;
		}

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
};

}
