/**
 * @file
 * @ingroup Filter
 */

#include "First.h"

namespace backend {

void First::filter (const AIPtr& entity) {
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
