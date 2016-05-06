/**
 * @file
 */

#pragma once

#include "backend/entity/ai/AICommon.h"
#include "core/String.h"
#include "core/Common.h"
#include "messages/Shared_generated.h"
#include <bitset>

using namespace ai;

namespace backend {

class SelectNpcsOfTypes: public IFilter {
private:
	std::bitset<network::messages::NpcType::NpcType_MAX> _npcTypes;
public:
	FILTER_FACTORY

	SelectNpcsOfTypes(const std::string& parameters) :
			IFilter("SelectNpcsOfTypes", parameters) {
		const char **names = network::messages::EnumNamesNpcType();
		std::vector<std::string> types;
		core::string::splitString(parameters, types, ",");
		for (const std::string& type : types) {
			int i = 0;
			while (*names) {
				if (!strcmp(*names, type.c_str())) {
					_npcTypes[i] = true;
					break;
				}
				++i;
				++names;
			}
		}
	}

	void filter(const AIPtr& entity) override {
		FilteredEntities& entities = getFilteredEntities(entity);
		backend::Npc& chr = ai::character_cast<AICharacter>(entity->getCharacter()).getNpc();
		chr.visitVisible([&] (const backend::EntityPtr& e) {
			if (!_npcTypes[e->npcType()])
				return;
			entities.push_back(e->id());
		});
	}
};

FILTER_FACTORY_IMPL(SelectNpcsOfTypes)

}
