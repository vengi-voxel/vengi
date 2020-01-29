/**
 * @file
 */

#pragma once

#include "backend/entity/ai/AICharacter.h"
#include "core/Common.h"

namespace backend {

class WanderAroundHome: public ai::movement::ISteering {
private:
	float _maxDistance;
public:
	STEERING_FACTORY(WanderAroundHome)

	explicit WanderAroundHome(const core::String& parameter) :
			ai::movement::ISteering() {
		if (parameter.empty()) {
			 _maxDistance = 40.0f;
		} else {
			 _maxDistance = std::stof(parameter);
		}
	}

	ai::MoveVector execute (const ai::AIPtr& ai, float speed) const override;
};


}
