/**
 * @file
 */

#include "ITimedNode.h"
#include <stdlib.h>
#define NOTSTARTED -1

namespace backend {

ITimedNode::ITimedNode(const core::String& name, const core::String& parameters, const ConditionPtr& condition) :
		TreeNode(name, parameters, condition), _timerMillis(NOTSTARTED) {
	if (!parameters.empty()) {
		_millis = ::atol(parameters.c_str());
	} else {
		_millis = 1000L;
	}
}

ai::TreeNodeStatus ITimedNode::execute(const AIPtr& entity, int64_t deltaMillis) {
	const ai::TreeNodeStatus result = TreeNode::execute(entity, deltaMillis);
	if (result == ai::TreeNodeStatus::CANNOTEXECUTE)
		return ai::TreeNodeStatus::CANNOTEXECUTE;

	if (_timerMillis == NOTSTARTED) {
		_timerMillis = _millis;
		const ai::TreeNodeStatus status = executeStart(entity, deltaMillis);
		if (status == ai::TreeNodeStatus::FINISHED)
			_timerMillis = NOTSTARTED;
		return state(entity, status);
	}

	if (_timerMillis - deltaMillis > 0) {
		_timerMillis -= deltaMillis;
		const ai::TreeNodeStatus status = executeRunning(entity, deltaMillis);
		if (status == ai::TreeNodeStatus::FINISHED)
			_timerMillis = NOTSTARTED;
		return state(entity, status);
	}

	_timerMillis = NOTSTARTED;
	return state(entity, executeExpired(entity, deltaMillis));
}

#undef NOTSTARTED

}
