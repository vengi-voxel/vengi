/**
 * @file
 * @ingroup Filter
 */
#pragma once

#include "IFilter.h"

namespace backend {

/**
 * @brief This filter just clears the selection
 */
class SelectEmpty: public IFilter {
public:
	FILTER_CLASS_SINGLETON(SelectEmpty)

	void filter (const AIPtr& entity) override;
};

inline void SelectEmpty::filter (const AIPtr& entity) {
	getFilteredEntities(entity).clear();
}

}
