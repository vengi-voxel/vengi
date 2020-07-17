/**
 * @file
 */

#pragma once

#include "ICondition.h"

namespace backend {

/**
 * @ingroup AI
 */
class IsSelectionAlive: public ICondition {
public:
	CONDITION_CLASS(IsSelectionAlive)
	CONDITION_FACTORY(IsSelectionAlive)

	bool evaluate(const AIPtr& entity) override;
};

}
