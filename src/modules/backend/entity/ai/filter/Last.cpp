/**
 * @file
 * @ingroup Filter
 */

#include "Last.h"

namespace backend {

void Last::filter (const AIPtr& entity) {
	// TODO: could be optimized by running them in reverse and check if there is a result
	for (auto& f : _filters) {
		f->filter(entity);
	}
	FilteredEntities& filtered = getFilteredEntities(entity);
	if (filtered.empty()) {
		return;
	}
	const ai::CharacterId id = filtered[filtered.size() - 1];
	filtered.clear();
	filtered.push_back(id);
}

}
