/**
 * @file
 * @ingroup Filter
 */
#pragma once

#include "filter/IFilter.h"

namespace ai {

/**
 * @brief This filter will just preserve the first entry of other filters
 */
class First: public IFilter {
protected:
	Filters _filters;
public:
	First(const std::string& parameters, const Filters& filters) :
		IFilter("First", parameters), _filters(filters) {
	}
	FILTER_ACTION_FACTORY(First)

	void filter (const AIPtr& entity) override;
};

}
