/**
 * @file
 */

#pragma once

#include <memory>

namespace ai {

class ICharacter;
typedef std::shared_ptr<ICharacter> ICharacterPtr;
class Zone;

class AI;
typedef std::shared_ptr<AI> AIPtr;

class TreeNode;
typedef std::shared_ptr<TreeNode> TreeNodePtr;

}

namespace backend {

class AICharacter;
typedef std::shared_ptr<AICharacter> AICharacterPtr;

}
