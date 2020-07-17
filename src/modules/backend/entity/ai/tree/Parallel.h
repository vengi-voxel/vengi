/**
 * @file
 */
#pragma once

#include "Selector.h"

namespace backend {

/**
 * @brief Executes all the connected children in the order they were added (no matter what
 * the TreeNodeStatus of the previous child was).
 *
 * http://aigamedev.com/open/article/parallel/
 */
class Parallel: public Selector {
public:
	SELECTOR_CLASS(Parallel)

	void getRunningChildren(const AIPtr& entity, std::vector<bool>& active) const override;
	/**
	 * @brief If one of the children was executed, and is still running, the ::TreeNodeStatus::RUNNING
	 * is returned, otherwise ::TreeNodeStatus::FINISHED is returned.
	 */
	ai::TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override;
};

}
