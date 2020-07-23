/**
 * @file
 */
#pragma once

#include "Steering.h"

namespace backend {
namespace movement {

/**
 * @brief Flees the current @c IFilter selection from the given @c ICharacter
 */
class SelectionFlee: public SelectionSteering {
public:
	STEERING_FACTORY(SelectionFlee)

	explicit SelectionFlee(const core::String&);

	virtual MoveVector execute (const AIPtr& ai, float speed) const override;
};

}
}
