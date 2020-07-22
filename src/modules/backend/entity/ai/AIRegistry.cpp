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
#include "tree/Fail.h"
#include "tree/Limit.h"
#include "tree/Invert.h"
#include "tree/Idle.h"
#include "tree/Parallel.h"
#include "tree/PrioritySelector.h"
#include "tree/ProbabilitySelector.h"
#include "tree/RandomSelector.h"
#include "tree/Sequence.h"
#include "tree/Steer.h"
#include "tree/Succeed.h"
#include "condition/And.h"
#include "condition/False.h"
#include "condition/HasEnemies.h"
#include "condition/Not.h"
#include "condition/Filter.h"
#include "condition/Or.h"
#include "condition/True.h"
#include "condition/IsInGroup.h"
#include "condition/IsGroupLeader.h"
#include "condition/IsCloseToGroup.h"
#include "condition/IsCloseToSelection.h"
#include "condition/IsSelectionAlive.h"
#include "condition/IsOnCooldown.h"
#include "filter/SelectEmpty.h"
#include "filter/SelectHighestAggro.h"
#include "filter/SelectGroupLeader.h"
#include "filter/SelectGroupMembers.h"
#include "filter/SelectZone.h"
#include "filter/Union.h"
#include "filter/Intersection.h"
#include "filter/Last.h"
#include "filter/First.h"
#include "filter/Random.h"
#include "filter/Difference.h"
#include "filter/Complement.h"
#include "filter/SelectAll.h"
#include "filter/SelectVisible.h"
#include "filter/SelectEntitiesOfTypes.h"
#include "filter/SelectIncreasePartner.h"
#include "movement/SelectionSeek.h"
#include "movement/SelectionFlee.h"
#include "movement/GroupFlee.h"
#include "movement/GroupSeek.h"
#include "movement/Steering.h"
#include "movement/TargetFlee.h"
#include "movement/TargetSeek.h"
#include "movement/Wander.h"
#include "movement/WeightedSteering.h"
#include "movement/WanderAroundHome.h"

namespace backend {

#define R_GET(Name) registerFactory(#Name, Name::getFactory());
#define R_MOVE(Name) registerFactory(#Name, movement::Name::getFactory());

AIRegistry::TreeNodeFactory::TreeNodeFactory() {
	R_GET(Fail);
	R_GET(Limit);
	R_GET(Invert);
	R_GET(Succeed);
	R_GET(Parallel);
	R_GET(PrioritySelector);
	R_GET(ProbabilitySelector);
	R_GET(RandomSelector);
	R_GET(Sequence);
	R_GET(Idle);
	R_GET(GoHome);
	R_GET(AttackOnSelection);
	R_GET(SetPointOfInterest);
	R_GET(Spawn);
	R_GET(Die);
	R_GET(TriggerCooldown);
	R_GET(TriggerCooldownOnSelection);
}

AIRegistry::SteerNodeFactory::SteerNodeFactory() {
	R_GET(Steer);
}

AIRegistry::SteeringFactory::SteeringFactory() {
	R_MOVE(Wander);
	R_MOVE(GroupSeek);
	R_MOVE(GroupFlee);
	R_MOVE(TargetSeek);
	R_MOVE(TargetFlee);
	R_MOVE(SelectionSeek);
	R_MOVE(SelectionFlee);
	R_MOVE(WanderAroundHome);
}

AIRegistry::FilterFactory::FilterFactory() {
	R_GET(SelectEmpty);
	R_GET(SelectGroupLeader);
	R_GET(SelectGroupMembers);
	R_GET(SelectHighestAggro);
	R_GET(SelectZone);
	R_GET(Union);
	R_GET(Intersection);
	R_GET(Last);
	R_GET(First);
	R_GET(Random);
	R_GET(Difference);
	R_GET(Complement);
	R_GET(SelectAll);
	R_GET(SelectVisible);
	R_GET(SelectIncreasePartner);
	R_GET(SelectEntitiesOfTypes);
}

AIRegistry::ConditionFactory::ConditionFactory() {
	R_GET(And);
	R_GET(False);
	R_GET(HasEnemies);
	R_GET(Not);
	R_GET(Or);
	R_GET(True);
	R_GET(Filter);
	R_GET(IsGroupLeader);
	R_GET(IsInGroup);
	R_GET(IsCloseToGroup);
	R_GET(IsCloseToSelection);
	R_GET(IsOnCooldown);
	R_GET(IsSelectionAlive);
}

}
