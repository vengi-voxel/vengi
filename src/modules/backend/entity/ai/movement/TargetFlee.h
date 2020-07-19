/**
 * @file
 */
#pragma once

#include "Steering.h"

namespace backend {
namespace movement {

/**
 * @brief Flees from a particular target
 */
class TargetFlee: public ISteering {
protected:
	glm::vec3 _target;
public:
	STEERING_FACTORY(TargetFlee)

	explicit TargetFlee(const core::String& parameters);

	bool isValid () const;

	virtual MoveVector execute (const AIPtr& ai, float speed) const override;
};


}
}
