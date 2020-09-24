/**
 * @file
 * @ingroup Filter
 */

#include "First.h"

namespace backend {

void First::filter (const AIPtr& entity) {
	FilteredEntities& filtered = getFilteredEntities(entity);
	for (auto& f : _filters) {
		f->filter(entity);
		if (!filtered.empty()) {
			// skip every other filter here
			break;
		}
	}
	if (filtered.empty()) {
		return;
	}
	const ai::CharacterId id = filtered[0];
	filtered.clear();
	filtered.push_back(id);

}

}
