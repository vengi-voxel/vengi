/**
 * @file
 * @ingroup Condition
 * @ingroup Filter
 */
#pragma once

#include "ICondition.h"
#include "backend/entity/ai/filter/IFilter.h"

#define FILTER_NAME "Filter"

namespace backend {

/**
 * @brief The filter condition executes some selection filters (@c IFilter) and evaluates to @c true if
 * the resulting set of all filter executions is non-empty. Use @c AI::getFilteredEntities to access the
 * result set and work with it in a TreeNode that is executed when this condition evaluated to true
 */
class Filter: public ICondition {
protected:
	Filters _filters;

	void getConditionNameWithValue(core::String& s, const AIPtr& entity) override;

public:
	CONDITION_FACTORY_NO_IMPL(Filter)

	explicit Filter (const Filters& filters);

	/**
	 * @brief Executes the attached filters and wiped the last filter results for the given @c AI entity
	 *
	 * @return @c true if the attached filters lead to a non-empty set, @c false otherwise
	 */
	bool evaluate(const AIPtr& entity) override;
};

inline ConditionPtr Filter::Factory::create(const ConditionFactoryContext *ctx) const {
	return std::make_shared<Filter>(ctx->filters);
}

}
