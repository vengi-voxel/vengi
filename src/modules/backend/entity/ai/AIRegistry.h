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

namespace backend {

/**
 * @brief The place to register your @ai{TreeNode} and @ai{ICondition} factories at
 */
class AIRegistry: public IAIFactory {
protected:
	class TreeNodeFactory: public IFactoryRegistry<TreeNode, TreeNodeFactoryContext> {
	public:
		TreeNodeFactory();
	};

	TreeNodeFactory _treeNodeFactory;

	class SteerNodeFactory: public IFactoryRegistry<TreeNode, SteerNodeFactoryContext> {
	public:
		SteerNodeFactory();
	};

	SteerNodeFactory _steerNodeFactory;

	class SteeringFactory: public IFactoryRegistry<movement::ISteering, SteeringFactoryContext> {
	public:
		SteeringFactory();
	};

	SteeringFactory _steeringFactory;

	class FilterFactory: public IFactoryRegistry<IFilter, FilterFactoryContext> {
	public:
		FilterFactory();
	};

	FilterFactory _filterFactory;

	class ConditionFactory: public IFactoryRegistry<ICondition, ConditionFactoryContext> {
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
	bool registerNodeFactory(const core::String& type, const ITreeNodeFactory& factory);
	/**
	 * @brief Unregisters a tree node factory of the given @c type. This can also be used to replace a built-in
	 * type with a user provided type.
	 *
	 * @return @c true if the unregister action was successful, @c false if not (e.g. it wasn't registered at all)
	 */
	bool unregisterNodeFactory(const core::String& type);

	bool registerSteerNodeFactory(const core::String& type, const ISteerNodeFactory& factory);
	/**
	 * @brief Unregisters a tree node factory of the given @c type. This can also be used to replace a built-in
	 * type with a user provided type.
	 *
	 * @return @c true if the unregister action was successful, @c false if not (e.g. it wasn't registered at all)
	 */
	bool unregisterSteerNodeFactory(const core::String& type);

	bool registerSteeringFactory(const core::String& type, const ISteeringFactory& factory);
	bool unregisterSteeringFactory(const core::String& type);

	bool registerFilterFactory(const core::String& type, const IFilterFactory& factory);

	/**
	 * @brief Unregisters a filter factory of the given @c type. This can also be used to replace a built-in
	 * type with a user provided type.
	 *
	 * @return @c true if the unregister action was successful, @c false if not (e.g. it wasn't registered at all)
	 */
	bool unregisterFilterFactory(const core::String& type);

	bool registerConditionFactory(const core::String& type, const IConditionFactory& factory);
	/**
	 * @brief Unregisters a condition factory of the given @c type. This can also be used to replace a built-in
	 * type with a user provided type.
	 *
	 * @return @c true if the unregister action was successful, @c false if not (e.g. it wasn't registered at all)
	 */
	bool unregisterConditionFactory(const core::String& type);

	TreeNodePtr createNode(const core::String& type, const TreeNodeFactoryContext& ctx) const override;
	TreeNodePtr createSteerNode(const core::String& type, const SteerNodeFactoryContext& ctx) const override;
	FilterPtr createFilter(const core::String& type, const FilterFactoryContext& ctx) const override;
	ConditionPtr createCondition(const core::String& type, const ConditionFactoryContext& ctx) const override;
	SteeringPtr createSteering(const core::String& type, const SteeringFactoryContext& ctx) const override;
};

inline TreeNodePtr AIRegistry::createNode(const core::String& nodeType, const TreeNodeFactoryContext& ctx) const {
	return _treeNodeFactory.create(nodeType, &ctx);
}

inline TreeNodePtr AIRegistry::createSteerNode(const core::String& nodeType, const SteerNodeFactoryContext& ctx) const {
	return _steerNodeFactory.create(nodeType, &ctx);
}

inline FilterPtr AIRegistry::createFilter(const core::String& nodeType, const FilterFactoryContext& ctx) const {
	return _filterFactory.create(nodeType, &ctx);
}

inline bool AIRegistry::registerNodeFactory(const core::String& nodeType, const ITreeNodeFactory& factory) {
	return _treeNodeFactory.registerFactory(nodeType, factory);
}

inline bool AIRegistry::registerFilterFactory(const core::String& nodeType, const IFilterFactory& factory) {
	return _filterFactory.registerFactory(nodeType, factory);
}

inline bool AIRegistry::registerConditionFactory(const core::String& nodeType, const IConditionFactory& factory) {
	return _conditionFactory.registerFactory(nodeType, factory);
}

inline bool AIRegistry::unregisterNodeFactory(const core::String& nodeType) {
	return _treeNodeFactory.unregisterFactory(nodeType);
}

inline bool AIRegistry::registerSteerNodeFactory(const core::String& type, const ISteerNodeFactory& factory) {
	return _steerNodeFactory.registerFactory(type, factory);
}

inline bool AIRegistry::unregisterSteerNodeFactory(const core::String& type) {
	return _steerNodeFactory.unregisterFactory(type);
}

inline bool AIRegistry::registerSteeringFactory(const core::String& type, const ISteeringFactory& factory) {
	return _steeringFactory.registerFactory(type, factory);
}

inline bool AIRegistry::unregisterSteeringFactory(const core::String& type) {
	return _steeringFactory.unregisterFactory(type);
}

inline bool AIRegistry::unregisterFilterFactory(const core::String& nodeType) {
	return _filterFactory.unregisterFactory(nodeType);
}

inline bool AIRegistry::unregisterConditionFactory(const core::String& nodeType) {
	return _conditionFactory.unregisterFactory(nodeType);
}

inline ConditionPtr AIRegistry::createCondition(const core::String& nodeType, const ConditionFactoryContext& ctx) const {
	return _conditionFactory.create(nodeType, &ctx);
}

inline SteeringPtr AIRegistry::createSteering(const core::String& type, const SteeringFactoryContext& ctx) const {
	return _steeringFactory.create(type, &ctx);
}

}
