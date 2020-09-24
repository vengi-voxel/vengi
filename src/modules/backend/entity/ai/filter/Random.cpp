/**
 * @file
 * @ingroup Filter
 */

#include "Random.h"
#include "ai-shared/common/CharacterId.h"
#include "backend/entity/ai/common/Random.h"
#include "core/StringUtil.h"
#include "filter/FilteredEntities.h"

namespace backend {

Random::Random(const core::String& parameters, const Filters& filters) :
	IFilter("Random", parameters), _filters(filters) {
	_n = core::string::toInt(parameters);
}

void Random::filter (const AIPtr& entity) {
	for (auto& f : _filters) {
		f->filter(entity);
	}
	FilteredEntities& filtered = getFilteredEntities(entity);
	if (filtered.empty()) {
		return;
	}

	FilteredEntities copy = filtered;
	const int maxFiltered = core_min(_n, (int)copy.size());
	filtered.clear();

	for (int i = 0; i < maxFiltered; ++i) {
		const size_t idx = (size_t)random(0, (int)copy.size() - 1);
		const ai::CharacterId id = copy[idx];
		copy.erase(idx);
		filtered.push_back(id);
	}
}

}
