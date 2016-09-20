/**
 * @file
 */

#pragma once

#include "backend/entity/ai/AICommon.h"
#include "Shared_generated.h"

using namespace ai;

namespace backend {

/**
 * @ingroup AI
 */
class SelectPrey: public IFilter {
private:
	network::EntityType _npcType;
public:
	FILTER_FACTORY(SelectPrey)

	SelectPrey(const std::string& parameters = "");

	void filter(const AIPtr& entity) override;
};

}
