/**
 * @file
 * @ingroup Filter
 * @image html complement.png
 */
#pragma once

#include "filter/IFilter.h"

namespace ai {

/**
 * @brief This filter performs a complement operation on already filtered entities with the results given by
 * the child filters.
 */
class Complement: public IFilter {
public:
	FILTER_ACTION_CLASS(Complement)
	FILTER_ACTION_FACTORY(Complement)

	void filter (const AIPtr& entity) override;
};

}
