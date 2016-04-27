#include "AIRegistry.h"
#include "tree/TreeNode.h"
#include "conditions/ICondition.h"
#include "tree/Fail.h"
#include "tree/Limit.h"
#include "tree/Invert.h"
#include "tree/Idle.h"
#include "tree/Print.h"
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
#include "filter/IFilter.h"
#include "filter/SelectEmpty.h"
#include "filter/SelectHighestAggro.h"
#include "filter/SelectGroupLeader.h"
#include "filter/SelectGroupMembers.h"
#include "filter/SelectZone.h"
#include "movement/SelectionSeek.h"
#include "movement/SelectionFlee.h"
#include "movement/GroupFlee.h"
#include "movement/GroupSeek.h"
#include "movement/Steering.h"
#include "movement/TargetFlee.h"
#include "movement/TargetSeek.h"
#include "movement/Wander.h"
#include "movement/WeightedSteering.h"

namespace ai {

#define R(Name) registerFactory(#Name, Name::FACTORY);
#define R_MOVE(Name) registerFactory(#Name, movement::Name::FACTORY);

AIRegistry::ConditionFactory::ConditionFactory() {
	R(And);
	R(False);
	R(HasEnemies);
	R(Not);
	R(Or);
	R(True);
	R(Filter);
	R(IsGroupLeader);
	R(IsInGroup);
	R(IsCloseToGroup);
}

AIRegistry::FilterFactory::FilterFactory() {
	R(SelectEmpty);
	R(SelectGroupLeader);
	R(SelectGroupMembers);
	R(SelectHighestAggro);
	R(SelectZone);
}

AIRegistry::TreeNodeFactory::TreeNodeFactory() {
	R(Fail);
	R(Limit);
	R(Invert);
	R(Succeed);
	R(Parallel);
	R(PrioritySelector);
	R(ProbabilitySelector);
	R(RandomSelector);
	R(Sequence);
	R(Print);
	R(Idle);
}

AIRegistry::SteerNodeFactory::SteerNodeFactory() {
	R(Steer);
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

#undef R

AIRegistry::AIRegistry() :
		IAIFactory() {
}

AIRegistry::~AIRegistry() {
}

TreeNodePtr AIRegistry::createNode(const std::string& nodeType, const TreeNodeFactoryContext& ctx) const {
	return _treeNodeFactory.create(nodeType, &ctx);
}

TreeNodePtr AIRegistry::createSteerNode(const std::string& nodeType, const SteerNodeFactoryContext& ctx) const {
	return _steerNodeFactory.create(nodeType, &ctx);
}

FilterPtr AIRegistry::createFilter(const std::string& nodeType, const FilterFactoryContext& ctx) const {
	return _filterFactory.create(nodeType, &ctx);
}

bool AIRegistry::registerNodeFactory(const std::string& nodeType, const ITreeNodeFactory& factory) {
	return _treeNodeFactory.registerFactory(nodeType, factory);
}

bool AIRegistry::registerFilterFactory(const std::string& nodeType, const IFilterFactory& factory) {
	return _filterFactory.registerFactory(nodeType, factory);
}

bool AIRegistry::registerConditionFactory(const std::string& nodeType, const IConditionFactory& factory) {
	return _conditionFactory.registerFactory(nodeType, factory);
}

bool AIRegistry::unregisterNodeFactory(const std::string& nodeType) {
	return _treeNodeFactory.unregisterFactory(nodeType);
}

bool AIRegistry::registerSteerNodeFactory(const std::string& type, const ISteerNodeFactory& factory) {
	return _steerNodeFactory.registerFactory(type, factory);
}

bool AIRegistry::unregisterSteerNodeFactory(const std::string& type) {
	return _steerNodeFactory.unregisterFactory(type);
}

bool AIRegistry::registerSteeringFactory(const std::string& type, const ISteeringFactory& factory) {
	return _steeringFactory.registerFactory(type, factory);
}

bool AIRegistry::unregisterSteeringFactory(const std::string& type) {
	return _steeringFactory.unregisterFactory(type);
}

bool AIRegistry::unregisterFilterFactory(const std::string& nodeType) {
	return _filterFactory.unregisterFactory(nodeType);
}

bool AIRegistry::unregisterConditionFactory(const std::string& nodeType) {
	return _conditionFactory.unregisterFactory(nodeType);
}

ConditionPtr AIRegistry::createCondition(const std::string& nodeType, const ConditionFactoryContext& ctx) const {
	return _conditionFactory.create(nodeType, &ctx);
}

SteeringPtr AIRegistry::createSteering(const std::string& type, const SteeringFactoryContext& ctx) const {
	return _steeringFactory.create(type, &ctx);
}

}
