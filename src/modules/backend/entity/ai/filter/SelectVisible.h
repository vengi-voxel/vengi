#pragma once

#include <filter/IFilter.h>

using namespace ai;

namespace backend {

class SelectVisible: public IFilter {
public:
	FILTER_FACTORY

	SelectVisible(const std::string& parameters = "") :
		IFilter("SelectVisible", parameters) {
	}

	void filter (const AIPtr& entity) override;
};

}
