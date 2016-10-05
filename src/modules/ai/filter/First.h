/**
 * @file
 * @ingroup Filter
 */
#pragma once

#include "filter/IFilter.h"

namespace ai {

/**
 * @brief This filter will just preserve the first entry of other filters
 */
class First: public IFilter {
protected:
	Filters _filters;
public:
	First(const std::string& parameters, const Filters& filters) :
		IFilter("First", parameters), _filters(filters) {
		ai_assert(filters.size() == 1, "First must have one child");
	}
	FILTER_ACTION_FACTORY(First)

	void filter (const AIPtr& entity) override;
};

inline void First::filter (const AIPtr& entity) {
	FilteredEntities& filtered = getFilteredEntities(entity);
	const FilteredEntities copy = filtered;
	filtered.clear();
	_filters.front()->filter(entity);
	const auto& value = getFilteredEntities(entity).front();
	filtered.clear();
	for (auto& e : copy) {
		filtered.push_back(e);
	}
	filtered.push_back(value);
}

}
