#pragma once

#include <string>

namespace ai {

class LUACondition {
private:
	const ConditionPtr& _condition;
public:
	LUACondition(const ConditionPtr& condition) :
			_condition(condition) {
	}

	inline const ConditionPtr& getCondition() const {
		return _condition;
	}
};

}
