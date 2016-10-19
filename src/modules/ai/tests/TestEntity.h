#pragma once

#include <SimpleAI.h>
#include <list>

class TestEntity : public ai::ICharacter {
public:
	TestEntity (const ai::CharacterId& id) :
			ai::ICharacter(id) {
	}
};
