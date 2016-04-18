#pragma once

#include "AICommon.h"
#include <tree/loaders/lua/LUATreeLoader.h>
#include <memory>

#include "AIRegistry.h"

namespace backend {

class AILoader: public ai::LUATreeLoader {
private:
	AIRegistryPtr _registry;
public:
	AILoader(AIRegistryPtr registry) :
			ai::LUATreeLoader(*registry.get()), _registry(registry) {
	}
};

typedef std::shared_ptr<AILoader> AILoaderPtr;

}
