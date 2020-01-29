/**
 * @file
 * @ingroup Condition
 */
#pragma once

#include "conditions/ICondition.h"
#include "common/IParser.h"
#include "conditions/Filter.h"
#include "AIRegistry.h"

namespace ai {

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

	void splitConditions(const core::String& string, std::vector<std::string>& tokens) const;
	bool fillInnerConditions(ConditionFactoryContext& ctx, const core::String& inner);
	bool fillInnerFilters(FilterFactoryContext& ctx, const core::String& inner);

public:
	ConditionParser(const IAIFactory& aiFactory, const core::String& conditionString);
	virtual ~ConditionParser();
	ConditionPtr getCondition();
};

}
