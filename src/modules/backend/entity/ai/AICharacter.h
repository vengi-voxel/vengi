/**
 * @file
 */

#pragma once

#include "ICharacter.h"
#include "AI.h"
#include <memory>

namespace backend {

class Npc;

/**
 * @ingroup AI
 */
class AICharacter : public ICharacter {
private:
	using Super = ICharacter;
	Npc& _npc;
public:
	AICharacter(ai::CharacterId id, Npc& npc) :
			Super(id), _npc(npc) {
	}

	inline Npc& getNpc() const {
		return _npc;
	}
};

typedef std::shared_ptr<AICharacter> AICharacterPtr;

inline Npc& getNpc(const AIPtr& ai) {
	return (static_cast<AICharacter*>(ai->getCharacter().get()))->getNpc();
}

}
