/**
 * @file
 */

#pragma once

#include "AICommon.h"

namespace backend {

class Npc;

class AICharacter : public ai::ICharacter {
private:
	Npc& _npc;
public:
	AICharacter(ai::CharacterId id, Npc& npc);
	~AICharacter();
	void update(long dt, bool debuggingActive) override;

	inline Npc& getNpc() const {
		return _npc;
	}
};

}
