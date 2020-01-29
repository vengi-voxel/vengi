/**
 * @file
 * @ingroup Filter
 */
#pragma once

#include "filter/IFilter.h"

namespace ai {

/**
 * @brief This filter will just preserve the last entry of other filters
 */
class Last: public IFilter {
protected:
	Filters _filters;
public:
	Last(const core::String& parameters, const Filters& filters) :
		IFilter("Last", parameters), _filters(filters) {
	}
	FILTER_ACTION_FACTORY(Last)

	void filter (const AIPtr& entity) override;
};

}
