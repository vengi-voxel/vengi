/**
 * @file
 */

#pragma once

#include "Steering.h"
#include "backend/entity/ai/AICharacter.h"
#include "core/StringUtil.h"

namespace backend {
namespace movement {

class WanderAroundHome: public movement::ISteering {
private:
	float _maxDistance;
public:
	STEERING_FACTORY(WanderAroundHome)

	explicit WanderAroundHome(const core::String& parameter) :
			movement::ISteering() {
		if (parameter.empty()) {
			 _maxDistance = 40.0f;
		} else {
			 _maxDistance = core::string::toFloat(parameter);
		}
	}

	MoveVector execute (const AIPtr& ai, float speed) const override;
};

}
}
