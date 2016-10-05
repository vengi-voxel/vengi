/**
 * @file
 * @ingroup Filter
 */
#pragma once

#include "filter/IFilter.h"
#include "common/Random.h"

namespace ai {

/**
 * @brief This filter will preserve only a few random entries
 */
class Random: public IFilter {
protected:
	Filters _filters;
	int _n;
public:
	Random(const std::string& parameters, const Filters& filters) :
		IFilter("Random", parameters), _filters(filters) {
		ai_assert(filters.size() == 1, "Random must have one child");
		_n = std::stoi(parameters);
	}

	FILTER_ACTION_FACTORY(Random)

	void filter (const AIPtr& entity) override;
};

inline void Random::filter (const AIPtr& entity) {
	FilteredEntities& filtered = getFilteredEntities(entity);
	const FilteredEntities copy = filtered;
	filtered.clear();
	_filters.front()->filter(entity);
	FilteredEntities rndFiltered = getFilteredEntities(entity);
	ai::shuffle(rndFiltered.begin(), rndFiltered.end());
	rndFiltered.resize(_n);
	for (auto& e : copy) {
		filtered.push_back(e);
	}
	for (auto& e : rndFiltered) {
		filtered.push_back(e);
	}
}

}
