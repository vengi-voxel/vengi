#pragma once

#include "filter/IFilter.h"

namespace ai {

/**
 * @brief This filter will pick the entities from the zone of the given entity
 */
class SelectZone: public IFilter {
public:
	FILTER_FACTORY

	SelectZone(const std::string& parameters = "") :
		IFilter("SelectZone", parameters) {
	}

	void filter (const AIPtr& entity) override;
};

}
