/**
 * @file
 */
#pragma once

#include "Steering.h"

namespace ai {
namespace movement {

/**
 * @brief Moves forward in the direction the character is currently facing into.
 *
 * Changes orientation (resp. rotation) in a range of [-rotation,rotation] where more
 * weight is given to keep the current orientation.
 */
class Wander: public ISteering {
protected:
	const float _rotation;
public:
	STEERING_FACTORY(Wander)

	explicit Wander(const core::String& parameter);

	MoveVector execute (const AIPtr& ai, float speed) const override;
};

}
}
