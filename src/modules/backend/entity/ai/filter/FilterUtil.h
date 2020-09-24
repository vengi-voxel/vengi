/**
 * @file
 */

#include "core/Algorithm.h"
#include "core/Common.h"
#include "core/collection/Array.h"
#include "FilteredEntities.h"

namespace backend {

struct FilterState {
	core::Array<FilteredEntities, 2> filteredArray;
	int n = 0;
	size_t max = 0u;

	void add(FilteredEntities& filtered, size_t(*action)(size_t max, core::Array<FilteredEntities, 2>&)) {
		filteredArray[n] = filtered;
		core::sort(filteredArray[n].begin(), filteredArray[n].end(), core::Less<ai::CharacterId>());
		max = core_max(filteredArray[n].size(), max);
		++n;
		filtered.clear();

		if (n >= 2) {
			max = action(max, filteredArray);
			n = 1;
		}
	}
};

}
