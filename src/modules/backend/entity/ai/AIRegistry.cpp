/**
 * @file
 */

#include "AIRegistry.h"

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

#include "movement/WanderAroundHome.h"

#include "attrib/ContainerProvider.h"

#include "core/io/Filesystem.h"
#include "core/App.h"

namespace backend {

bool AIRegistry::init() {
	if (!Super::init()) {
		Log::error("Failed to initialize the lua behaviour tree registry");
		return false;
	}

	const core::String& script = io::filesystem()->load("behaviourtreenodes.lua");
	if (!evaluate(script)) {
		Log::error("Failed to load behaviour tree nodes");
		return false;
	}

	registerNodeFactory("GoHome", GoHome::getFactory());
	registerNodeFactory("AttackOnSelection", AttackOnSelection::getFactory());
	registerNodeFactory("SetPointOfInterest", SetPointOfInterest::getFactory());
	registerNodeFactory("Spawn", Spawn::getFactory());
	registerNodeFactory("Die", Die::getFactory());
	registerNodeFactory("TriggerCooldown", TriggerCooldown::getFactory());
	registerNodeFactory("TriggerCooldownOnSelection", TriggerCooldownOnSelection::getFactory());

	registerConditionFactory("IsCloseToSelection", IsCloseToSelection::getFactory());
	registerConditionFactory("IsOnCooldown", IsOnCooldown::getFactory());
	registerConditionFactory("IsSelectionAlive", IsSelectionAlive::getFactory());

	registerFilterFactory("SelectVisible", SelectVisible::getFactory());
	registerFilterFactory("SelectIncreasePartner", SelectIncreasePartner::getFactory());
	registerFilterFactory("SelectEntitiesOfTypes", SelectEntitiesOfTypes::getFactory());

	registerSteeringFactory("WanderAroundHome", WanderAroundHome::getFactory());
	return true;
}

}
