/**
 * @file
 */

#pragma once

#include "Steering.h"

namespace backend {
namespace movement {

class WanderAroundHome: public movement::ISteering {
private:
	float _maxDistance;
public:
	STEERING_FACTORY(WanderAroundHome)

	explicit WanderAroundHome(const core::String& parameter);

	MoveVector execute (const AIPtr& ai, float speed) const override;
};

}
}
