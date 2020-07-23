/**
 * @file
 * @ingroup Filter
 */
#pragma once

#include "IFilter.h"
#include "backend/entity/ai/group/GroupId.h"

namespace backend {

/**
 * @brief This filter will pick the entities from the groups the given @c AI instance is in
 */
class SelectGroupMembers: public IFilter {
protected:
	GroupId _groupId;
public:
	FILTER_FACTORY(SelectGroupMembers)

	explicit SelectGroupMembers(const core::String& parameters = "");

	void filter (const AIPtr& entity) override;
};

}
