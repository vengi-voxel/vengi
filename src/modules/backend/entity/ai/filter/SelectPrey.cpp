#include "SelectPrey.h"
#include "entity/Npc.h"

namespace backend {

SelectPrey::SelectPrey(const std::string& parameters) :
		IFilter("SelectPrey", parameters), _npcType(network::messages::NpcType_NONE) {
	const char **names = network::messages::EnumNamesNpcType();
	int i = 0;
	while (*names) {
		if (!strcmp(*names, parameters.c_str())) {
			_npcType = static_cast<network::messages::NpcType>(i);
			break;
		}
		++i;
		++names;
	}
	core_assert(_npcType > network::messages::NpcType_NONE);
	core_assert(_npcType < network::messages::NpcType_MAX);
}

void SelectPrey::filter(const AIPtr& entity) {
	FilteredEntities& entities = getFilteredEntities(entity);
	backend::Npc& chr = ai::character_cast<AICharacter>(entity->getCharacter()).getNpc();
	chr.visitVisible([&] (const backend::EntityPtr& e) {
		if (e->npcType() == _npcType)
			entities.push_back(e->id());
	});
}

FILTER_FACTORY_IMPL(SelectPrey)

}
