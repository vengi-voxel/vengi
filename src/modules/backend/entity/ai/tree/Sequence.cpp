/**
 * @file
 */

#include "Sequence.h"
#include "core/Common.h"
#include "backend/entity/ai/AI.h"

namespace backend {

ai::TreeNodeStatus Sequence::execute(const AIPtr& entity, int64_t deltaMillis) {
	if (Selector::execute(entity, deltaMillis) == ai::TreeNodeStatus::CANNOTEXECUTE)
		return ai::TreeNodeStatus::CANNOTEXECUTE;

	ai::TreeNodeStatus result = ai::TreeNodeStatus::FINISHED;
	const int progress = core_max(0, getSelectorState(entity));

	const std::size_t size = _children.size();
	for (std::size_t i = static_cast<std::size_t>(progress); i < size; ++i) {
		TreeNodePtr& child = _children[i];

		result = child->execute(entity, deltaMillis);

		if (result == ai::TreeNodeStatus::RUNNING) {
			setSelectorState(entity, static_cast<int>(i));
			break;
		} else if (result == ai::TreeNodeStatus::CANNOTEXECUTE || result == ai::TreeNodeStatus::FAILED) {
			resetState(entity);
			break;
		} else if (result == ai::TreeNodeStatus::EXCEPTION) {
			break;
		}
	}

	if (result != ai::TreeNodeStatus::RUNNING) {
		resetState(entity);
	}

	return state(entity, result);
}

void Sequence::resetState(const AIPtr& entity) {
	setSelectorState(entity, AI_NOTHING_SELECTED);
	TreeNode::resetState(entity);
}

}
