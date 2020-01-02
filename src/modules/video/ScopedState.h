/**
 * @file
 */

#pragma once

#include "Types.h"

namespace video {

/**
 * @brief Enables or disables a given state and restore the previous state once the scope is left.
 *
 * @ingroup Video
 */
class ScopedState {
private:
	const video::State _state;
	bool _old;
	const bool _enable;
public:
	ScopedState(video::State state, bool enable = true);
	~ScopedState();
};

}
