#pragma once

#include "conditions/ICondition.h"

namespace ai {

/**
 * @brief This condition will just swap the result of the contained condition
 */
class Not: public ICondition {
protected:
	ConditionPtr _condition;

	void getConditionNameWithValue(std::stringstream& s, const AIPtr& entity) override {
		s << "(" << _condition->getNameWithConditions(entity) << ")";
	}

public:
	Not(const ConditionPtr& condition) :
			ICondition("Not", ""), _condition(condition) {
	}
	virtual ~Not() {
	}

	CONDITION_FACTORY

	bool evaluate(const AIPtr& entity) override;

	std::ostream& print(std::ostream& stream, int level) const override;
};

inline bool Not::evaluate(const AIPtr& entity) {
	return !_condition->evaluate(entity);
}

}
