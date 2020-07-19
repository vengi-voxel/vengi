/**
 * @file
 * @ingroup Filter
 */
#pragma once

#include "IFilter.h"

namespace backend {

/**
 * @brief This filter will pick the entity with the highest aggro value
 */
class SelectHighestAggro: public IFilter {
public:
	FILTER_CLASS_SINGLETON(SelectHighestAggro)

	void filter (const AIPtr& entity) override;
};

}
