/**
 * @file
 */
#pragma once

#include "tree/ITimedNode.h"

namespace ai {

/**
 * @brief @c ITimedNode that is just idling until the given time is elapsed.
 */
class Idle: public ai::ITimedNode {
public:
	TIMERNODE_CLASS(Idle)
};

}
