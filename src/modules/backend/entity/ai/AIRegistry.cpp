/**
 * @file
 */

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
#include "filter/SelectEntitiesOfTypes.h"
#include "filter/SelectIncreasePartner.h"
#include "filter/SelectPrey.h"

namespace backend {

void AIRegistry::init() {
	registerNodeFactory("GoHome", GoHome::getFactory());
	registerNodeFactory("AttackOnSelection", AttackOnSelection::getFactory());
	registerFilterFactory("SelectVisible", SelectVisible::getFactory());
	registerFilterFactory("SelectIncreasePartner", SelectIncreasePartner::getFactory());
	registerFilterFactory("SelectPrey", SelectPrey::getFactory());
	registerFilterFactory("SelectEntitiesOfTypes", SelectEntitiesOfTypes::getFactory());
	registerConditionFactory("IsCloseToSelection", IsCloseToSelection::getFactory());
	registerConditionFactory("IsOnCooldown", IsOnCooldown::getFactory());
	registerConditionFactory("IsSelectionAlive", IsSelectionAlive::getFactory());
	registerNodeFactory("SetPointOfInterest", SetPointOfInterest::getFactory());
	registerNodeFactory("Spawn", Spawn::getFactory());
	registerNodeFactory("Die", core::Singleton<Die::Factory>::getInstance());
	registerNodeFactory("TriggerCooldown", TriggerCooldown::getFactory());
	registerNodeFactory("TriggerCooldownOnSelection", TriggerCooldownOnSelection::getFactory());
}

}
