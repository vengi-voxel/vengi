/**
 * @file
 */

#pragma once

#include "AICommon.h"

namespace backend {

class Npc;

/**
 * @ingroup AI
 */
class AICharacter : public ai::ICharacter {
private:
	Npc& _npc;
public:
	AICharacter(ai::CharacterId id, Npc& npc);
	~AICharacter();
	void update(int64_t dt, bool debuggingActive) override;

	inline Npc& getNpc() const {
		return _npc;
	}
};

}
