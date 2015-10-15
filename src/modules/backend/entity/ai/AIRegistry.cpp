#include "AIRegistry.h"

#include "backend/entity/EntityStorage.h"
#include "backend/spawn/SpawnMgr.h"
#include "core/Singleton.h"

#include "action/Die.h"
#include "action/GoHome.h"
#include "action/Spawn.h"
#include "action/AttackOnSelection.h"
#include "action/SetPointOfInterest.h"
#include "action/TriggerCooldown.h"
#include "action/TriggerCooldownOnSelection.h"

#include "condition/IsCloseToSelection.h"
#include "condition/IsSelectionAlive.h"
#include "condition/IsOnCooldown.h"

#include "filter/SelectVisible.h"
#include "filter/SelectNpcsOfTypes.h"
#include "filter/SelectIncreasePartner.h"
#include "filter/SelectPrey.h"

namespace backend {

void AIRegistry::init(const backend::SpawnMgrPtr& spawnMgr) {
	registerNodeFactory("GoHome", core::Singleton<GoHome::Factory>::getInstance());
	registerNodeFactory("AttackOnSelection", core::Singleton<AttackOnSelection::Factory>::getInstance());
	registerFilterFactory("SelectVisible", SelectVisible::FACTORY);
	registerFilterFactory("SelectIncreasePartner", SelectIncreasePartner::FACTORY);
	registerFilterFactory("SelectPrey", SelectPrey::FACTORY);
	registerFilterFactory("SelectNpcsOfTypes", SelectNpcsOfTypes::FACTORY);
	registerConditionFactory("IsCloseToSelection", IsCloseToSelection::FACTORY);
	registerConditionFactory("IsOnCooldown", IsOnCooldown::FACTORY);
	registerConditionFactory("IsSelectionAlive", IsSelectionAlive::FACTORY);
	registerNodeFactory("SetPointOfInterest", SetPointOfInterest::FACTORY);
	registerNodeFactory("Spawn", Spawn::getInstance(spawnMgr));
	registerNodeFactory("Die", core::Singleton<Die::Factory>::getInstance());
	registerNodeFactory("TriggerCooldown", TriggerCooldown::FACTORY);
	registerNodeFactory("TriggerCooldownOnSelection", TriggerCooldownOnSelection::FACTORY);
}

}
