#pragma once

#include "conditions/ICondition.h"

namespace ai {

/**
 * @brief This condition will logically or all contained conditions
 */
class Or: public ICondition {
protected:
	Conditions _conditions;

	void getConditionNameWithValue(std::stringstream& s, const AIPtr& entity) override;

public:
	Or(const Conditions& conditions) :
			ICondition("Or", ""), _conditions(conditions) {
	}
	virtual ~Or() {
	}

	CONDITION_FACTORY

	bool evaluate(const AIPtr& entity) override;

	std::ostream& print(std::ostream& stream, int level) const override;
};

}
