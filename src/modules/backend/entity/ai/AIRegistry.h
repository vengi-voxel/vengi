/**
 * @file
 */

#pragma once

#include "AICommon.h"
#include <memory>

namespace backend {

/**
 * @ingroup AI
 */
class AIRegistry: public ai::AIRegistry {
public:
	void init();
};

typedef std::shared_ptr<AIRegistry> AIRegistryPtr;

}
