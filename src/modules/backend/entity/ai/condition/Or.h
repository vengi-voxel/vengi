/**
 * @file
 * @ingroup Condition
 */
#pragma once

#include "ICondition.h"

namespace backend {

/**
 * @brief This condition will logically or all contained conditions
 */
class Or: public ICondition {
protected:
	Conditions _conditions;
	CONDITION_PRINT_SUBCONDITIONS_GETCONDITIONNAMEWITHVALUE

public:
	explicit Or(const Conditions& conditions) :
			ICondition("Or", ""), _conditions(conditions) {
	}
	virtual ~Or() {
	}

	CONDITION_FACTORY_NO_IMPL(Or)

	bool evaluate(const AIPtr& entity) override {
		for (ConditionsIter i = _conditions.begin(); i != _conditions.end(); ++i) {
			if ((*i)->evaluate(entity)) {
				return true;
			}
		}

		return false;
	}
};

inline ConditionPtr Or::Factory::create(const ConditionFactoryContext *ctx) const {
	if (ctx->conditions.size() < 2) {
		return ConditionPtr();
	}
	return std::make_shared<Or>(ctx->conditions);
}

}
