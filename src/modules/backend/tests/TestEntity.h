/**
 * @file
 */

#pragma once

#include "backend/entity/ai/ICharacter.h"

class TestEntity : public backend::ICharacter {
public:
	TestEntity (const ai::CharacterId& id) :
			backend::ICharacter(id) {
	}
};
