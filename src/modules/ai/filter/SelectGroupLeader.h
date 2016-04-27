#pragma once

#include "filter/IFilter.h"

namespace ai {

/**
 * @brief This filter will pick the group leader of the specified group
 */
class SelectGroupLeader: public IFilter {
protected:
	GroupId _groupId;
public:
	FILTER_FACTORY

	SelectGroupLeader(const std::string& parameters = "") :
		IFilter("SelectGroupLeader", parameters) {
		if (_parameters.empty())
			_groupId = -1;
		else
			_groupId = std::stoi(_parameters);
	}

	void filter (const AIPtr& entity) override;
};

}
