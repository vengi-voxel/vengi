/**
 * @file
 * @ingroup Condition
 */
#pragma once

#include "ICondition.h"

namespace backend {

/**
 * @brief This condition just always evaluates to @c true
 */
class True: public ICondition {
public:
	CONDITION_CLASS_SINGLETON(True)

	bool evaluate(const AIPtr& entity) override;
};

inline bool True::evaluate(const AIPtr& /* entity */) {
	return true;
}

}
