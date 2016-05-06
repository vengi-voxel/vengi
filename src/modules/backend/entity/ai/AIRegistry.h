/**
 * @file
 */

#pragma once

#include "AICommon.h"
#include <memory>

#include "backend/entity/EntityStorage.h"

namespace backend {

class SpawnMgr;
typedef std::shared_ptr<SpawnMgr> SpawnMgrPtr;

class AIRegistry: public ai::AIRegistry {
public:
	void init(const backend::SpawnMgrPtr& spawnMgr);
};

typedef std::shared_ptr<AIRegistry> AIRegistryPtr;

}
