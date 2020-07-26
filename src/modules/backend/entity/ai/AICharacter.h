/**
 * @file
 */

#pragma once

#include "ICharacter.h"
#include "AI.h"

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

typedef core::SharedPtr<AICharacter> AICharacterPtr;

inline Npc& getNpc(const AIPtr& ai) {
	const ICharacterPtr& chr = ai->getCharacter();
	return static_cast<AICharacter*>(chr.get())->getNpc();
}

}
