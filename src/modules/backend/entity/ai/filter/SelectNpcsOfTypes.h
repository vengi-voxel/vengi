/**
 * @file
 */

#pragma once

#include "backend/entity/ai/AICommon.h"
#include "core/String.h"
#include "core/Common.h"
#include "Shared_generated.h"
#include <bitset>

using namespace ai;

namespace backend {

class SelectNpcsOfTypes: public IFilter {
private:
	std::bitset<(int)network::EntityType::MAX> _npcTypes;
public:
	FILTER_FACTORY(SelectNpcsOfTypes)

	SelectNpcsOfTypes(const std::string& parameters) :
			IFilter("SelectNpcsOfTypes", parameters) {
		std::vector<std::string> types;
		core::string::splitString(parameters, types, ",");
		for (const std::string& type : types) {
			auto npcType = network::getEnum<network::EntityType>(type.c_str(), network::EnumNamesEntityType());
			core_assert(npcType != network::EntityType::NONE);
			_npcTypes[(int)npcType] = true;
		}
	}

	void filter(const AIPtr& entity) override {
		FilteredEntities& entities = getFilteredEntities(entity);
		backend::Npc& chr = ai::character_cast<AICharacter>(entity->getCharacter()).getNpc();
		chr.visitVisible([&] (const backend::EntityPtr& e) {
			if (!_npcTypes[(int)e->npcType()]) {
				return;
			}
			entities.push_back(e->id());
		});
	}
};

}
