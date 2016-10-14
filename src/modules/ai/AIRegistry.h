/**
 * @file
 */
#pragma once

#include "IAIFactory.h"
#include "AIFactories.h"
#include "common/IFactoryRegistry.h"
#include "tree/TreeNode.h"
#include "conditions/ICondition.h"
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
#include "filter/IFilter.h"
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

namespace ai {

#define R_GET(Name) registerFactory(#Name, Name::getFactory());
#define R_MOVE(Name) registerFactory(#Name, movement::Name::getFactory());

/**
 * @brief The place to register your @ai{TreeNode} and @ai{ICondition} factories at
 */
class AIRegistry: public IAIFactory {
protected:
	class TreeNodeFactory: public IFactoryRegistry<std::string, TreeNode, TreeNodeFactoryContext> {
	public:
		TreeNodeFactory() {
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
	};

	TreeNodeFactory _treeNodeFactory;

	class SteerNodeFactory: public IFactoryRegistry<std::string, TreeNode, SteerNodeFactoryContext> {
	public:
		SteerNodeFactory() {
			R_GET(Steer);
		}
	};

	SteerNodeFactory _steerNodeFactory;

	class SteeringFactory: public IFactoryRegistry<std::string, movement::ISteering, SteeringFactoryContext> {
	public:
		SteeringFactory() {
			R_MOVE(Wander);
			R_MOVE(GroupSeek);
			R_MOVE(GroupFlee);
			R_MOVE(TargetSeek);
			R_MOVE(TargetFlee);
			R_MOVE(SelectionSeek);
			R_MOVE(SelectionFlee);
		}
	};

	SteeringFactory _steeringFactory;

	class FilterFactory: public IFactoryRegistry<std::string, IFilter, FilterFactoryContext> {
	public:
		FilterFactory() {
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
	};

	FilterFactory _filterFactory;

	class ConditionFactory: public IFactoryRegistry<std::string, ICondition, ConditionFactoryContext> {
	public:
		ConditionFactory() {
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
	};

	ConditionFactory _conditionFactory;
public:
	AIRegistry() :
			IAIFactory() {
	}
	virtual ~AIRegistry() {}

	/**
	 * @brief Registers a tree node factory of the given @c type.
	 * @param[in] type The name that is used in the behaviour tree to identify nodes of the
	 * that are assigned to the given factory
	 * @param[in] factory The factory that will create the real node.
	 * @return @c true if the unregister action was successful, @c false if not (e.g. it wasn't registered at all)
	 */
	bool registerNodeFactory(const std::string& type, const ITreeNodeFactory& factory);
	/**
	 * @brief Unregisters a tree node factory of the given @c type. This can also be used to replace a built-in
	 * type with a user provided type.
	 *
	 * @return @c true if the unregister action was successful, @c false if not (e.g. it wasn't registered at all)
	 */
	bool unregisterNodeFactory(const std::string& type);

	bool registerSteerNodeFactory(const std::string& type, const ISteerNodeFactory& factory);
	/**
	 * @brief Unregisters a tree node factory of the given @c type. This can also be used to replace a built-in
	 * type with a user provided type.
	 *
	 * @return @c true if the unregister action was successful, @c false if not (e.g. it wasn't registered at all)
	 */
	bool unregisterSteerNodeFactory(const std::string& type);

	bool registerSteeringFactory(const std::string& type, const ISteeringFactory& factory);
	bool unregisterSteeringFactory(const std::string& type);

	bool registerFilterFactory(const std::string& type, const IFilterFactory& factory);

	/**
	 * @brief Unregisters a filter factory of the given @c type. This can also be used to replace a built-in
	 * type with a user provided type.
	 *
	 * @return @c true if the unregister action was successful, @c false if not (e.g. it wasn't registered at all)
	 */
	bool unregisterFilterFactory(const std::string& type);

	bool registerConditionFactory(const std::string& type, const IConditionFactory& factory);
	/**
	 * @brief Unregisters a condition factory of the given @c type. This can also be used to replace a built-in
	 * type with a user provided type.
	 *
	 * @return @c true if the unregister action was successful, @c false if not (e.g. it wasn't registered at all)
	 */
	bool unregisterConditionFactory(const std::string& type);

	TreeNodePtr createNode(const std::string& type, const TreeNodeFactoryContext& ctx) const override;
	TreeNodePtr createSteerNode(const std::string& type, const SteerNodeFactoryContext& ctx) const override;
	FilterPtr createFilter(const std::string& type, const FilterFactoryContext& ctx) const override;
	ConditionPtr createCondition(const std::string& type, const ConditionFactoryContext& ctx) const override;
	SteeringPtr createSteering(const std::string& type, const SteeringFactoryContext& ctx) const override;
};

#undef R_GET
#undef R_MOVE

inline TreeNodePtr AIRegistry::createNode(const std::string& nodeType, const TreeNodeFactoryContext& ctx) const {
	return _treeNodeFactory.create(nodeType, &ctx);
}

inline TreeNodePtr AIRegistry::createSteerNode(const std::string& nodeType, const SteerNodeFactoryContext& ctx) const {
	return _steerNodeFactory.create(nodeType, &ctx);
}

inline FilterPtr AIRegistry::createFilter(const std::string& nodeType, const FilterFactoryContext& ctx) const {
	return _filterFactory.create(nodeType, &ctx);
}

inline bool AIRegistry::registerNodeFactory(const std::string& nodeType, const ITreeNodeFactory& factory) {
	return _treeNodeFactory.registerFactory(nodeType, factory);
}

inline bool AIRegistry::registerFilterFactory(const std::string& nodeType, const IFilterFactory& factory) {
	return _filterFactory.registerFactory(nodeType, factory);
}

inline bool AIRegistry::registerConditionFactory(const std::string& nodeType, const IConditionFactory& factory) {
	return _conditionFactory.registerFactory(nodeType, factory);
}

inline bool AIRegistry::unregisterNodeFactory(const std::string& nodeType) {
	return _treeNodeFactory.unregisterFactory(nodeType);
}

inline bool AIRegistry::registerSteerNodeFactory(const std::string& type, const ISteerNodeFactory& factory) {
	return _steerNodeFactory.registerFactory(type, factory);
}

inline bool AIRegistry::unregisterSteerNodeFactory(const std::string& type) {
	return _steerNodeFactory.unregisterFactory(type);
}

inline bool AIRegistry::registerSteeringFactory(const std::string& type, const ISteeringFactory& factory) {
	return _steeringFactory.registerFactory(type, factory);
}

inline bool AIRegistry::unregisterSteeringFactory(const std::string& type) {
	return _steeringFactory.unregisterFactory(type);
}

inline bool AIRegistry::unregisterFilterFactory(const std::string& nodeType) {
	return _filterFactory.unregisterFactory(nodeType);
}

inline bool AIRegistry::unregisterConditionFactory(const std::string& nodeType) {
	return _conditionFactory.unregisterFactory(nodeType);
}

inline ConditionPtr AIRegistry::createCondition(const std::string& nodeType, const ConditionFactoryContext& ctx) const {
	return _conditionFactory.create(nodeType, &ctx);
}

inline SteeringPtr AIRegistry::createSteering(const std::string& type, const SteeringFactoryContext& ctx) const {
	return _steeringFactory.create(type, &ctx);
}

}
