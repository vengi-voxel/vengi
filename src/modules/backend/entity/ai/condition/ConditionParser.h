/**
 * @file
 * @ingroup Condition
 */
#pragma once

#include "ICondition.h"
#include "backend/entity/ai/common/IParser.h"
#include "Filter.h"
#include "backend/entity/ai/AIRegistry.h"

namespace backend {

class IAIFactory;

/**
 * @brief Transforms the string representation of a condition with all its sub conditions and
 * parameters into a @c ICondition instance.
 *
 * @c #ConditionName{Parameters}(#SubCondition{SubConditionParameters},...)
 * Parameters and subconditions are both optional.
 */
class ConditionParser : public IParser {
private:
	const IAIFactory& _aiFactory;
	core::String _conditionString;

	void splitConditions(const core::String& string, std::vector<core::String>& tokens) const;
	bool fillInnerConditions(ConditionFactoryContext& ctx, const core::String& inner);
	bool fillInnerFilters(FilterFactoryContext& ctx, const core::String& inner);

public:
	ConditionParser(const IAIFactory& aiFactory, const core::String& conditionString);
	virtual ~ConditionParser();
	ConditionPtr getCondition();
};

}
