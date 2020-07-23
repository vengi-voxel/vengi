/**
 * @file
 */

#include "IFilter.h"
#include "backend/entity/ai/AI.h"

namespace backend {

FilteredEntities& IFilter::getFilteredEntities(const AIPtr& ai) {
	return ai->_filteredEntities;
}

IFilter::IFilter(const core::String& name, const core::String& parameters) :
		_name(name), _parameters(parameters) {
}

IFilter::~IFilter() {
}

}
