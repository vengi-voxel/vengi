/**
 * @file
 * @ingroup Filter
 */
#pragma once

#include "filter/IFilter.h"

namespace ai {

/**
 * @brief This filter will preserve only a few random entries
 */
class Random: public IFilter {
protected:
	Filters _filters;
	int _n;
public:
	Random(const core::String& parameters, const Filters& filters) :
		IFilter("Random", parameters), _filters(filters) {
		_n = std::stoi(parameters);
	}

	FILTER_ACTION_FACTORY(Random)

	void filter (const AIPtr& entity) override;
};

}
