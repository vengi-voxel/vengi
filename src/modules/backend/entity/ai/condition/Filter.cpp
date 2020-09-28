/**
 * @file
 * @ingroup Condition
 * @ingroup Filter
 */

#include "Filter.h"
#include "backend/entity/ai/AI.h"
#include "core/Common.h"

namespace backend {

void Filter::getConditionNameWithValue(core::String& s, const AIPtr& entity) {
	bool first = true;
	s += "(";
	for (const FilterPtr& filter : _filters) {
		if (!first) {
			s += ",";
		}
		s += filter->getName();
		const core::String& param = filter->getParameters();
		if (!param.empty()) {
			s += "{";
			s += param;
			s += "}";
		}
		first = false;
	}
	s += ")[";
	bool firstChr = true;
	int cnt = 0;
	for (ai::CharacterId id : entity->_filteredEntities) {
		if (!firstChr) {
			s += ",";
		}
		s.append((int)id);
		firstChr = false;
		++cnt;
		if (cnt > 5) {
			s += ",...";
			break;
		}
	}
	s += "]";
}

Filter::Filter (const Filters& filters) :
		ICondition(FILTER_NAME, ""), _filters(filters) {
}

bool Filter::evaluate(const AIPtr& entity) {
	entity->_filteredEntities.clear();
	for (const FilterPtr& filter : _filters) {
		filter->filter(entity);
	}
	return state(!entity->_filteredEntities.empty());
}

}
