/**
 * @file
 */
#pragma once

#include "Selector.h"
#include <vector>

namespace backend {

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
	ProbabilitySelector(const core::String& name, const core::String& parameters, const ConditionPtr& condition);

	virtual ~ProbabilitySelector() {
	}

	NODE_FACTORY(ProbabilitySelector)

	ai::TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override;
};

}
