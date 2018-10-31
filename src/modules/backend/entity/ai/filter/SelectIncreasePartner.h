/**
 * @file
 */

#pragma once

#include "cooldown/CooldownType.h"
#include "backend/entity/ai/AICommon.h"

namespace backend {

/**
 * @brief Select entities of the same type and triggers a cooldown to not select
 * the same entity again and again.
 * @ingroup AI
 */
class SelectIncreasePartner: public ai::IFilter {
private:
	cooldown::Type _cooldownId;
public:
	FILTER_FACTORY(SelectIncreasePartner)

	/**
	 * @param[in] parameters The parameters given by the script. Must be
	 * the cooldown id that is triggered
	 */
	SelectIncreasePartner(const std::string& parameters = "");

	void filter(const ai::AIPtr& entity) override;
};

}
