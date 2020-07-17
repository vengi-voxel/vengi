/**
 * @file
 * @ingroup Filter
 */
#pragma once

#include "IFilter.h"
#include "backend/entity/ai/zone/Zone.h"

namespace backend {

/**
 * @brief This filter will pick the group leader of the specified group
 */
class SelectGroupLeader: public IFilter {
protected:
	GroupId _groupId;
public:
	FILTER_FACTORY(SelectGroupLeader)

	explicit SelectGroupLeader(const core::String& parameters = "");

	void filter (const AIPtr& entity) override;
};

}
