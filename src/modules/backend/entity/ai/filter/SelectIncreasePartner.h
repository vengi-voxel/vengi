/**
 * @file
 */

#pragma once

#include "cooldown/CooldownType.h"
#include "backend/entity/ai/filter/IFilter.h"

namespace backend {

/**
 * @brief Select entities of the same type and triggers a cooldown to not select
 * the same entity again and again.
 * @ingroup AI
 */
class SelectIncreasePartner: public IFilter {
private:
	cooldown::Type _cooldownId;
public:
	FILTER_FACTORY(SelectIncreasePartner)

	/**
	 * @param[in] parameters The parameters given by the script. Must be
	 * the cooldown id that is triggered
	 */
	SelectIncreasePartner(const core::String& parameters = "");

	void filter(const AIPtr& entity) override;
};

}
