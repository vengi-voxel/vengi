/**
 * @file
 * @ingroup Filter
 * @image html union.png
 */

#include "Union.h"
#include "core/Common.h"
#include <algorithm>
#include <vector>
#include <iterator>

namespace backend {

void Union::filter (const AIPtr& entity) {
	FilteredEntities& filtered = getFilteredEntities(entity);
	// create a copy
	const FilteredEntities alreadyFiltered = filtered;
	// now clear the entity list
	filtered.clear();

	std::vector<FilteredEntities> filteredArray(_filters.size());
	int n = 0;
	size_t max = 0u;
	for (auto& f : _filters) {
		f->filter(entity);
		filteredArray[n++] = filtered;
		max = core_max(filtered.size(), max);
		// safe and clear
		filtered.clear();
	}

	for (size_t i = 0; i < filteredArray.size(); ++i) {
		std::sort(filteredArray[i].begin(), filteredArray[i].end());
	}

	FilteredEntities result(max);
	std::set_union(
			filteredArray[0].begin(), filteredArray[0].end(),
			filteredArray[1].begin(), filteredArray[1].end(),
			std::back_inserter(result));

	if (filteredArray.size() >= 2u) {
		FilteredEntities buffer(max);
		for (size_t i = 2; i < filteredArray.size(); ++i) {
			buffer.clear();
			std::sort(result.begin(), result.end());
			std::set_union(
					result.begin(), result.end(),
					filteredArray[i].begin(), filteredArray[i].end(),
					std::back_inserter(buffer));
			core::exchange(result, buffer);
		}
	}

	filtered.reserve(alreadyFiltered.size() + max);
	for (auto& e : alreadyFiltered) {
		filtered.push_back(e);
	}
	for (auto& e : result) {
		filtered.push_back(e);
	}
}

}
