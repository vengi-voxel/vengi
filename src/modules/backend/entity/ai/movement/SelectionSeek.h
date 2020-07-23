/**
 * @file
 */
#pragma once

#include "Steering.h"

namespace backend {
namespace movement {

/**
 * @brief Seeks the current @c IFilter selection from the given @c ICharacter
 */
class SelectionSeek: public SelectionSteering {
public:
	STEERING_FACTORY(SelectionSeek)

	explicit SelectionSeek(const core::String&);

	virtual MoveVector execute (const AIPtr& ai, float speed) const override;
};

}
}
