#pragma once

#include "IAIFactory.h"
#include "common/IFactoryRegistry.h"
#include <vector>

namespace ai {

namespace movement {
typedef std::vector<SteeringPtr> Steerings;
}

/**
 * @brief Context for ITreeNodeFactory
 */
struct TreeNodeFactoryContext {
	std::string name;
	std::string parameters;
	ConditionPtr condition;
	TreeNodeFactoryContext(const std::string& _name, const std::string& _parameters, const ConditionPtr& _condition) :
			name(_name), parameters(_parameters), condition(_condition) {
	}
};

struct SteerNodeFactoryContext {
	std::string name;
	std::string parameters;
	ConditionPtr condition;
	movement::Steerings steerings;
	SteerNodeFactoryContext(const std::string& _name, const std::string& _parameters, const ConditionPtr& _condition, const movement::Steerings& _steerings) :
			name(_name), parameters(_parameters), condition(_condition), steerings(_steerings) {
	}
};

struct FilterFactoryContext {
	// Parameters for the filter - can get hand over to the ctor in your factory implementation.
	std::string parameters;
	FilterFactoryContext(const std::string& _parameters) :
		parameters(_parameters) {
	}
};

struct SteeringFactoryContext {
	// Parameters for the steering class - can get hand over to the ctor in your factory implementation.
	std::string parameters;
	SteeringFactoryContext(const std::string& _parameters) :
		parameters(_parameters) {
	}
};

struct ConditionFactoryContext {
	// Parameters for the condition - can get hand over to the ctor in your factory implementation.
	std::string parameters;
	// Some conditions have child conditions
	Conditions conditions;
	// The filter condition also has filters
	Filters filters;
	bool filter;
	ConditionFactoryContext(const std::string& _parameters) :
		parameters(_parameters), filter(false) {
	}
};

/**
 * @brief This factory will create tree nodes. It uses the @c TreeNodeFactoryContext to
 * collect all the needed data for this action.
 */
class ITreeNodeFactory: public IFactory<TreeNode, TreeNodeFactoryContext> {
public:
	virtual ~ITreeNodeFactory() {
	}
	virtual TreeNodePtr create(const TreeNodeFactoryContext *ctx) const override = 0;
};

class ISteeringFactory: public IFactory<movement::ISteering, SteeringFactoryContext> {
public:
	virtual ~ISteeringFactory() {
	}
	virtual SteeringPtr create(const SteeringFactoryContext* ctx) const override = 0;
};

class ISteerNodeFactory: public IFactory<TreeNode, SteerNodeFactoryContext> {
public:
	virtual ~ISteerNodeFactory() {
	}
	virtual TreeNodePtr create(const SteerNodeFactoryContext *ctx) const override = 0;
};

class IFilterFactory: public IFactory<IFilter, FilterFactoryContext> {
public:
	virtual ~IFilterFactory() {
	}
	virtual FilterPtr create(const FilterFactoryContext *ctx) const override = 0;
};

class IConditionFactory: public IFactory<ICondition, ConditionFactoryContext> {
public:
	virtual ~IConditionFactory() {
	}
	virtual ConditionPtr create(const ConditionFactoryContext *ctx) const override = 0;
};

/**
 * @brief The place to register your @c TreeNode and @c ICondition factories at
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
	AIRegistry();
	virtual ~AIRegistry();

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

}
