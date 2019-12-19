/**
 * @file
 * @ingroup Filter
 * @image html union.png
 */
#pragma once

#include "filter/IFilter.h"

namespace ai {

/**
 * @brief This filter merges several other filter results
 */
class Union: public IFilter {
public:
	FILTER_ACTION_CLASS(Union)
	FILTER_ACTION_FACTORY(Union)

	void filter (const AIPtr& entity) override;
};

}
