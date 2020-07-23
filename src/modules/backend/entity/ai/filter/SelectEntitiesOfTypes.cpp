/**
 * @file
 */

#include "SelectEntitiesOfTypes.h"
#include "core/StringUtil.h"
#include "core/Common.h"
#include "backend/entity/Npc.h"
#include "backend/entity/ai/AICharacter.h"
#include "network/ProtocolEnum.h"
#include "core/Enum.h"
#include <vector>

namespace backend {

SelectEntitiesOfTypes::SelectEntitiesOfTypes(const core::String& parameters) :
		IFilter("SelectEntitiesOfTypes", parameters) {
	std::vector<core::String> types;
	core::string::splitString(parameters, types, ",");
	for (const core::String& type : types) {
		auto entityType = network::getEnum<network::EntityType>(type.c_str(), network::EnumNamesEntityType());
		core_assert_always(entityType != network::EntityType::NONE);
		_entityTypes[core::enumVal(entityType)] = true;
	}
}

void SelectEntitiesOfTypes::filter(const AIPtr& entity) {
	FilteredEntities& entities = getFilteredEntities(entity);
	backend::Npc& chr = getNpc(entity);
	chr.visitVisible([&] (const backend::EntityPtr& e) {
		if (!_entityTypes[core::enumVal(e->entityType())]) {
			return;
		}
		entities.push_back(e->id());
	});
}

}
