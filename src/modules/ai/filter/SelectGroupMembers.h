#pragma once

#include "filter/IFilter.h"

namespace ai {

/**
 * @brief This filter will pick the entities from the groups the given @c AI instance is in
 */
class SelectGroupMembers: public IFilter {
protected:
	GroupId _groupId;
public:
	FILTER_FACTORY

	SelectGroupMembers(const std::string& parameters = "") :
		IFilter("SelectGroupMembers", parameters) {
		if (_parameters.empty())
			_groupId = -1;
		else
			_groupId = std::stoi(_parameters);
	}

	void filter (const AIPtr& entity) override;
};

}
