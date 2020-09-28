/**
 * @file
 * @ingroup Condition
 */
#pragma once

#include "ICondition.h"

namespace backend {

/**
 * @brief This condition will just swap the result of the contained condition
 */
class Not: public ICondition {
protected:
	ConditionPtr _condition;

	void getConditionNameWithValue(core::String& s, const AIPtr& entity) override {
		s += "(";
		s += _condition->getNameWithConditions(entity);
		s += ")";
	}

public:
	CONDITION_FACTORY_NO_IMPL(Not)

	explicit Not(const ConditionPtr& condition) :
			ICondition("Not", ""), _condition(condition) {
	}
	virtual ~Not() {
	}

	bool evaluate(const AIPtr& entity) override {
		return state(!_condition->evaluate(entity));
	}
};

inline ConditionPtr Not::Factory::create(const ConditionFactoryContext *ctx) const {
	if (ctx->conditions.size() != 1u) {
		return ConditionPtr();
	}
	return std::make_shared<Not>(ctx->conditions.front());
}

}
