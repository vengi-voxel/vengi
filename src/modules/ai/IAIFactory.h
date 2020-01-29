/**
 * @file
 */
#pragma once

#include <memory>
#include "core/String.h"
#include <vector>
#include <list>

namespace ai {

class TreeNode;
typedef std::shared_ptr<TreeNode> TreeNodePtr;
typedef std::vector<TreeNodePtr> TreeNodes;

class IFilter;
typedef std::shared_ptr<IFilter> FilterPtr;
typedef std::list<FilterPtr> Filters;

namespace movement {
class ISteering;
}
typedef std::shared_ptr<movement::ISteering> SteeringPtr;
namespace movement {
typedef std::vector<SteeringPtr> Steerings;
}

class ICondition;
typedef std::shared_ptr<ICondition> ConditionPtr;
typedef std::vector<ConditionPtr> Conditions;

struct TreeNodeFactoryContext;
struct ConditionFactoryContext;
struct FilterFactoryContext;
struct SteerNodeFactoryContext;
struct SteeringFactoryContext;

class IAIFactory {
public:
	virtual ~IAIFactory() {
	}

	/**
	 * @brief Allocates a new @c TreeNode for the given @c type. The @c type must be registered in the @c AIRegistry for this to work.
	 */
	virtual TreeNodePtr createNode(const core::String& type, const TreeNodeFactoryContext& ctx) const = 0;
	/**
	 * @brief Allocates a new @c TreeNode for the given @c type. The @c type must be registered in the @c AIRegistry for this to work.
	 */
	virtual TreeNodePtr createSteerNode(const core::String& type, const SteerNodeFactoryContext& ctx) const = 0;
	/**
	 * @brief Allocates a new @c IFilter for the given @c type. The @c type must be registered in the @c AIRegistry for this to work.
	 */
	virtual FilterPtr createFilter(const core::String& type, const FilterFactoryContext& ctx) const = 0;
	/**
	 * @brief Allocates a new @c ICondition for the given @c type. The @c type must be registered in the @c AIRegistry for this to work.
	 */
	virtual ConditionPtr createCondition(const core::String& type, const ConditionFactoryContext& ctx) const = 0;
	/**
	 * @brief Creates a new @c ISteering for the given @c type. The @c type must be registered in the @c AIRegistry for this to work.
	 */
	virtual SteeringPtr createSteering(const core::String& type, const SteeringFactoryContext& ctx) const = 0;
};

}
