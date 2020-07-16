/**
 * @file
 */

#pragma once

#include "Shared_generated.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/Enum.h"
#include "backend/entity/ai/filter/IFilter.h"
#include <bitset>

namespace backend {

/**
 * @ingroup AI
 */
class SelectEntitiesOfTypes: public IFilter {
private:
	std::bitset<core::enumVal(network::EntityType::MAX)> _entityTypes;
public:
	FILTER_FACTORY(SelectEntitiesOfTypes)

	SelectEntitiesOfTypes(const core::String& parameters);

	void filter(const AIPtr& entity) override;
};

}
