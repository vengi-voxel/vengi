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

	explicit GroupFlee(const std::string& parameters);

	inline bool isValid () const {
		return _groupId != -1;
	}

	virtual MoveVector execute (const AIPtr& ai, float speed) const override;
};

}
}
