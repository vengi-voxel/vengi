#include "tree/ProbabilitySelector.h"
#include "AI.h"
#include "common/String.h"
#include "common/Random.h"

namespace ai {

ProbabilitySelector::ProbabilitySelector(const std::string& name, const std::string& parameters, const ConditionPtr& condition) :
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

TreeNodeStatus ProbabilitySelector::execute(const AIPtr& entity, int64_t deltaMillis) {
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

NODE_FACTORY_IMPL(ProbabilitySelector)

}
