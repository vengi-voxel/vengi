/**
 * @file
 */
#pragma once

#include "ITimedNode.h"

namespace backend {

/**
 * @brief @c ITimedNode that is just idling until the given time is elapsed.
 */
class Idle: public ITimedNode {
public:
	TIMERNODE_CLASS(Idle)
};

}
