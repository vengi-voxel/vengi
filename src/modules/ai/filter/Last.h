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
	Last(const std::string& parameters, const Filters& filters) :
		IFilter("Last", parameters), _filters(filters) {
		ai_assert(filters.size() == 1, "Last must have one child");
	}
	FILTER_ACTION_FACTORY(Last)

	void filter (const AIPtr& entity) override;
};

}
