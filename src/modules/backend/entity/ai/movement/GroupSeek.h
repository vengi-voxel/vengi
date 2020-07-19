/**
 * @file
 */
#pragma once

#include "Steering.h"
#include "backend/entity/ai/group/GroupId.h"

namespace backend {
namespace movement {

/**
 * @brief Seeks a particular group
 */
class GroupSeek: public ISteering {
protected:
	GroupId _groupId;
public:
	STEERING_FACTORY(GroupSeek)

	explicit GroupSeek(const core::String& parameters);

	inline bool isValid () const {
		return _groupId != -1;
	}

	virtual MoveVector execute (const AIPtr& ai, float speed) const override;
};

}
}
