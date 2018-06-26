/**
 * @page SimpleAI SimpleAI documentation
 *
 * @section purpose Purpose
 *
 * SimpleAI is a small and (hopefully) easy to use C++ library for behaviour
 * tree based @ai{AI}. It's main focus is about games - but of course it can be
 * used for other things, too. The project is released under the MIT license
 * and thus can be used for open source as well as commercial and closed
 * source projects. If you use it, it would be nice to provide a link to the
 * github page.
 *
 * - [GitHub page](http://github.com/mgerhardy/simpleai/)
 * - Small example use case:
 *   - https://github.com/mgerhardy/engine/tree/master/src/modules/backend/entity/ai
 *
 * @section features Features
 *
 * * Header only c++11
 * * Threadsafe (hopefully, checked with hellgrind, tsan and other tools)
 * * LUA and XML script interface to create the trees - but every other data source
 *   is possible, too. The LUA and XML script interfaces are just implemented as an
 *   [example](https://github.com/mgerhardy/simpleai/blob/master/src/run/behaviours.lua)
 *   even though they are of course fully usable to create behaviour trees.
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
 * @section compilation Compile the lib:
 *
 * * autotools based compilation
 *   * `./autogen.sh`
 *   * `./configure`
 *   * `make`
 *   * `make install`
 * * cmake based compilation (enable tests, remote debugger and so on via options)
 *   * `cmake CMakeLists.txt`
 *   * `make`
 * * Compile the remote debugger
 *   * qmake
 *     * `cd src/debug`
 *     * `qmake`
 *     * `make`
 *   * cmake based compilation
 *     * `cmake CMakeLists.txt -DSIMPLEAI_DEBUGGER=ON`
 *     * `make`
 *
 * @section run Running it:
 *
 * SimpleAI comes with a small tool that is located in src/run. You can
 * execute your own trees with:
 *
 * - `./simpleai-run -file src/run/behaviours.lua`
 *
 * After you ran it, you can connect with the remote @ref debugger and inspect the live
 * state of every spawned @ai{AI} entity.
 *
 * @section using Using it:
 * * Make sure your character extends @ai{ICharacter} or includes it as component.
 * * Implement your behaviour tree loader by extending the class @ai{ITreeloader}.
 * * Extend the @debug{AIDebugger} to deliver your own @debug{MapView} that renders the map of
 *   your application.
 * * Add your own condition, filter and task factories to the @ai{AIRegistry} or via @ai{LUAAIRegistry}.
 * * Assign attributes to your characters that should be shown in the
 *   debuggers live view.
 *
 * As the name states, it should be easy to use and to integrate into your application. You
 * have the ability to create new customized actions, override existing ones with your
 * own implementations and so on.
 *
 * To integrate the @ai{AI} into your application, your entity class should implement or include
 * the @ai{ICharacter} interface. You only have to call the @ai{ICharacter::update()} method
 * to get the @ai{AI} and the character updated. You can view the included `simpleai-run` tool
 * for getting an idea on how to do this.
 *
 * Once this step is done, you can go on with creating new actions for your application. All you
 * have to do for this is to extend @ai{ITask}. The entity instance given to the
 * @ai{ITask::doAction()} method contains the @ai{ICharacter} that you bound to your
 * application entity.
 *
 * After implementing these actions, all you have to do in order to use them with e.g. the
 * existing @ai{LUATreeLoader} is to add them to the registry. Just call
 * @ai{AIRegistry::registerNodeFactory()} on your @ai{AIRegistry} instance and you are ready
 * to write @ai{LUA} scripts with it. Again, as a reference, just check out the example code.
 * You can also create nodes, conditions, filter and steering methods via LUA directly. See
 * @ai{LUAAIRegistry} for more information about this.
 *
 * Note the usage of a few macros that makes your life easier:
 * * @ai{TASK_CLASS}
 * * @ai{NODE_FACTORY}
 * * @ai{CONDITION_CLASS}
 * * @ai{CONDITION_FACTORY}
 *
 * Now you only have to do:
 * @code
 * ai::AIRegistry registry;
 * registry.registerNodeFactory("ExampleTask", ai::example::ExampleTask::FACTORY);
 *
 * ai::LUATreeLoader loader(registry);
 * loader.init(allMyLuaBehaviourTreesInThisString);
 * const ai::TreeNodePtr& root = loader.load("BehaviourNameEgDefensiveBehaviour");
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
 * * Show the attributes that are assigned to an entity (ai::ICharacter::setAttribute())
 * * Pause execution of a zone
 * * Single step in a @ref Zone (after it was paused)
 * * Reset states of the @ai{AI} in a @ref Zone
 * * Live editing of the behaviour tree (update, add, remove)
 *
 * Examples on how to customize the debugger
 * * [some classes](https://github.com/mgerhardy/simpleai/blob/master/contrib/exampledebugger) that provide a custom map view and map item rendering with custom data from attributes.
 *
 * @section legal Legal
 *
 * Copyright (C) 2015-2017 Martin Gerhardy <martin.gerhardy@gmail.com>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/**
 * @file
 *
 * @defgroup AI
 * @{
 *
 * Main header file that you should include when you use SimpleAI.
 *
 * If you also want to use the default loaders for your behaviour trees, you can
 * include the following loader header files in your code:
 * @code{.cpp}
 * #include <tree/loaders/lua/LUATreeLoader.h>
 * #include <tree/loaders/xml/XMLTreeLoader.h>
 * @endcode
 * or define AI_INCLUDE_LUA and/or AI_INCLUDE_XML
 *
 * You can control how to allocate memory by defining your own allocator class via @c AI_ALLOCATOR_CLASS
 *
 * SimpleAI uses a left handed coordinate system with y pointing upwards (meaning, if
 * you are using a 2d application, only handle x and z).
 */
#pragma once

#include "common/Types.h"
#include "common/MemoryAllocator.h"
#include "common/String.h"
#include "common/Math.h"
#include "common/Random.h"
#include "common/Log.h"
#include "common/MoveVector.h"
#include "common/Random.h"
#include "common/Thread.h"
#include "common/ExecutionTime.h"

#include "AI.h"
#include "AIFactories.h"
#include "AIRegistry.h"
#include "ICharacter.h"

#include "tree/TreeNode.h"
#include "tree/ITask.h"
#include "tree/ITimedNode.h"
#include "tree/TreeNodeParser.h"
#include "tree/loaders/ITreeLoader.h"

#include "group/GroupId.h"
#include "group/GroupMgr.h"

#include "server/Server.h"

#include "zone/Zone.h"

#include "conditions/ICondition.h"
#include "filter/IFilter.h"

#ifdef AI_INCLUDE_LUA
#include "tree/loaders/lua/LUATreeLoader.h"
#include "LUAAIRegistry.h"
#endif

#ifdef AI_INCLUDE_XML
#include "tree/loaders/xml/XMLTreeLoader.h"
#endif

/**
 * @}
 */
