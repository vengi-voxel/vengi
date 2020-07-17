/**
 * @file
 */

#pragma once

#include "backend/entity/ai/condition/ICondition.h"

namespace backend {

/**
 * @ingroup AI
 */
class IsCloseToSelection: public ICondition {
protected:
	int _distance;

public:
	IsCloseToSelection(const core::String& parameters);
	CONDITION_FACTORY(IsCloseToSelection)

	bool evaluate(const AIPtr& entity) override;
};

}
