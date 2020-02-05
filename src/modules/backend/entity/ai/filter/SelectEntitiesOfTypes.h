/**
 * @file
 */

#pragma once

#include "Shared_generated.h"
#include "backend/entity/ai/AICommon.h"
#include "core/Common.h"
#include <bitset>
#include "core/Enum.h"

namespace backend {

/**
 * @ingroup AI
 */
class SelectEntitiesOfTypes: public ai::IFilter {
private:
	std::bitset<core::enumVal(network::EntityType::MAX)> _entityTypes;
public:
	FILTER_FACTORY(SelectEntitiesOfTypes)

	SelectEntitiesOfTypes(const core::String& parameters);

	void filter(const ai::AIPtr& entity) override;
};

}
