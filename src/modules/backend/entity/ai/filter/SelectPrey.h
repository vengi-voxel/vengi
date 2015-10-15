#pragma once

#include <filter/IFilter.h>
#include "core/String.h"
#include "core/Common.h"
#include "network/messages/Shared_generated.h"

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
