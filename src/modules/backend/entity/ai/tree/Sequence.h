/**
 * @file
 */
#pragma once

#include "Selector.h"

namespace backend {

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

	ai::TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override;

	void resetState(const AIPtr& entity) override;
};

}
