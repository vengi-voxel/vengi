/**
 * @file
 * @ingroup Filter
 * @image html difference.png
 */
#pragma once

#include "IFilter.h"

namespace backend {

/**
 * @brief This filter performs a difference operation between several filter results. The result
 * consists of elements that are in A and not in B, C, D, ...
 */
class Difference: public IFilter {
public:
	FILTER_ACTION_CLASS(Difference)
	FILTER_ACTION_FACTORY(Difference)

	void filter (const AIPtr& entity) override;
};

}
