#include "tree/Sequence.h"
#include "AI.h"

namespace ai {

TreeNodeStatus Sequence::execute(const AIPtr& entity, int64_t deltaMillis) {
	if (Selector::execute(entity, deltaMillis) == CANNOTEXECUTE)
		return CANNOTEXECUTE;

	TreeNodeStatus result = FINISHED;
	const int progress = std::max(0, getSelectorState(entity));

	const std::size_t size = _children.size();
	for (std::size_t i = static_cast<std::size_t>(progress); i < size; ++i) {
		TreeNodePtr& child = _children[i];

		result = child->execute(entity, deltaMillis);

		if (result == RUNNING) {
			setSelectorState(entity, static_cast<int>(i));
			break;
		} else if (result == CANNOTEXECUTE || result == FAILED) {
			resetState(entity);
			break;
		} else if (result == EXCEPTION) {
			break;
		}
	}

	if (result != RUNNING) {
		resetState(entity);
	}

	return state(entity, result);
}

void Sequence::resetState(const AIPtr& entity) {
	setSelectorState(entity, AI_NOTHING_SELECTED);
	TreeNode::resetState(entity);
}

NODE_FACTORY_IMPL(Sequence)

}
