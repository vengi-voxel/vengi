/**
 * @file
 */
#pragma once

#include "Steering.h"

namespace ai {
namespace movement {

/**
 * @brief Flees from a particular group
 */
class GroupFlee: public ISteering {
protected:
	GroupId _groupId;
public:
	STEERING_FACTORY(GroupFlee)

	explicit GroupFlee(const std::string& parameters) :
			ISteering() {
		_groupId = ::atoi(parameters.c_str());
	}

	inline bool isValid () const {
		return _groupId != -1;
	}

	virtual MoveVector execute (const AIPtr& ai, float speed) const override {
		const Zone* zone = ai->getZone();
		if (zone == nullptr) {
			return MoveVector(VEC3_INFINITE, 0.0f);
		}
		const glm::vec3& target = zone->getGroupMgr().getPosition(_groupId);
		if (isInfinite(target)) {
			return MoveVector(target, 0.0f);
		}
		const glm::vec3& v = glm::normalize(ai->getCharacter()->getPosition() - target);
		const float orientation = angle(v);
		const MoveVector d(v * speed, orientation);
		return d;
	}
};

}
}
