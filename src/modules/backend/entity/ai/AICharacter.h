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
	AICharacter(ai::CharacterId id, Npc& npc);
	~AICharacter();
	void update(int64_t dt, bool debuggingActive) override;

	void setPosition(const glm::vec3& position) override;

	void setOrientation(float orientation) override;

	inline Npc& getNpc() const {
		return _npc;
	}
};

typedef std::shared_ptr<AICharacter> AICharacterPtr;

inline Npc& getNpc(const AIPtr& ai) {
	return character_cast<AICharacter>(ai->getCharacter()).getNpc();
}

}
