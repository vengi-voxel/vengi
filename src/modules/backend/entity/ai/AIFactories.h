/**
 * @file
 */
#pragma once

#include "IAIFactory.h"
#include "common/IFactoryRegistry.h"
#include <vector>
#include <list>

namespace backend {

namespace movement {
typedef std::vector<SteeringPtr> Steerings;
}
typedef std::vector<ConditionPtr> Conditions;
typedef std::list<FilterPtr> Filters;

/**
 * @brief Context for ITreeNodeFactory
 */
struct TreeNodeFactoryContext {
	core::String name;
	core::String parameters;
	ConditionPtr condition;
	TreeNodeFactoryContext(const core::String& _name, const core::String& _parameters, const ConditionPtr& _condition) :
			name(_name), parameters(_parameters), condition(_condition) {
	}
};

struct SteerNodeFactoryContext {
	core::String name;
	core::String parameters;
	ConditionPtr condition;
	movement::Steerings steerings;
	SteerNodeFactoryContext(const core::String& _name, const core::String& _parameters, const ConditionPtr& _condition, const movement::Steerings& _steerings) :
			name(_name), parameters(_parameters), condition(_condition), steerings(_steerings) {
	}
};

struct FilterFactoryContext {
	// Parameters for the filter - can get hand over to the ctor in your factory implementation.
	core::String parameters;
	Filters filters;
	explicit FilterFactoryContext(const core::String& _parameters) :
		parameters(_parameters) {
	}
};

struct SteeringFactoryContext {
	// Parameters for the steering class - can get hand over to the ctor in your factory implementation.
	core::String parameters;
	explicit SteeringFactoryContext(const core::String& _parameters) :
		parameters(_parameters) {
	}
};

struct ConditionFactoryContext {
	// Parameters for the condition - can get hand over to the ctor in your factory implementation.
	core::String parameters;
	// Some conditions have child conditions
	Conditions conditions;
	// The filter condition also has filters
	Filters filters;
	bool filter;
	explicit ConditionFactoryContext(const core::String& _parameters) :
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

}
