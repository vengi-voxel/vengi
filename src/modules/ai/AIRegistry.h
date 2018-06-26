/**
 * @file
 */
#pragma once

#include "IAIFactory.h"
#include "AIFactories.h"
#include "common/IFactoryRegistry.h"
#include "tree/TreeNode.h"
#include "conditions/ICondition.h"
#include "filter/IFilter.h"

namespace ai {

/**
 * @brief The place to register your @ai{TreeNode} and @ai{ICondition} factories at
 */
class AIRegistry: public IAIFactory {
protected:
	class TreeNodeFactory: public IFactoryRegistry<std::string, TreeNode, TreeNodeFactoryContext> {
	public:
		TreeNodeFactory();
	};

	TreeNodeFactory _treeNodeFactory;

	class SteerNodeFactory: public IFactoryRegistry<std::string, TreeNode, SteerNodeFactoryContext> {
	public:
		SteerNodeFactory();
	};

	SteerNodeFactory _steerNodeFactory;

	class SteeringFactory: public IFactoryRegistry<std::string, movement::ISteering, SteeringFactoryContext> {
	public:
		SteeringFactory();
	};

	SteeringFactory _steeringFactory;

	class FilterFactory: public IFactoryRegistry<std::string, IFilter, FilterFactoryContext> {
	public:
		FilterFactory();
	};

	FilterFactory _filterFactory;

	class ConditionFactory: public IFactoryRegistry<std::string, ICondition, ConditionFactoryContext> {
	public:
		ConditionFactory();
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
