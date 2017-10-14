/**
 * @file
 */

#include "SelectPrey.h"
#include "backend/entity/Npc.h"
#include "backend/entity/ai/AICharacter.h"
#include "core/Common.h"
#include "core/String.h"

namespace backend {

SelectPrey::SelectPrey(const std::string& parameters) :
		IFilter("SelectPrey", parameters), _npcType(network::EntityType::NONE) {
	_npcType = network::getEnum<network::EntityType>(parameters.c_str(), network::EnumNamesEntityType());
	core_assert_always(_npcType != network::EntityType::NONE);
}

void SelectPrey::filter(const AIPtr& entity) {
	FilteredEntities& entities = getFilteredEntities(entity);
	backend::Npc& chr = getNpc(entity);
	chr.visitVisible([&] (const backend::EntityPtr& e) {
		if (e->entityType() == _npcType) {
			entities.push_back(e->id());
		}
	});
}

}
