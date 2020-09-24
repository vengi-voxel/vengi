/**
 * @file
 * @ingroup Filter
 * @image html difference.png
 */

#include "Difference.h"
#include "core/Algorithm.h"
#include "core/Common.h"
#include "FilteredEntities.h"
#include "FilterUtil.h"

namespace backend {

static size_t difference(size_t maxSize, core::Array<FilteredEntities, 2>& filteredArray) {
	FilteredEntities intersection(maxSize);
	int outSize = 0;
	core::sortedDifference(filteredArray[0].data(), filteredArray[0].size(), filteredArray[1].data(), filteredArray[1].size(), intersection.data(), intersection.capacity(), outSize);
	filteredArray[0].clear();
	filteredArray[0].append(intersection.data(), outSize);
	return outSize;
}

void Difference::filter (const AIPtr& entity) {
	FilteredEntities& filtered = getFilteredEntities(entity);
	// create a copy
	const FilteredEntities alreadyFiltered = filtered;
	// now clear the entity list
	filtered.clear();

	FilterState state;
	for (auto& f : _filters) {
		f->filter(entity);
		if (filtered.empty()) {
			continue;
		}
		state.add(filtered, difference);
	}
	core_assert(state.n <= 1);
	if (state.n < 1) {
		// restore original state
		filtered.append(alreadyFiltered.data(), alreadyFiltered.size());
		return;
	}

	core_assert(filtered.empty());
	filtered.reserve(alreadyFiltered.size() + state.filteredArray[0].size());
	filtered.append(alreadyFiltered.data(), alreadyFiltered.size());
	filtered.append(state.filteredArray[0].data(), state.filteredArray[0].size());
}

}
