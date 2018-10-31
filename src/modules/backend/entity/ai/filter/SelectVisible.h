/**
 * @file
 */

#pragma once

#include "backend/entity/ai/AICommon.h"

namespace backend {

/**
 * @ingroup AI
 */
class SelectVisible: public ai::IFilter {
public:
	FILTER_FACTORY(SelectVisible)
	FILTER_CLASS(SelectVisible)

	void filter (const ai::AIPtr& entity) override;
};

}
