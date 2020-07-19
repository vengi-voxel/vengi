/**
 * @file
 */
#pragma once

#include "Steering.h"

namespace backend {
namespace movement {

/**
 * @brief Seeks a particular target
 */
class TargetSeek: public ISteering {
protected:
	glm::vec3 _target;
	bool _valid;
public:
	STEERING_FACTORY(TargetSeek)

	explicit TargetSeek(const core::String& parameters);

	inline bool isValid () const;

	virtual MoveVector execute (const AIPtr& ai, float speed) const override;
};

}
}
