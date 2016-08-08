/**
 * @file
 */

#pragma once

#include "backend/entity/ai/AICommon.h"
#include "core/String.h"
#include "core/Common.h"
#include "Shared_generated.h"

using namespace ai;

namespace backend {

class SelectPrey: public IFilter {
private:
	network::EntityType _npcType;
public:
	FILTER_FACTORY(SelectPrey)

	SelectPrey(const std::string& parameters = "");

	void filter(const AIPtr& entity) override;
};

}
