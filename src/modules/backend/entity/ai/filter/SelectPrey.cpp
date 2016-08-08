/**
 * @file
 */

#include "SelectPrey.h"
#include "backend/entity/Npc.h"
#include "core/Common.h"

namespace backend {

SelectPrey::SelectPrey(const std::string& parameters) :
		IFilter("SelectPrey", parameters), _npcType(network::EntityType::NONE) {
	const char **names = network::EnumNamesEntityType();
	int i = 0;
	while (*names) {
		if (!strcmp(*names, parameters.c_str())) {
			_npcType = static_cast<network::EntityType>(i);
			break;
		}
		++i;
		++names;
	}
	core_assert(_npcType > network::EntityType::NONE);
	core_assert(_npcType < network::EntityType::MAX);
}

void SelectPrey::filter(const AIPtr& entity) {
	FilteredEntities& entities = getFilteredEntities(entity);
	backend::Npc& chr = ai::character_cast<AICharacter>(entity->getCharacter()).getNpc();
	chr.visitVisible([&] (const backend::EntityPtr& e) {
		if (e->npcType() == _npcType)
			entities.push_back(e->id());
	});
}

}
