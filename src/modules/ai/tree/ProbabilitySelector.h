/**
 * @file
 */
#pragma once

#include "tree/Selector.h"
#include <vector>
#include "AI.h"
#include "common/StringUtil.h"
#include "common/Random.h"

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
	ProbabilitySelector(const core::String& name, const core::String& parameters, const ConditionPtr& condition) :
			Selector(name, parameters, condition), _weightSum(0.0f) {
		std::vector<std::string> tokens;
		Str::splitString(parameters, tokens, ",");
		const int weightAmount = static_cast<int>(tokens.size());
		for (int i = 0; i < weightAmount; i++) {
			const float weight = Str::strToFloat(tokens[i]);
			_weightSum += weight;
			_weights[i] = weight;
		}
	}

	virtual ~ProbabilitySelector() {
	}

	NODE_FACTORY(ProbabilitySelector)

	TreeNodeStatus execute(const AIPtr& entity, int64_t deltaMillis) override {
		if (Selector::execute(entity, deltaMillis) == CANNOTEXECUTE)
			return CANNOTEXECUTE;

		int index = getSelectorState(entity);
		if (index == AI_NOTHING_SELECTED) {
			float rndIndex = ai::randomf(_weightSum);
			const int weightAmount = static_cast<int>(_weights.size());
			for (; index < weightAmount; ++index) {
				if (rndIndex < _weights[index])
					break;
				rndIndex -= _weights[index];
			}
		}

		const TreeNodePtr& child = _children[index];
		const TreeNodeStatus result = child->execute(entity, deltaMillis);
		if (result == RUNNING) {
			setSelectorState(entity, index);
		} else {
			setSelectorState(entity, AI_NOTHING_SELECTED);
		}
		child->resetState(entity);

		const int size = static_cast<int>(_children.size());
		for (int i = 0; i < size; ++i) {
			if (i == index)
				continue;
			_children[i]->resetState(entity);
		}
		return state(entity, result);
	}
};

}
