/**
 * @file
 * @ingroup Condition
 */
#pragma once

#include "ICondition.h"

namespace backend {

/**
 * @brief This condition will logically and all contained conditions
 */
class And: public ICondition {
protected:
	Conditions _conditions;
	CONDITION_PRINT_SUBCONDITIONS_GETCONDITIONNAMEWITHVALUE

public:
	explicit And(const Conditions& conditions) :
			ICondition("And", ""), _conditions(conditions) {
	}
	virtual ~And() {
	}

	CONDITION_FACTORY_NO_IMPL(And)

	bool evaluate(const AIPtr& entity) override {
		for (ConditionsIter i = _conditions.begin(); i != _conditions.end(); ++i) {
			if (!(*i)->evaluate(entity)) {
				return false;
			}
		}

		return true;
	}
};

inline ConditionPtr And::Factory::create(const ConditionFactoryContext *ctx) const {
	if (ctx->conditions.size() < 2) {
		return ConditionPtr();
	}
	return std::make_shared<And>(ctx->conditions);
}

}
