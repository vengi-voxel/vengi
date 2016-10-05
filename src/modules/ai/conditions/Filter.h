/**
 * @file
 */
#pragma once

#include "conditions/ICondition.h"
#include "filter/IFilter.h"

#define FILTER_NAME "Filter"

namespace ai {

/**
 * @brief The filter condition executes some selection filters (@c IFilter) and evaluates to @c true if
 * the resulting set of all filter executions is non-empty. Use @c AI::getFilteredEntities to access the
 * result set and work with it in a TreeNode that is executed when this condition evaluated to true
 */
class Filter: public ICondition {
protected:
	Filters _filters;

	void getConditionNameWithValue(std::stringstream& s, const AIPtr& entity) override {
		bool first = true;
		s << "(";
		auto copy = entity->_filteredEntities;
		for (const FilterPtr& filter : _filters) {
			if (!first)
				s << ",";
			s << filter->getName() << "{" << filter->getParameters() << "}[";
			entity->_filteredEntities.clear();
			filter->filter(entity);
			bool firstChr = true;
			int cnt = 0;
			for (CharacterId id : entity->_filteredEntities) {
				if (!firstChr)
					s << ",";
				s << id;
				firstChr = false;
				++cnt;
				if (cnt > 15) {
					s << ",...";
					break;
				}
			}
			s << "]";
			first = false;
		}
		entity->_filteredEntities = copy;
		s << ")";
	}

public:
	CONDITION_FACTORY_NO_IMPL(Filter)

	explicit Filter (const Filters& filters) :
			ICondition(FILTER_NAME, ""), _filters(filters) {
	}

	/**
	 * @brief Executes the attached filters and wiped the last filter results for the given @c AI entity
	 *
	 * @return @c true if the attached filters lead to a non-empty set, @c false otherwise
	 */
	bool evaluate(const AIPtr& entity) override {
		entity->_filteredEntities.clear();
		for (const FilterPtr filter : _filters) {
			filter->filter(entity);
		}
		return !entity->_filteredEntities.empty();
	}
};

inline ConditionPtr Filter::Factory::create(const ConditionFactoryContext *ctx) const {
	return std::make_shared<Filter>(ctx->filters);
}

}
