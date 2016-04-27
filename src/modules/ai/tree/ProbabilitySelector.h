#pragma once

#include "tree/Selector.h"
#include <vector>

namespace ai {

/**
 * @brief This node executes one of the attached children randomly based on the given weights. The node is
 * executed until it is no longer in the running state
 *
 * http://aigamedev.com/open/article/selector/
 */
class ProbabilitySelector: public Selector {
protected:
	std::vector<float> _weights;
	float _weightSum;
public:
	ProbabilitySelector(const std::string& name, const std::string& parameters, const ConditionPtr& condition);

	virtual ~ProbabilitySelector() {
	}

	NODE_FACTORY

	TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override;
};

}
