/**
 * @file
 * @ingroup Filter
 */

#include "Random.h"
#include "backend/entity/ai/common/Random.h"
#include "core/StringUtil.h"

namespace backend {

Random::Random(const core::String& parameters, const Filters& filters) :
	IFilter("Random", parameters), _filters(filters) {
	_n = core::string::toInt(parameters);
}

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
