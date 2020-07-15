/**
 * @file
 * @ingroup Filter
 */

#include "Random.h"
#include "backend/entity/ai/common/Random.h"

namespace backend {

void Random::filter (const AIPtr& entity) {
	FilteredEntities& filtered = getFilteredEntities(entity);
	const FilteredEntities copy = filtered;
	filtered.clear();
	_filters.front()->filter(entity);
	FilteredEntities rndFiltered = getFilteredEntities(entity);
	shuffle(rndFiltered.begin(), rndFiltered.end());
	rndFiltered.resize(_n);
	for (auto& e : copy) {
		filtered.push_back(e);
	}
	for (auto& e : rndFiltered) {
		filtered.push_back(e);
	}
}

}
