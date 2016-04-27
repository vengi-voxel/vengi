#pragma once

#include "filter/IFilter.h"

namespace ai {

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
