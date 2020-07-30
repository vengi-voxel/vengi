/**
 * @file
 */

#include "ProbabilitySelector.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/common/Random.h"
#include "core/StringUtil.h"

namespace backend {

ProbabilitySelector::ProbabilitySelector(const core::String& name, const core::String& parameters, const ConditionPtr& condition) :
		Selector(name, parameters, condition), _weightSum(0.0f) {
	std::vector<core::String> tokens;
	core::string::splitString(parameters, tokens, ",");
	const int weightAmount = static_cast<int>(tokens.size());
	for (int i = 0; i < weightAmount; i++) {
		const float weight = core::string::toFloat(tokens[i]);
		_weightSum += weight;
		_weights[i] = weight;
	}
}

ai::TreeNodeStatus ProbabilitySelector::execute(const AIPtr& entity, int64_t deltaMillis) {
	if (Selector::execute(entity, deltaMillis) == ai::TreeNodeStatus::CANNOTEXECUTE)
		return ai::TreeNodeStatus::CANNOTEXECUTE;

	int index = getSelectorState(entity);
	if (index == AI_NOTHING_SELECTED) {
		float rndIndex = randomf(_weightSum);
		const int weightAmount = static_cast<int>(_weights.size());
		for (; index < weightAmount; ++index) {
			if (rndIndex < _weights[index])
				break;
			rndIndex -= _weights[index];
		}
	}

	const TreeNodePtr& child = _children[index];
	const ai::TreeNodeStatus result = child->execute(entity, deltaMillis);
	if (result == ai::TreeNodeStatus::RUNNING) {
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

}
