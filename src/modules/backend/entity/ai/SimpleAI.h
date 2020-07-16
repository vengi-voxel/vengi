/**
 * * LUA script interface to create the trees - but every other data source
 *   is possible, too.
 * * @ref Aggro list implementation
 * * Several standard selectors and conditions (see below)
 * * Group management
 * * Movement implementations for steering
 * * Network based remote @ref debugging with live editing of the behaviour tree
 * * QT5 based remote debugger
 *   * can also be used for other AI implementations. (E.g. there are java protocol classes)
 *   * [example](https://github.com/mgerhardy/simpleai/blob/master/contrib/exampledebugger) on how to extend the debugger to render application specific stuff into the map view.
 * * @ref Zone support (each zone can be debugged separately)
 * * @ref Filter support
 *
 * As a default set of conditions, nodes and filters that SimpleAI already comes with:
 * * Conditions
 *   * @ai{And}
 *   * @ai{False}
 *   * @ai{Filter}
 *   * @ai{HasEnemies}
 *   * @ai{IsCloseToGroup}
 *   * @ai{IsGroupLeader}
 *   * @ai{IsInGroup}
 *   * @ai{Not}
 *   * @ai{Or}
 *   * @ai{True}
 * * Nodes
 *   * @ai{Fail}
 *   * @ai{Idle}
 *   * @ai{Invert}
 *   * @ai{Limit}
 *   * @ai{Parallel}
 *   * @ai{PrioritySelector}
 *   * @ai{ProbabilitySelector}
 *   * @ai{RandomSelector}
 *   * @ai{Sequence}
 *   * @ai{Steer}
 *   * @ai{Succeed}
 * * Filter
 *   * @ai{Complement}
 *   * @ai{Difference}
 *   * @ai{First} - only put the first entry of a filter to the list
 *   * @ai{Intersection} - intersection between several filter results
 *   * @ai{Last} - only put the last entry of a filter to the list
 *   * @ai{Random} - only preserve n random entries of the sub filters
 *   * @ai{SelectAll} - this filter is a nop - it will just use the already filtered entities
 *   * @ai{SelectEmpty} - clears the selection
 *   * @ai{SelectGroupLeader}
 *   * @ai{SelectGroupMembers} - select all the group members of a specified group
 *   * @ai{SelectHighestAggro} - put the highest @ref Aggro @ai{CharacterId} into the selection
 *   * @ai{SelectZone} - select all known entities in the zone
 *   * @ai{Union} - merges several other filter results
 * * Steering
 *   * @movement{GroupFlee}
 *   * @movement{GroupSeek}
 *   * @movement{SelectionFlee}
 *   * @movement{SelectionSeek}
 *   * @movement{TargetFlee}
 *   * @movement{TargetSeek}
 *   * @movement{Wander}
 *
 * After implementing actions add them to the registry. Just call
 * @ai{AIRegistry::registerNodeFactory()} on your @ai{AIRegistry} instance and you are ready
 * to write @ai{LUA} scripts with it.
 *
 * Note the usage of a few macros that makes your life easier:
 * * @ai{TASK_CLASS}
 * * @ai{NODE_FACTORY}
 * * @ai{CONDITION_CLASS}
 * * @ai{CONDITION_FACTORY}
 *
 * Now you only have to do:
 * @code
 * AIRegistry registry;
 * registry.registerNodeFactory("ExampleTask", ai::example::ExampleTask::FACTORY);
 *
 * ai::LUATreeLoader loader(registry);
 * loader.init(allMyLuaBehaviourTreesInThisString);
 * const TreeNodePtr& root = loader.load("BehaviourNameEgDefensiveBehaviour");
 * @endcode
 *
 * The root node created by the load method should be given to your @ai{ICharacter}
 * implementation which holds an instance of the @ai{AI} class. Each SimpleAI controlled
 * entity in your world will have one of them.
 *
 * @section debugging Remote Debugging
 *
 * @image html aidebugger.png
 *
 * The remote debugger can also render your custom map widget which allows
 * you to show the characters in their "natural" environment instead of on
 * a boring plane. You can choose which entities should be available for
 * remote debugging. The included `Server` class handles the serialization
 * of the entities. You can create one instance of a server per application map
 * you have. Once you add an entity to the server, it gets automatically
 * serialized and is broadcasted to all connected clients. If no client is
 * connected, nothing is serialized and thus the remote debugging has almost
 * no performance influence.
 *
 * The remote debugging can only be active for one `Zone`. So every other
 * zone is also not affected by the debugging overhead. Usually you would
 * split a zone by logical units, like one map is one zone. But you can also
 * split them up into logical units like boss ai, supporter ai, ... and so on.
 *
 * Features so far:
 * * Render the behaviour tree (either in a table, or as a node tree)
 * * Show the @ref Aggro state
 * * Show the attributes that are assigned to an entity (ICharacter::setAttribute())
 * * Pause execution of a zone
 * * Single step in a @ref Zone (after it was paused)
 * * Reset states of the @ai{AI} in a @ref Zone
 * * Live editing of the behaviour tree (update, add, remove)
 */

/**
 * @file
 *
 * @defgroup AI
 * @{
 */
#pragma once

#include "AIRegistry.h"

#include "tree/loaders/lua/LUATreeLoader.h"
#include "LUAAIRegistry.h"

/**
 * @}
 */
