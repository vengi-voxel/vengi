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
class AIRegistry: public ai::LUAAIRegistry {
private:
	using Super = ai::LUAAIRegistry;
public:
	bool init() override;
};

typedef std::shared_ptr<AIRegistry> AIRegistryPtr;

}
