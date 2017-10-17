/**
 * @file
 */

#pragma once

#include "Shared_generated.h"
#include "backend/entity/ai/AICommon.h"

using namespace ai;

namespace backend {

/**
 * @ingroup AI
 */
// TODO: remove me and use SelectEntitiesOfTypes instead
class SelectPrey: public IFilter {
private:
	network::EntityType _npcType;
public:
	FILTER_FACTORY(SelectPrey)

	SelectPrey(const std::string& parameters = "");

	void filter(const AIPtr& entity) override;
};

}
