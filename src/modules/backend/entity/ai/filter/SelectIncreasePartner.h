/**
 * @file
 */

#pragma once

#include "cooldown/CooldownType.h"
#include "backend/entity/ai/AICommon.h"

using namespace ai;

namespace backend {

/**
 * @ingroup AI
 */
class SelectIncreasePartner: public IFilter {
private:
	cooldown::Type _cooldownId;
public:
	FILTER_FACTORY(SelectIncreasePartner)

	SelectIncreasePartner(const std::string& parameters = "");

	void filter(const AIPtr& entity) override;
};

}
