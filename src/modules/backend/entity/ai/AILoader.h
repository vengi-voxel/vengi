/**
 * @file
 */

#pragma once

#include "core/Log.h"
#include "tree/loaders/lua/LUATreeLoader.h"
#include <memory>

#include "LUAAIRegistry.h"

namespace backend {

/**
 * @ingroup AI
 */
class AILoader: public LUATreeLoader {
private:
	AIRegistryPtr _registry;
public:
	AILoader(const AIRegistryPtr& registry) :
			LUATreeLoader(*registry.get()), _registry(registry) {
	}
};

typedef std::shared_ptr<AILoader> AILoaderPtr;

}
