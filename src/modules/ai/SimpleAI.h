/**
 * @section SimpleAI documentation
 *
 * SimpleAI is a small C++ AI behaviour tree library
 *
 * - [GitHub page](http://github.com/mgerhardy/simpleai/)
 *
 * @section legal Legal
 *
 * Copyright (C) 2015 Martin Gerhardy <martin.gerhardy@gmail.com>
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
 * Main header file that you should include when you use SimpleAI.
 *
 * If you also want to use the default loaders for your behaviour trees, you can
 * include the following loader header files in your code:
 * @code{.cpp}
 * #include <tree/loaders/lua/LUATreeLoader.h>
 * #include <tree/loaders/xml/XMLTreeLoader.h>
 * @endcode
 * or define SIMPLEAI_INCLUDE_LUA and/or SIMPLEAI_INCLUDE_XML
 *
 * SimpleAI uses a right handed coordinate system.
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
#include "common/ThreadPool.h"
#include "common/ThreadScheduler.h"
#include "common/ExecutionTime.h"

#include "AI.h"
#include "AIFactories.h"
#include "AIRegistry.h"
#include "ICharacter.h"

#include "tree/Fail.h"
#include "tree/Limit.h"
#include "tree/Idle.h"
#include "tree/Invert.h"
#include "tree/Parallel.h"
#include "tree/PrioritySelector.h"
#include "tree/Selector.h"
#include "tree/Sequence.h"
#include "tree/Steer.h"
#include "tree/TreeNode.h"
#include "tree/TreeNodeImpl.h"
#include "tree/ITask.h"
#include "tree/ITimedNode.h"
#include "tree/TreeNodeParser.h"
#include "tree/loaders/ITreeLoader.h"

#include "group/GroupId.h"
#include "group/GroupMgr.h"

#include "movement/SelectionSeek.h"
#include "movement/GroupFlee.h"
#include "movement/GroupSeek.h"
#include "movement/Steering.h"
#include "movement/TargetFlee.h"
#include "movement/TargetSeek.h"
#include "movement/Wander.h"
#include "movement/WeightedSteering.h"

#include "server/Network.h"
#include "server/NetworkImpl.h"
#include "server/Server.h"
#include "server/ServerImpl.h"
#include "server/IProtocolHandler.h"
#include "server/ProtocolHandlerRegistry.h"
#include "server/ProtocolMessageFactory.h"
#include "server/AICharacterDetailsMessage.h"
#include "server/AICharacterStaticMessage.h"
#include "server/AIPauseMessage.h"
#include "server/AIStepMessage.h"
#include "server/AISelectMessage.h"
#include "server/AIStateMessage.h"
#include "server/AINamesMessage.h"
#include "server/AIChangeMessage.h"
#include "server/AIAddNodeMessage.h"
#include "server/AIDeleteNodeMessage.h"
#include "server/AIUpdateNodeMessage.h"

#include "zone/Zone.h"

#include "conditions/And.h"
#include "conditions/ICondition.h"
#include "conditions/ConditionParser.h"
#include "conditions/False.h"
#include "conditions/HasEnemies.h"
#include "conditions/IsGroupLeader.h"
#include "conditions/IsInGroup.h"
#include "conditions/Not.h"
#include "conditions/Or.h"
#include "conditions/True.h"

#include "filter/IFilter.h"
#include "filter/SelectEmpty.h"
#include "filter/SelectGroupLeader.h"
#include "filter/SelectGroupMembers.h"
#include "filter/SelectHighestAggro.h"
#include "filter/SelectZone.h"

#ifdef SIMPLEAI_INCLUDE_LUA
#include "tree/loaders/lua/LUATreeLoader.h"
#endif

#ifdef SIMPLEAI_INCLUDE_XML
#include "tree/loaders/xml/XMLTreeLoader.h"
#endif
