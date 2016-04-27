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
	STEERING_FACTORY

	GroupSeek(const std::string& parameters) :
			ISteering() {
		_groupId = ::atoi(parameters.c_str());
	}

	inline bool isValid () const {
		return _groupId != -1;
	}

	virtual MoveVector execute (const AIPtr& ai, float speed) const override {
		const glm::vec3& target = ai->getGroupPosition(_groupId);
		if (isInfinite(target)) {
			return MoveVector(target, 0.0f);
		}
		glm::vec3 v = target - ai->getCharacter()->getPosition();
		double orientation = 0.0;
		if (glm::length2(v) > 0.0f) {
			orientation = angle(glm::normalize(v) * speed);
		}
		const MoveVector d(v, orientation);
		return d;
	}

	std::ostream& print(std::ostream& stream, int level) const override {
		for (int i = 0; i < level; ++i) {
			stream << '\t';
		}
		stream << "GroupSeek(" << _groupId << ")";
		return stream;
	}
};

}
}
