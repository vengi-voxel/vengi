/**
 * @file
 */

#include "AIRegistry.h"
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
#include "conditions/And.h"
#include "conditions/False.h"
#include "conditions/HasEnemies.h"
#include "conditions/Not.h"
#include "conditions/Filter.h"
#include "conditions/Or.h"
#include "conditions/True.h"
#include "conditions/IsInGroup.h"
#include "conditions/IsGroupLeader.h"
#include "conditions/IsCloseToGroup.h"
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
#include "movement/SelectionSeek.h"
#include "movement/SelectionFlee.h"
#include "movement/GroupFlee.h"
#include "movement/GroupSeek.h"
#include "movement/Steering.h"
#include "movement/TargetFlee.h"
#include "movement/TargetSeek.h"
#include "movement/Wander.h"
#include "movement/WeightedSteering.h"

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
}

}
