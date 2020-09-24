/**
 * @file
 * @ingroup Filter
 * @image html complement.png
 */

#include "Complement.h"
#include "core/Algorithm.h"
#include "core/Common.h"
#include "FilteredEntities.h"
#include "FilterUtil.h"

namespace backend {

static size_t complement(size_t maxSize, core::Array<FilteredEntities, 2>& filteredArray) {
	FilteredEntities intersection(maxSize);
	int outSize = 0;
	core::sortedDifference(filteredArray[0].data(), filteredArray[0].size(), filteredArray[1].data(), filteredArray[1].size(), intersection.data(), intersection.capacity(), outSize);
	filteredArray[0].clear();
	filteredArray[0].append(intersection.data(), outSize);
	return outSize;
}

void Complement::filter (const AIPtr& entity) {
	FilteredEntities& filtered = getFilteredEntities(entity);

	FilterState state;
	state.add(filtered, nullptr);

	filtered.clear();

	for (auto& f : _filters) {
		f->filter(entity);
		if (filtered.empty()) {
			continue;
		}

		state.add(filtered, complement);
	}
	core_assert(state.n == 1);
	core_assert(filtered.empty());
	filtered.append(state.filteredArray[0].data(), state.filteredArray[0].size());
}

}
