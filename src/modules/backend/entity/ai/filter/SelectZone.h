/**
 * @file
 * @ingroup Filter
 */
#pragma once

#include "IFilter.h"

namespace backend {

/**
 * @brief This filter will pick the entities from the zone of the given entity
 */
class SelectZone: public IFilter {
public:
	FILTER_FACTORY(SelectZone)

	explicit SelectZone(const core::String& parameters = "");

	void filter (const AIPtr& entity) override;
};

}
