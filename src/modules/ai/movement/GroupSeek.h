#pragma once

#include "Steering.h"

namespace ai {
namespace movement {

/**
 * @brief Seeks a particular group
 */
class GroupSeek: public ISteering {
protected:
	GroupId _groupId;
public:
	STEERING_FACTORY(GroupSeek)

	explicit GroupSeek(const std::string& parameters) :
			ISteering() {
		_groupId = ::atoi(parameters.c_str());
	}

	inline bool isValid () const {
		return _groupId != -1;
	}

	virtual MoveVector execute (const AIPtr& ai, float speed) const override {
		const Zone* zone = ai->getZone();
		if (zone == nullptr) {
			return MoveVector(INFINITE, 0.0f);
		}
		const glm::vec3& target = zone->getGroupMgr().getPosition(_groupId);
		if (isInfinite(target)) {
			return MoveVector(target, 0.0f);
		}
		const glm::vec3& v = glm::normalize(target - ai->getCharacter()->getPosition());
		const double orientation = angle(v);
		const MoveVector d(v * speed, orientation);
		return d;
	}
};

}
}
