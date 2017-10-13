/**
 * @file
 */

#pragma once

#include "backend/entity/ai/AICommon.h"
#include "core/Common.h"
#include "Shared_generated.h"
#include <bitset>

using namespace ai;

namespace backend {

/**
 * @ingroup AI
 */
class SelectEntitiesOfTypes: public IFilter {
private:
	std::bitset<std::enum_value(network::EntityType::MAX)> _entityTypes;
public:
	FILTER_FACTORY(SelectEntitiesOfTypes)

	SelectEntitiesOfTypes(const std::string& parameters);

	void filter(const AIPtr& entity) override;
};

}
