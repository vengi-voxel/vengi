/**
 * @file
 * @ingroup Condition
 * @ingroup Filter
 */

#include "Filter.h"
#include "backend/entity/ai/AI.h"

namespace backend {

void Filter::getConditionNameWithValue(core::String& s, const AIPtr& entity) {
	bool first = true;
	s += "(";
	auto copy = entity->_filteredEntities;
	for (const FilterPtr& filter : _filters) {
		if (!first)
			s += ",";
		s += filter->getName();
		s += "{";
		s += filter->getParameters();
		s += "}[";
		entity->_filteredEntities.clear();
		filter->filter(entity);
		bool firstChr = true;
		int cnt = 0;
		for (ai::CharacterId id : entity->_filteredEntities) {
			if (!firstChr)
				s += ",";
			s += id;
			firstChr = false;
			++cnt;
			if (cnt > 15) {
				s += ",...";
				break;
			}
		}
		s += "]";
		first = false;
	}
	entity->_filteredEntities = copy;
	s += ")";
}

Filter::Filter (const Filters& filters) :
		ICondition(FILTER_NAME, ""), _filters(filters) {
}

bool Filter::evaluate(const AIPtr& entity) {
	entity->_filteredEntities.clear();
	for (const FilterPtr& filter : _filters) {
		filter->filter(entity);
	}
	return !entity->_filteredEntities.empty();
}

}
