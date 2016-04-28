#pragma once

#include "backend/entity/ai/AICommon.h"
#include "core/String.h"
#include "core/Common.h"
#include "messages/Shared_generated.h"

using namespace ai;

namespace backend {

class SelectPrey: public IFilter {
private:
	network::messages::NpcType _npcType;
public:
	FILTER_FACTORY

	SelectPrey(const std::string& parameters = "");

	void filter(const AIPtr& entity) override;
};

}
