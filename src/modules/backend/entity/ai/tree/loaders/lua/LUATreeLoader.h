/**
 * @file
 * @defgroup LUA
 * @{
 * @code
 * function idle (parentnode)
 * 	local prio = parentnode:addNode("PrioritySelector", "walkuncrowded")
 * 		prio:addNode("Steer(Wander)", "wanderfreely")
 * end
 *
 * function wolf ()
 * 	local name = "ANIMAL_WOLF"
 * 	local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
 * 	local parallel = rootnode:addNode("Parallel", "hunt")
 * 	parallel:setCondition("Not(IsOnCooldown{HUNT})")
 * 		parallel:addNode("Steer(SelectionSeek)", "follow"):setCondition("Filter(SelectEntitiesOfType{ANIMAL_RABBIT})")
 * 		parallel:addNode("AttackOnSelection", "attack"):setCondition("IsCloseToSelection{1}")
 * 		parallel:addNode("SetPointOfInterest", "setpoi"):setCondition("IsCloseToSelection{1}")
 * 		parallel:addNode("TriggerCooldown{HUNT}", "increasecooldown"):setCondition("Not(IsSelectionAlive)")
 * 	idle(rootNode)
 * end
 *
 * function rabbit ()
 * 	local name = "ANIMAL_RABBIT"
 * 	local rootNode = AI.createTree(name):createRoot("PrioritySelector", name)
 * 	rootnode:addNode("Steer(SelectionFlee)", "fleefromhunter"):setCondition("And(Filter(SelectEntitiesOfTypes{ANIMAL_WOLF}),IsCloseToSelection{10})")
 * 	idle(rootNode)
 * end
 *
 * function init ()
 * 	wolf()
 * 	rabbit()
 * end
 * @endcode
 */
#pragma once

#include "backend/entity/ai/tree/loaders/ITreeLoader.h"
#include "backend/entity/ai/tree/TreeNode.h"
#include "backend/entity/ai/AIRegistry.h"
#include "backend/entity/ai/conditions/ConditionParser.h"
#include "backend/entity/ai/tree/TreeNodeParser.h"
#include "commonlua/LUA.h"

namespace backend {

/**
 * @brief Implementation of @c ITreeLoader that gets its data from a lua script
 */
class LUATreeLoader: public ITreeLoader {
public:
	LUATreeLoader(const IAIFactory& aiFactory);

	/**
	 * @brief this will initialize the loader once with all the defined behaviours from the given lua string.
	 */
	bool init(const core::String& luaString);
};

}

/**
 * @}
 */
