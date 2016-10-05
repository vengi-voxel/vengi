/**
 * @file
 */
#pragma once

#include "IAIFactory.h"
#include "common/IFactoryRegistry.h"
#include <vector>

namespace ai {

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
	Filters filters;
	explicit FilterFactoryContext(const std::string& _parameters) :
		parameters(_parameters) {
	}
};

struct SteeringFactoryContext {
	// Parameters for the steering class - can get hand over to the ctor in your factory implementation.
	std::string parameters;
	explicit SteeringFactoryContext(const std::string& _parameters) :
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
	explicit ConditionFactoryContext(const std::string& _parameters) :
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
