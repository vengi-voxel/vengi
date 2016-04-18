#pragma once

#include "core/String.h"
#include "core/Common.h"
#include "cooldown/CooldownType.h"
#include "backend/entity/ai/AICommon.h"

using namespace ai;

namespace backend {

class SelectIncreasePartner: public IFilter {
private:
	cooldown::CooldownType _cooldownId;
public:
	FILTER_FACTORY

	SelectIncreasePartner(const std::string& parameters = "") :
			IFilter("SelectIncreasePartner", parameters) {
		_cooldownId = static_cast<cooldown::CooldownType>(core::string::toInt(parameters));
		core_assert(_cooldownId > cooldown::NONE);
		core_assert(_cooldownId < cooldown::MAX);
	}

	void filter(const AIPtr& entity) override;
};

}
