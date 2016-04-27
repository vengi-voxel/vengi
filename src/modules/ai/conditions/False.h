#pragma once

#include "conditions/ICondition.h"

namespace ai {

/**
 * @brief This condition just always evaluates to @c false
 */
class False: public ICondition {
public:
	CONDITION_CLASS_SINGLETON(False)

	bool evaluate(const AIPtr& entity) override;

	std::ostream& print(std::ostream& stream, int level) const override;
};

inline bool False::evaluate(const AIPtr& /*entity*/) {
	return false;
}

}
