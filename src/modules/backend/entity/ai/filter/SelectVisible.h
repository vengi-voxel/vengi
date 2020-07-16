/**
 * @file
 */

#pragma once

#include "core/Log.h"
#include "backend/entity/ai/filter/IFilter.h"

namespace backend {

/**
 * @ingroup AI
 */
class SelectVisible: public IFilter {
public:
	FILTER_FACTORY(SelectVisible)
	FILTER_CLASS(SelectVisible)

	void filter (const AIPtr& entity) override;
};

}
