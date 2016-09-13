/**
 * @file
 */

#pragma once

#include "backend/entity/ai/AICommon.h"

using namespace ai;

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
