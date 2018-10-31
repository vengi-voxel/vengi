/**
 * @file
 */

#pragma once

#include "Shared_generated.h"
#include "backend/entity/ai/AICommon.h"
#include "core/Common.h"
#include <bitset>

namespace backend {

/**
 * @ingroup AI
 */
class SelectEntitiesOfTypes: public ai::IFilter {
private:
	std::bitset<std::enum_value(network::EntityType::MAX)> _entityTypes;
public:
	FILTER_FACTORY(SelectEntitiesOfTypes)

	SelectEntitiesOfTypes(const std::string& parameters);

	void filter(const ai::AIPtr& entity) override;
};

}
