/**
 * @file
 * @ingroup Filter
 * @image html intersection.png
 */
#pragma once

#include "IFilter.h"

namespace backend {

/**
 * @brief This filter performs an intersection between several filter results
 */
class Intersection: public IFilter {
public:
	FILTER_ACTION_CLASS(Intersection)
	FILTER_ACTION_FACTORY(Intersection)

	void filter (const AIPtr& entity) override;
};

}
